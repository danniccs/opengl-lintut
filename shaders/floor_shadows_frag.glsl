#version 430 core
out vec4 FragColor;

in VS_OUT {
    vec3 worldFragPos;
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
    vec2 poissonDisk[32];
    vec2 shadowTexelSize;
    int NUM_SEARCH_SAMPLES;
    int NUM_PCF_SAMPLES;
    float poissonSpread;
};

uniform sampler2D floorTexture;
uniform sampler2D shadowMap;
uniform sampler2D spotShadowMap;
uniform sampler2D randomAngles;
uniform vec3 viewPos;
uniform Light spotLight;
uniform Light dirLight;

const int MAX_NUM_SAMPLES = 64;

float estimateBlockerDepth(vec3 projCoords, Light light, sampler2D map, vec2 rotation[MAX_NUM_SAMPLES]) {
    // Calculate size of blocker search.
    float searchWidth = light.width * projCoords.z;

    // Calculate average blocker depth.
    float blockerDepth = 0.0;
    int numBlockers = 0;

    for (int i = 0; i < NUM_SEARCH_SAMPLES; ++i) {
        vec2 offset = vec2(poissonDisk[i].x * rotation[i].x - poissonDisk[i].y * rotation[i].y,
                           poissonDisk[i].x * rotation[i].y + poissonDisk[i].y * rotation[i].x);
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

        // Get random angles to rotate poisson disk coordinates.
        vec2 rotation[MAX_NUM_SAMPLES];
        for (int i = 0; i < NUM_PCF_SAMPLES; ++i) {
            vec2 angleTexCoords = fract(i * fs_in.worldFragPos.xz);
            rotation[i] = texture(randomAngles, angleTexCoords).rg;
        }

        float bias = max(0.005 * (1.0 -  ndotl), 0.0005);
        projCoords.z -= bias;

        // Estimate average blocker depth
        float blockerDepth = estimateBlockerDepth(projCoords, light, map, rotation);

        // Use PCF to calculate shadow value
        if (blockerDepth > 0.0) {
            float wPenumbra = ((projCoords.z - blockerDepth) * light.width / blockerDepth) * 200.0;

            // For Poisson disk inner sampling
            vec2 innerOffset[4];
            for (int j = 0; j < 4; j++) {
                innerOffset[j] = vec2(poissonDisk[j].x * rotation[j].x - poissonDisk[j].y * rotation[j].y,
                                      poissonDisk[j].x * rotation[j].y + poissonDisk[j].y * rotation[j].x);
            }

            for (int i = 0; i < NUM_PCF_SAMPLES; ++i) {
                vec2 offset = vec2(poissonDisk[i].x * rotation[i].x - poissonDisk[i].y * rotation[i].y,
                                   poissonDisk[i].x * rotation[i].y + poissonDisk[i].y * rotation[i].x);
                vec2 sampleCenter = projCoords.xy + offset * shadowTexelSize * wPenumbra;

                // With interpolation
                /*
                vec2 sampleFloor = floor(sampleCenter / shadowTexelSize);
                float t1 = fract(sampleCenter.x / shadowTexelSize.x);
                float t2 = fract(sampleCenter.y / shadowTexelSize.y);

                float x1 = texture(map, sampleFloor * shadowTexelSize).r;
                float x2 = texture(map, (sampleFloor + vec2(1.0, 0.0)) * shadowTexelSize).r;
                float y1 = texture(map, (sampleFloor + vec2(0.0, 1.0)) * shadowTexelSize).r;
                float y2 = texture(map, (sampleFloor + vec2(1.0, 1.0)) * shadowTexelSize).r;

                x1 = x1 < projCoords.z ? 0.0 : 1.0;
                x2 = x2 < projCoords.z ? 0.0 : 1.0;
                y1 = y1 < projCoords.z ? 0.0 : 1.0;
                y2 = y2 < projCoords.z ? 0.0 : 1.0;

                shadow += mix(mix(x1, x2, t1), mix(y1, y2, t1), t2);
                */

                // With Poisson disk sampling (remember to divide final result by 4.0)
                for (int j = 0; j < 4; ++j) {
                    float depth = texture(map, sampleCenter + innerOffset[j] * shadowTexelSize).r;
                    shadow += depth < projCoords.z ? 0.0 : 1.0;
                }
            }
            shadow /= (NUM_PCF_SAMPLES * 4.0);
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
    vec3 viewDir = normalize(viewPos - fs_in.worldFragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = vec3(0.0);
    // Set specular value to 0 with dirLight to observe shadows more easily.
    //vec3 specular = dirLight.specular * spec; // assuming bright white light color

    float dirShadow = shadowCalculation(fs_in.fragPosLightSpace, ndotl, shadowMap, dirLight);
    vec3 dirColor = ambient + dirShadow * (diffuse + specular);

    // Spot light
    float epsilon = spotLight.cutOff - spotLight.outerCutOff;
    lightDir = normalize(spotLight.position - fs_in.worldFragPos);
    float theta = dot(lightDir, normalize(-spotLight.direction));
    float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);
    float dist = length(spotLight.position - fs_in.worldFragPos);
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