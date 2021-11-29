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

    // temporary
    vec3 normal;
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

// temporary
uniform vec3 viewPos;

uniform Light dirLight;
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

#define PI 3.1415926538

vec3 calcLight(Light light, vec3 n, vec3 v, vec3 l, float shadow, vec3 F0,
               vec3 albedo, float metallic, float roughness, float ndotl, float ndotv);
float shadowCalculation(vec4 pos, float ndotl, sampler2D map, Light light);

// PBR functions.
float GGXNDF(vec3 n, vec3 m, float roughness);
float lambdaGGX(float a2);
float geometrySmith(float ndotv, float ndotl, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

out vec4 FragColor;


vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, fs_in.texCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(fs_in.worldFragPos);
    vec3 Q2  = dFdy(fs_in.worldFragPos);
    vec2 st1 = dFdx(fs_in.texCoords);
    vec2 st2 = dFdy(fs_in.texCoords);

    vec3 N   = normalize(fs_in.normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main() {
    // Load PBR values.
    vec3 albedo = pow(texture(albedoMap, fs_in.texCoords).rgb, vec3(2.2));
    //vec3 normal = texture(normalMap, fs_in.texCoords).rgb;
    // temporary
    vec3 normal = getNormalFromMap();
    float metallic = texture(metallicMap, fs_in.texCoords).r;
    float roughness = texture(roughnessMap, fs_in.texCoords).r;

    // Transform the normal to the [-1,1] range.
    normal = normalize(normal * 2.0 - 1.0);

    //vec3 v = normalize(fs_in.frenetFragPos - fs_in.frenetViewPos);
    vec3 v = normalize(fs_in.worldFragPos - viewPos);
    float ndotv = max(dot(normal,v), 0.0);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);

    // Shadow from directional light
    //vec3 l = -fs_in.frenetLightDir;
    vec3 l = normalize(-dirLight.direction);
    float ndotl = max(dot(l, normal), 0.0);
    float dirShadow = shadowCalculation(fs_in.fragPosDirSpace, ndotl, shadowMap, dirLight);
    Lo += calcLight(dirLight, normal, v, l, dirShadow, F0, albedo, metallic,
                    roughness, ndotl, ndotv);

    // Shadow from spot light
    //l = normalize(fs_in.frenetFragPos - fs_in.frenetSpotPos);
    l = normalize(fs_in.worldFragPos - spotLight.position);
    ndotl = max(dot(l, normal), 0.0);
    float spotShadow = shadowCalculation(fs_in.fragPosSpotSpace, ndotl, spotShadowMap, spotLight);
    Lo += calcLight(spotLight, normal, v, l, spotShadow, F0, albedo, metallic,
                    roughness, ndotl, ndotv);

    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;

    // HDR Tonemapping
    color = color / (color + vec3(1.0));
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0f);
}

vec3 calcLight(Light light, vec3 n, vec3 v, vec3 l, float shadow, vec3 F0,
               vec3 albedo, float metallic, float roughness, float ndotl, float ndotv) {
    // Calculate the color of light at the fragment.
    float intensity = 1.0;
    float attenuation = 1.0;
    if (!light.directional) {
        float theta = dot(l, normalize(-light.direction));
        float epsilon = light.cutOff - light.outerCutOff;
        intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);
        float d = length(light.position - fs_in.worldFragPos);
        attenuation = 1.0 / (d * d);
    }
    vec3 cLight = attenuation * intensity * light.diffuse;

    vec3 h = normalize(v + l);
    float NDF = GGXNDF(n, h, roughness);
    float G = geometrySmith(ndotv, ndotl, roughness);
    vec3 F = fresnelSchlick(max(dot(h,v), 0.0), F0);

    vec3 numerator = NDF * G * F;
    // Add 0.0001 to the denominator to avoid dividing by 0.
    float denominator = 4 * max(dot(n,v), 0.0) * max(dot(n,l), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kD = vec3(1.0) - F;
    // Multiply kD by the inverse of metalness so metals do not have
    // subsurface scattering.
    kD *= 1.0 - metallic;

    return (kD * albedo + specular * PI) * cLight * ndotl;
}

// Calculate GGX/Trowbridge-Reitz NDF.
float GGXNDF(vec3 n, vec3 m, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float ndotm = max(dot(n, m), 0.0);
    float ndotm2 = ndotm*ndotm;

    float nom   = a2;
    float denom = (ndotm2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

// Calculate GGX lambda function.
float lambdaGGX(float a2)
{
    float num = -1.0 + sqrt(1.0 + 1.0 / a2);
    float denom = 2.0;

    return num / denom;
}

// Use the Smith height-correlated masking-shadowing function.
float geometrySmith(float ndotv, float ndotl, float roughness)
{
    float r = (roughness * roughness);
    float nv2 = (ndotv * ndotv);
    float nl2 = (ndotl * ndotl);
    float a2v = (nv2) / (r * (1.0 - nv2));
    float a2l = (nl2) / (r * (1.0 - nl2));

    float lambdaV = lambdaGGX(a2v);
    float lambdaL = lambdaGGX(a2l);

    return 1.0 / (1.0 + lambdaV * lambdaL);
}

// Calculate Fresnel reflection using Schlick approximation.
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float estimateBlockerDepth(vec3 projCoords, Light light, sampler2D map, vec4 rotation) {
    // Calculate size of blocker search.
    float searchWidth = light.width * projCoords.z;

    // Calculate average blocker depth.
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