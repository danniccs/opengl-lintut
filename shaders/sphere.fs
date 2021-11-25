#version 430 core

in VS_OUT {
    vec3 worldFragPos;
    vec2 texCoords;

    vec3 frenetFragPos;
    vec3 frenetViewPos;
    vec3 frenetLightDir;
    vec3 frenetSpotPos;

    vec4 fragPosDirSpace;
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

uniform Light directionalLight;
uniform Light spotLight;
uniform sampler2D shadowMap;
uniform sampler2D spotShadowMap;
uniform sampler2D randomAngles;

// PBR textures.
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform float ao;

vec3 calcLight(Light light, vec3 normal, vec3 viewDir, float shadow);
vec3 calcSimpleLight(Light light, vec3 normal, vec3 viewDir, float shadow);
float shadowCalculation(vec4 pos, float ndotl, sampler2D map, Light light);

out vec4 FragColor;


void main() {
    // Load PBR values.
    vec3 albedo = pow(texture(albedoMap, fs_in.texCoords).rgb, vec3(2.2));
    vec3 normal = texture(normalMap, fs_in.texCoords).rgb;
    float metallic = texture(metallicMap, fs_in.texCoords).r;
    float roughness = texture(roughnessMap, fs_in.texCoords).r;

    // Transform the normal to the [-1,1] range.
    normal = normalize(normal * 2.0 - 1.0);

    vec3 result = vec3(0.0);
    vec3 viewDir = normalize(fs_in.frenetFragPos - fs_in.frenetViewPos);

    // Shadow from directional light
    vec3 l = -fs_in.frenetLightDir;
    float ndotl = max(dot(l, normal), 0.0);
    float dirShadow = shadowCalculation(fs_in.fragPosDirSpace, ndotl, shadowMap, directionalLight);

    // Shadow from spot light
    l = normalize(fs_in.frenetFragPos - fs_in.frenetSpotPos);
    ndotl = max(dot(l, normal), 0.0);
    float spotShadow = shadowCalculation(fs_in.fragPosSpotSpace, ndotl, spotShadowMap, spotLight);

    FragColor = vec4(result, 1.0f);
}

vec3 calcLight(Light light, vec3 normal, vec3 viewDir, float shadow, vec3 albedo) {
    vec3 lightVec;
    float theta;
    float intensity;
    float epsilon = light.cutOff - light.outerCutOff;
    float dist;
    float attenuation;

    // Calculate ambient lighting
    vec3 ambient = vec3(0.03) * albedo * ao;
    return ambient;
}

float estimateBlockerDepth(vec3 projCoords, Light light, sampler2D map, vec4 rotation) {
    // Calculate size of blocker search
    float searchWidth = light.width * projCoords.z;

    // Calculate average blocker depth
    float blockerDepth = 0.0;
    int numBlockers = 0;

    for (int i = 0; i < NUM_SEARCH_SAMPLES; ++i) {
        vec2 offset = vec2(poissonDisk[i].x * rotation.x - poissonDisk[i].y * rotation.y,
                           poissonDisk[i].x * rotation.y + poissonDisk[i].y * rotation.x);
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
        ivec2 angleTexCoords = ivec2(fs_in.worldFragPos.x, fs_in.worldFragPos.y);
        vec4 rotation = texture(randomAngles, angleTexCoords, 0);

        float bias = max(0.02 * (1.0 -  ndotl), 0.005);
        projCoords.z -= bias;

        // Estimate average blocker depth
        float blockerDepth = estimateBlockerDepth(projCoords, light, map, rotation);

        /*
        // For Poisson disk inner sampling
        vec2 innerOffset[4];
        for (int j = 0; j < 4; j++) {
            innerOffset[j] = vec2(poissonDisk[j].x * rotation.x - poissonDisk[j].y * rotation.y,
                                  poissonDisk[j].x * rotation.y + poissonDisk[j].y * rotation.x);
        }
        */

        // Use PCF to calculate shadow value
        if (blockerDepth > 0.0) {
            float wPenumbra = ((projCoords.z - blockerDepth) * light.width / blockerDepth) * 200.0;

            for (int i = 0; i < NUM_PCF_SAMPLES; ++i) {
                vec2 offset = vec2(poissonDisk[i].x * rotation.x - poissonDisk[i].y * rotation.y,
                                   poissonDisk[i].x * rotation.y + poissonDisk[i].y * rotation.x);
                vec2 sampleCenter = projCoords.xy + offset * shadowTexelSize * wPenumbra;

                // With interpolation
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

                /*
                // With Poisson disk sampling (remember to divide final result by 4.0)
                for (int j = 0; j < 4; ++j) {
                    float depth = texture(map, sampleCenter + innerOffset[j] * shadowTexelSize).r;
                    shadow += depth < projCoords.z ? 0.0 : 1.0;
                }
                */
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