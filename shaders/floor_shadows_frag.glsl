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

uniform sampler2D floorTexture;
uniform sampler2D randomAngles;
uniform sampler2D shadowMap;
uniform sampler2D spotShadowMap;
uniform vec3 viewPos;
uniform Light spotLight;
uniform Light dirLight;
uniform vec2 shadowTexelSize;

vec2 poissonDisk[4] = vec2[](
  vec2( -0.94201624, -0.39906216 ),
  vec2( 0.94558609, -0.76890725 ),
  vec2( -0.094184101, -0.92938870 ),
  vec2( 0.34495938, 0.29387760 )
);
float poissonSpread = 700.0;

float shadowCalculation(vec4 pos, float ndotl, sampler2D map, Light light) {
    // perform perspective divide
    vec3 projCoords = pos.xyz / pos.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // Get random coordinates to rotate poisson disk
    // ivec2 angleTexCoords = ivec2(fs_in.fragPos.x, fs_in.fragPos.y);
    // vec4 rotation = texture(randomAngles, angleTexCoords, 0);

    float shadow = 0.0;
    // Check if fragment is beyond far plane of frustum
    if (projCoords.z <= 1.0) {
        float bias = max(0.02 * (1.0 -  ndotl), 0.005);
        projCoords.z -= bias;

        // Calculate size of blocker search
        int searchWidth = 8;//int(light.width * projCoords.z);
        // Calculate average blocker depth
        float blockerDepth = 0.0;
        int numBlockers = 0;
        int searchOffset = searchWidth / 2;
        for (int i = -searchOffset; i <= searchOffset; ++i) {
            for (int j = -searchOffset; j <= searchOffset; ++j) {
                float depth = texture(map, projCoords.xy + vec2(i, j) * shadowTexelSize).r;
                if (depth < projCoords.z) {
                    blockerDepth += depth;
                    ++numBlockers;
                }
            }
        }
        blockerDepth /= numBlockers;

        // Use PCF to calculate shadow value
        if (numBlockers > 0) {
            int wPenumbra = int((projCoords.z - blockerDepth) * light.width / blockerDepth * 100);
            int pcfOffset = wPenumbra / 2;
            for (int i = -pcfOffset; i <= pcfOffset; ++i) {
                for (int j = -pcfOffset; j <= pcfOffset; ++j) {
                    float depth = texture(map, projCoords.xy + vec2(i, j) * shadowTexelSize).r;
                    shadow += depth < projCoords.z ? 0.0 : 1.0;
                }
            }
            shadow *= 1 / float(pow(wPenumbra, 2));
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