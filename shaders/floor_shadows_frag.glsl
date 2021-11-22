#version 430 core
out vec4 FragColor;

in VS_OUT {
    vec3 fragPos;
    vec3 normal;
    vec2 texCoords;
    vec4 fragPosLightSpace;
    vec4 fragPosSpotSpace;
} fs_in;

struct Light {
    vec3 position;
    vec3 direction;
    float cutOff; // max angle at which it gives full light
    float outerCutOff; // max angle at which it gives any light
    bool directional;
    float width;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

layout (std140, binding = 0) uniform shadowBlock {
    vec2 shadowTexelSize;
    vec2 poissonDisk[32];
    float poissonSpread;
    int NUM_SEARCH_SAMPLES;
    int NUM_PCF_SAMPLES;
};

uniform sampler2D floorTexture;
uniform sampler2D randomAngles;
uniform sampler2D shadowMap;
uniform sampler2D spotShadowMap;
uniform vec3 viewPos;
uniform Light spotLight;
uniform Light dirLight;

float estimateBlockerDepth(vec3 projCoords, Light light, sampler2D map, vec4 rotation) {
    // Calculate size of blocker search
    float searchWidth = max(2.0, light.width * projCoords.z / 10.0);

    // Calculate average blocker depth
    float blockerDepth = 0.0;
    int numBlockers = 0;

    for (int i = 0; i <= NUM_SEARCH_SAMPLES; ++i) {
        vec2 offset = vec2(poissonDisk[i].x * rotation.x,
                           poissonDisk[i].y * rotation.y);
        float depth = texture(map, projCoords.xy + offset * shadowTexelSize * searchWidth).r;
        if (depth < projCoords.z) {
            blockerDepth += depth;
            ++numBlockers;
        }
    }
    return blockerDepth /= numBlockers;
}

float shadowCalculation(vec4 pos, float ndotl, sampler2D map, Light light) {
    // perform perspective divide
    vec3 projCoords = pos.xyz / pos.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    float shadow = 0.0;
    // Check if fragment is beyond far plane of frustum
    if (projCoords.z <= 1.0) {

        // Get random coordinates to rotate poisson disk
        ivec2 angleTexCoords = ivec2(fs_in.fragPos.x, fs_in.fragPos.y);
        vec4 rotation = texture(randomAngles, angleTexCoords, 0);

        float bias = max(0.02 * (1.0 -  ndotl), 0.005);
        projCoords.z -= bias;

        // Estimate average blocker depth
        float blockerDepth = estimateBlockerDepth(projCoords, light, map, rotation);

        // Use PCF to calculate shadow value
        if (blockerDepth > 0.0) {
            float wPenumbra = max(2.0, (projCoords.z - blockerDepth) * light.width / blockerDepth * 200.0);
            for (int i = 0; i <= NUM_PCF_SAMPLES; ++i) {
                vec2 offset = vec2(poissonDisk[i].x * rotation.x,
                                   poissonDisk[i].y * rotation.y);
                float depth = texture(map, projCoords.xy + offset * shadowTexelSize * wPenumbra).r;
                shadow += depth < projCoords.z ? 0.0 : 1.0;
            }
            shadow /= NUM_PCF_SAMPLES;
        }
        else
            shadow = 1.0;
    }
    else
        shadow = 1.0;

    return shadow;
}

void main() {           
    vec3 color = texture(floorTexture, fs_in.texCoords).rgb;

    // Directional light
    // ambient
    vec3 ambient = dirLight.ambient * color;
    // diffuse
    vec3 lightDir = normalize(-dirLight.direction);
    vec3 normal = normalize(fs_in.normal);
    float ndotl = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = ndotl * color * dirLight.diffuse;
    // specular
    vec3 viewDir = normalize(viewPos - fs_in.fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = dirLight.specular * spec; // assuming bright white light color

    float dirShadow = shadowCalculation(fs_in.fragPosLightSpace, ndotl, shadowMap, dirLight);
    vec3 dirColor = ambient + dirShadow * (diffuse + specular);

    // Spot light
    float epsilon = spotLight.cutOff - spotLight.outerCutOff;
    lightDir = normalize(spotLight.position - fs_in.fragPos);
    float theta = dot(lightDir, normalize(-spotLight.direction));
    float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);
    float dist = length(spotLight.position - fs_in.fragPos);
    float attenuation = 1.0 / (spotLight.constant +
                               spotLight.linear * dist +
                               spotLight.quadratic * (dist * dist));
    ndotl = max(dot(lightDir, normal), 0.0);
    // diffuse
    diffuse = ndotl * color * spotLight.diffuse * attenuation * intensity;
    // specular
    halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    specular = spotLight.specular * spec * attenuation * intensity;

    float spotShadow = shadowCalculation(fs_in.fragPosSpotSpace, ndotl, spotShadowMap, spotLight);
    vec3 spotColor = spotShadow * (diffuse + specular);

    FragColor = vec4(dirColor + spotColor, 1.0);
}