#version 430 core

in VS_OUT {
  vec3 worldFragPos;
  vec4 viewFragPos;
  vec2 texCoords;

  vec3 frenetFragPos;
  vec3 frenetViewPos;
  vec3 frenetLightDir;
  vec3 frenetSpotPos;
  vec3 frenetSpotDir;
  vec3 frenetTubePos;
  vec3 frenetP0;
  vec3 frenetP1;
}
fs_in;

struct Light {
  vec3 position;
  vec3 direction;
  float cutOff;       // max angle at which it gives full light
  float outerCutOff;  // max angle at which it gives any light
  bool directional;
  float width;
  float len;

  vec3 cLight;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;

  float constant;
  float linear;
  float quadratic;
};

layout(std140, binding = 0) uniform shadowBlock {
  vec2 poissonDisk[32];
  float cascadePlaneDistances[5];
  vec2 shadowTexelSize;
  int NUM_SEARCH_SAMPLES;
  int NUM_PCF_SAMPLES;
  float shadowMult;
  int NUM_CSM_FRUSTA;
};

layout(std140, binding = 1) uniform CSMBlock { mat4 dirLightSpaceMatrices[5]; };
uniform mat4 spotSpaceMat;
uniform mat4 tubeSpaceMat;

uniform Light dirLight;
uniform Light spotLight;
uniform Light tubeLight;
uniform sampler2DArray dirCSM;
uniform sampler2D spotShadowMap;
uniform sampler2D tubeShadowMap;
uniform sampler2D randomAngles;

// PBR textures.
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;

// For Parallax Occlusion Mapping
uniform float heightScale;
uniform sampler2D heightMap;

// Choose whether to draw area lights.
uniform bool areaLights;
uniform bool showTube;

const float PI = 3.1415926538;
const int MAX_NUM_SAMPLES = 64;

out vec4 FragColor;

// Calculate GGX/Trowbridge-Reitz NDF.
float GGXNDF(vec3 n, vec3 m, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float ndotm = max(dot(n, m), 0.0);
  float ndotm2 = ndotm * ndotm;

  float num = a2;
  float denom = (ndotm2 * (a2 - 1.0) + 1.0);
  denom = PI * denom * denom;

  return num / denom;
}

// Calculate GGX lambda function.
float lambdaGGX(float a2) {
  float num = -1.0 + sqrt(1.0 + 1.0 / a2);
  float denom = 2.0;

  return num / denom;
}

// Use the Smith height-correlated masking-shadowing function.
float geometrySmith(float ndotv, float ndotl, float roughness) {
  float r = (roughness * roughness);
  float nv2 = (ndotv * ndotv);
  float nl2 = (ndotl * ndotl);
  float a2v = (nv2) / (r * (1.0 - nv2));
  float a2l = (nl2) / (r * (1.0 - nl2));

  float lambdaV = lambdaGGX(a2v);
  float lambdaL = lambdaGGX(a2l);

  return 1.0 / (1.0 + lambdaV * lambdaL);
}

// Calculate Fresnel reflection using the Schlick approximation.
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Alternative Geometry functions
float geometrySchlickGGX(float NdotV, float roughness) {
  float r = (roughness + 1.0);
  float k = (r * r) / 8.0;

  float nom = NdotV;
  float denom = NdotV * (1.0 - k) + k;

  return nom / denom;
}

float geometrySmithAlt(vec3 N, vec3 V, vec3 L, float roughness) {
  float NdotV = max(dot(N, V), 0.0);
  float NdotL = max(dot(N, L), 0.0);
  float ggx2 = geometrySchlickGGX(NdotV, roughness);
  float ggx1 = geometrySchlickGGX(NdotL, roughness);

  return ggx1 * ggx2;
}

// Calculate the reflectance equation for point and directional lights.
vec3 calcLight(Light light, vec3 frenetDir, vec3 n, vec3 v, vec3 l, vec3 F0,
               vec3 albedo, float metallic, float roughness, float ndotl,
               float ndotv) {
  // Calculate the color of light at the fragment.
  float intensity = 1.0;
  float attenuation = 1.0;
  if (light.directional == false) {
    float theta = dot(l, normalize(-frenetDir));
    // Add 0.0001 to prevent division by 0.
    float epsilon = light.cutOff - light.outerCutOff + 0.0001;
    intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    float d = length(light.position - fs_in.worldFragPos);
    float radius = light.width / 2.0;
    attenuation = (radius * radius) / (d * d);
  }
  vec3 cLight = light.cLight * attenuation * intensity;

  vec3 h = normalize(v + l);
  float D = GGXNDF(n, h, roughness);
  // float G = geometrySmithAlt(n, v, l, roughness);
  vec3 F = fresnelSchlick(max(dot(h, v), 0.0), F0);

  /*
    vec3 numerator = D * G * F;
    // Add 0.0001 to the denominator to avoid dividing by 0.
    float denominator = 4 * ndotv * ndotl + 0.0001;
    vec3 specular = numerator / denominator;
    */

  // Approximate G2 * denominator using Hammon's method.
  float absNL = abs(ndotl);
  float absNV = abs(ndotv);
  float G2Denom =
      0.5 / mix(2.0 * absNL * absNV, absNL + absNV, roughness * roughness);

  vec3 specular = D * G2Denom * F;

  // Using kD ensures energy conservation.
  vec3 kD = vec3(1.0) - F;
  /*
  Multiply kD by the inverse of metalness so metals do not have
  subsurface scattering.
  */
  kD *= 1.0 - metallic;

  return (kD * albedo + specular * PI) * cLight * ndotl;
}

/*
Calculate GGX/Trowbridge-Reitz NDF modified to account for
the change in energy caused by using a representative point
solution for a sphere light.
*/
float sphereGGXNDF(vec3 n, vec3 m, float roughness, float radius, float d) {
  float alpha = roughness * roughness;
  float alpha2 = alpha * alpha;
  float alphaP = min(alpha + radius / (2.0 * d), 1.0);
  float alphaP2 = alphaP * alphaP;

  float ndotm = max(dot(n, m), 0.0);
  float ndotm2 = ndotm * ndotm;

  float num = alpha2 * alpha2;
  float denom = (ndotm2 * (alpha2 - 1.0) + 1.0);
  denom = denom * denom * PI * alphaP2;

  return num / denom;
}

vec3 calcSphereLambert(Light light, vec3 frenetLightDir, vec3 v, vec3 l,
                       vec3 F0, vec3 albedo, float metallic, float ndotl) {
  float radius = light.width / 2.0;
  float d = length(light.position - fs_in.worldFragPos);
  // Calculate the color of light at the fragment.
  float theta = dot(l, normalize(-frenetLightDir));
  // Add 0.0001 to prevent division by 0.
  float epsilon = light.cutOff - light.outerCutOff + 0.0001;
  float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
  float attenuation = (radius * radius) / (d * d);
  vec3 cLight = light.cLight * attenuation * intensity;

  vec3 h = normalize(v + l);
  vec3 F = fresnelSchlick(max(dot(h, v), 0.0), F0);
  // Using kD ensures energy conservation.
  vec3 kD = vec3(1.0) - F;
  // Multiply kD by the inverse of metalness so metals do not have
  // subsurface scattering.
  kD *= 1.0 - metallic;

  return kD * albedo * cLight * ndotl;
}

// Use Karis' most representative point solution for spherical lights.
vec3 calcSphereGlossy(Light light, vec3 frenetLightDir, vec3 frenetLightPos,
                      vec3 n, vec3 v, vec3 F0, float roughness, vec3 albedo,
                      float metallic, float ndotv) {
  float radius = light.width / 2.0;

  // Calculate a new light vector.
  vec3 viewReflect = reflect(-v, n);
  vec3 L = frenetLightPos - fs_in.frenetFragPos;
  vec3 pcr = dot(L, viewReflect) * viewReflect - L;
  vec3 pcs = L + pcr * min(radius / length(pcr), 1.0);
  vec3 l = normalize(pcs);
  float d = length(pcs);
  float ndotl = max(dot(n, l), 0.0);

  // Calculate the color of light at the fragment.
  float theta = dot(l, normalize(-frenetLightDir));
  // Add 0.0001 to prevent division by 0.
  float epsilon = light.cutOff - light.outerCutOff + 0.0001;
  float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
  float attenuation = (radius * radius) / (d * d);
  vec3 cLight = light.cLight * attenuation * intensity;

  vec3 h = normalize(v + l);
  float D = sphereGGXNDF(n, h, roughness, radius, d);
  // float G = geometrySmithAlt(n, v, l, roughness);
  vec3 F = fresnelSchlick(max(dot(h, v), 0.0), F0);

  /*
  vec3 numerator = D * G * F;
  // Add 0.0001 to the denominator to avoid dividing by 0.
  float denominator = 4 * ndotv * ndotl + 0.0001;
  vec3 specular = numerator / denominator;
  */

  // Approximate G2 * denominator using Hammon's method.
  float absNL = abs(ndotl);
  float absNV = abs(ndotv);
  float G2Denom =
      0.5 / mix(2.0 * absNL * absNV, absNL + absNV, roughness * roughness);

  vec3 specular = D * G2Denom * F;

  // Using kD ensures energy conservation.
  vec3 kD = vec3(1.0) - F;
  // Multiply kD by the inverse of metalness so metals do not have
  // subsurface scattering.
  kD *= 1.0 - metallic;

  return (kD * albedo + specular * PI) * cLight * ndotl;
}

/*
Calculate GGX/Trowbridge-Reitz NDF modified to account for
the change in energy caused by using a representative point
solution for a tube light.
*/
float tubeGGXNDF(vec3 n, vec3 m, float roughness, float halfLen, float radius,
                 float dL, float dS) {
  float alpha = roughness * roughness;
  float alpha2 = alpha * alpha;
  float alphaT = min(alpha + halfLen / (2.0 * dL), 1.0);
  float alphaS = min(alpha + radius / (2.0 * dS), 1.0);
  float alphaS2 = alphaS * alphaS;

  float ndotm = max(dot(n, m), 0.0);
  float ndotm2 = ndotm * ndotm;

  float num = alpha2 * alpha2 * alpha;
  float denom = (ndotm2 * (alpha2 - 1.0) + 1.0);
  denom = denom * denom * PI * alphaS2 * alphaT;

  return num / denom;
}

vec3 calcTubeLambert(Light light, vec3 frenetP0, vec3 frenetP1, vec3 n, vec3 v,
                     vec3 l, vec3 F0, vec3 albedo, float metallic) {
  vec3 h = normalize(v + l);
  vec3 F = fresnelSchlick(max(dot(h, v), 0.0), F0);
  // Using kD ensures energy conservation.
  vec3 kD = vec3(1.0) - F;
  // Multiply kD by the inverse of metalness so metals do not have
  // subsurface scattering.
  kD *= 1.0 - metallic;

  vec3 L0 = frenetP0 - fs_in.frenetFragPos;
  vec3 L1 = frenetP1 - fs_in.frenetFragPos;
  float d0 = length(L0);
  float d1 = length(L1);

  float num =
      2.0 * clamp(dot(n, L0) / (2.0 * d0) + dot(n, L1) / (2.0 * d1), 0.0, 1.0);
  float denom = d0 * d1 + dot(L0, L1) + 2.0;

  return kD * (albedo / PI) * (num / denom) * light.cLight;
}

// Use Karis' most representative point solution for tube lights.
vec3 calcTubeGlossy(Light light, vec3 frenetP0, vec3 frenetP1, vec3 n, vec3 v,
                    vec3 F0, float roughness, vec3 albedo, float metallic,
                    float ndotv) {
  float radius = light.width / 2.0;
  vec3 r = reflect(-v, n);

  // Calculate a new light position for a line light.
  vec3 L0 = frenetP0 - fs_in.frenetFragPos;
  vec3 L1 = frenetP1 - fs_in.frenetFragPos;
  vec3 Ld = L1 - L0;
  float tNum = dot(r, L0) * dot(r, Ld) - dot(L0, Ld);
  float lenLd = length(Ld);
  float tDen = lenLd * lenLd - dot(r, Ld) * dot(r, Ld);
  float t = clamp(tNum / tDen, 0.0, 1.0);
  vec3 L = L0 + t * Ld;
  float dL = length(L);

  // Modify the new light vector using the sphere light modification.
  vec3 pcr = dot(L, r) * r - L;
  vec3 pcs = L + pcr * min(radius / length(pcr), 1.0);

  vec3 l = normalize(pcs);
  float d = length(pcs);
  float ndotl = max(dot(n, l), 0.0);

  // Calculate the color of light at the fragment.
  float attenuation = 1.0 / (length(L0) * length(L1) + dot(L0, L1));
  vec3 cLight = light.cLight * attenuation;

  vec3 h = normalize(v + l);
  float D = tubeGGXNDF(n, h, roughness, light.len / 2.0, radius, dL, d);
  // float G = geometrySmithAlt(n, v, l, roughness);
  vec3 F = fresnelSchlick(max(dot(h, v), 0.0), F0);

  /*
  vec3 numerator = D * G * F;
  Add 0.0001 to the denominator to avoid dividing by 0.
  float denominator = 4 * ndotv * ndotl + 0.0001;
  vec3 specular = numerator / denominator;
  */

  // Approximate G2 * denominator using Hammon's method.
  float absNL = abs(ndotl);
  float absNV = abs(ndotv);
  float G2Denom =
      0.5 / mix(2.0 * absNL * absNV, absNL + absNV, roughness * roughness);

  vec3 specular = D * G2Denom * F;

  // Using kD ensures energy conservation.
  vec3 kD = vec3(1.0) - F;
  // Multiply kD by the inverse of metalness so metals do not have
  // subsurface scattering.
  kD *= 1.0 - metallic;

  return (kD * albedo + specular * PI) * cLight * ndotl;
}

float estimateBlockerDepth(vec3 projCoords, Light light, sampler2D map,
                           vec2 rotation[MAX_NUM_SAMPLES]) {
  // Calculate size of blocker search
  float searchWidth = light.width * projCoords.z;

  // Calculate average blocker depth
  float blockerDepth = 0.0;
  int numBlockers = 0;

  for (int i = 0; i < NUM_SEARCH_SAMPLES; ++i) {
    vec2 offset = vec2(
        poissonDisk[i].x * rotation[i].x - poissonDisk[i].y * rotation[i].y,
        poissonDisk[i].x * rotation[i].y + poissonDisk[i].y * rotation[i].x);
    float depth = texture(map, projCoords.xy + offset * shadowTexelSize *
                                                   searchWidth * shadowMult)
                      .r;
    if (depth < projCoords.z) {
      blockerDepth += depth;
      ++numBlockers;
    }
  }
  return blockerDepth /= numBlockers;
}

float estimateBlockerDepthCSM(vec3 projCoords, Light light, sampler2DArray CSM,
                              int layer, vec2 rotation[MAX_NUM_SAMPLES]) {
  // Calculate size of blocker search
  float searchWidth = max(light.width, light.len) * projCoords.z;

  // Calculate average blocker depth
  float blockerDepth = 0.0;
  int numBlockers = 0;

  for (int i = 0; i < NUM_SEARCH_SAMPLES; ++i) {
    vec2 offset = vec2(
        poissonDisk[i].x * rotation[i].x - poissonDisk[i].y * rotation[i].y,
        poissonDisk[i].x * rotation[i].y + poissonDisk[i].y * rotation[i].x);
    vec3 CSMCoord = vec3(
        projCoords.xy + offset * shadowTexelSize * searchWidth * shadowMult,
        layer);
    float depth = texture(CSM, CSMCoord).r;
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
    vec2 rotation[MAX_NUM_SAMPLES];
    for (int i = 0; i < NUM_PCF_SAMPLES; ++i) {
      vec2 angleTexCoords = fract(fs_in.worldFragPos.xz * i);
      rotation[i] = texture(randomAngles, angleTexCoords).rg;
    }

    float bias = max(0.01 * (1.0 - ndotl), 0.005);
    projCoords.z -= bias;

    // Estimate average blocker depth
    float blockerDepth = estimateBlockerDepth(projCoords, light, map, rotation);

    // Use PCF to calculate shadow value
    if (blockerDepth > 0.0) {
      float wPenumbra = ((projCoords.z - blockerDepth) *
                         max(light.width, light.len) / blockerDepth) *
                        200.0;

      // For Poisson disk inner sampling
      vec2 innerOffset[4];
      for (int j = 0; j < 4; j++) {
        innerOffset[j] = vec2(
            poissonDisk[j].x * rotation[j].x - poissonDisk[j].y * rotation[j].y,
            poissonDisk[j].x * rotation[j].y + poissonDisk[j].y * rotation[j].x);
      }

      for (int i = 0; i < NUM_PCF_SAMPLES; ++i) {
        vec2 offset = vec2(
            poissonDisk[i].x * rotation[i].x - poissonDisk[i].y * rotation[i].y,
            poissonDisk[i].x * rotation[i].y + poissonDisk[i].y * rotation[i].x);
        vec2 sampleCenter =
            projCoords.xy + offset * shadowTexelSize * wPenumbra * shadowMult;

        // With Poisson disk sampling (remember to divide final result by 4.0)
        for (int j = 0; j < 4; ++j) {
          float depth =
              texture(map, sampleCenter + innerOffset[j] * shadowTexelSize).r;
          shadow += depth < projCoords.z ? 0.0 : 1.0;
        }
      }
      shadow /= (NUM_PCF_SAMPLES * 4.0);
    } else
      shadow = 1.0;
  } else
    shadow = 1.0;

  return shadow;
}

float CSMCalculation(vec4 pos, float ndotl, sampler2DArray CSM, int layer,
                     Light light) {
  // perform perspective divide
  vec3 projCoords = pos.xyz / pos.w;
  // transform to [0,1] range
  projCoords = projCoords * 0.5 + 0.5;

  float shadow = 0.0;
  // Check if fragment is beyond far plane of frustum
  if (projCoords.z <= 1.0) {
    // Get random coordinates to rotate poisson disk
    vec2 rotation[MAX_NUM_SAMPLES];
    for (int i = 0; i < NUM_PCF_SAMPLES; ++i) {
      vec2 angleTexCoords = fract(fs_in.worldFragPos.xz * i);
      rotation[i] = texture(randomAngles, angleTexCoords).rg;
    }

    float bias = max(0.05 * (1.0 - ndotl), 0.005);
    bias /= (cascadePlaneDistances[layer] * 0.5);
    projCoords.z -= bias;

    // Estimate average blocker depth
    float blockerDepth =
        estimateBlockerDepthCSM(projCoords, light, CSM, layer, rotation);

    // Use PCF to calculate shadow value
    if (blockerDepth > 0.0) {
      float wPenumbra =
          ((projCoords.z - blockerDepth) * light.width / blockerDepth) * 200.0;

      // For Poisson disk inner sampling
      vec2 innerOffset[4];
      for (int j = 0; j < 4; j++) {
        innerOffset[j] = vec2(
            poissonDisk[j].x * rotation[j].x - poissonDisk[j].y * rotation[j].y,
            poissonDisk[j].x * rotation[j].y + poissonDisk[j].y * rotation[j].x);
      }

      for (int i = 0; i < NUM_PCF_SAMPLES; ++i) {
        vec2 offset = vec2(
            poissonDisk[i].x * rotation[i].x - poissonDisk[i].y * rotation[i].y,
            poissonDisk[i].x * rotation[i].y + poissonDisk[i].y * rotation[i].x);
        vec2 sampleCenter =
            projCoords.xy + offset * shadowTexelSize * wPenumbra * shadowMult;

        // With Poisson disk sampling (remember to divide final result by 4.0)
        for (int j = 0; j < 4; ++j) {
          vec3 CSMCoord = vec3(sampleCenter + innerOffset[j] * shadowTexelSize,
                               layer);
          float depth = texture(CSM, CSMCoord).r;
          shadow += depth < projCoords.z ? 0.0 : 1.0;
        }
      }
      shadow /= (NUM_PCF_SAMPLES * 4.0);
    } else
      shadow = 1.0;
  } else
    shadow = 1.0;

  return shadow;
}

vec2 parallaxOcclusion(vec2 texCoords, vec3 v) {
  const float minLayers = 8.0;
  const float maxLayers = 32.0;
  const float numLayers =
      mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), v), 0.0));
  const float layerHeight = 1.0 / numLayers;
  float currentLayerHeight = numLayers;

  vec2 P = v.xy * heightScale;
  vec2 deltaUV = P / numLayers;

  vec2 currentUV = texCoords;
  float currentHeight = texture(heightMap, currentUV).r;

  while (currentLayerHeight > currentHeight) {
    currentUV += deltaUV;
    currentHeight = texture(heightMap, currentUV).r;
    currentLayerHeight -= layerHeight;
  }

  vec2 prevUV = currentUV - deltaUV;

  // Get height differences before and after getting final UV.
  float afterHeight = currentHeight - currentLayerHeight;
  float beforeHeight =
      texture(heightMap, prevUV).r - (currentHeight + layerHeight);

  // Interpolate the texture coordinates.
  float t = afterHeight / (afterHeight - beforeHeight);
  vec2 finalUV = mix(prevUV, currentUV, t);

  return finalUV;
}

void main() {
  vec3 v = normalize(fs_in.frenetViewPos - fs_in.frenetFragPos);
  vec2 texCoords = parallaxOcclusion(fs_in.texCoords, v);

  const float gamma = 2.2;

  // Load PBR values.
  vec3 albedo = pow(texture(albedoMap, fs_in.texCoords).rgb, vec3(gamma));
  vec3 normal = texture(normalMap, fs_in.texCoords).rgb;
  float metallic = texture(metallicMap, fs_in.texCoords).r;
  float roughness = texture(roughnessMap, fs_in.texCoords).r;
  float ao = pow(texture(aoMap, fs_in.texCoords).r, gamma);

  // Transform the normal to the [-1,1] range.
  normal = normalize(normal * 2.0 - 1.0);

  float ndotv = max(dot(normal, v), 0.0);

  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo, metallic);

  // Select cascade layer.
  float depthVal = abs(fs_in.viewFragPos.z);
  int layer = 0;
  for (int i = 0; i < NUM_CSM_FRUSTA; ++i) {
    if (depthVal < cascadePlaneDistances[i]) {
      layer = i;
      break;
    }
  }

  vec4 fragPosDirSpace =
      dirLightSpaceMatrices[layer] * vec4(fs_in.worldFragPos, 1.0);
  vec4 fragPosSpotSpace = spotSpaceMat * vec4(fs_in.worldFragPos, 1.0);
  vec4 fragPosTubeSpace = tubeSpaceMat * vec4(fs_in.worldFragPos, 1.0);

  vec3 Lo = vec3(0.0);

  // Lo from directional light.
  vec3 l = normalize(-fs_in.frenetLightDir);
  float ndotl = max(dot(l, normal), 0.0);
  float shadow =
      CSMCalculation(fragPosDirSpace, ndotl, dirCSM, layer, dirLight);
  Lo += shadow * calcLight(dirLight, fs_in.frenetLightDir, normal, v, l, F0,
                           albedo, metallic, roughness, ndotl, ndotv);

  // Lo from spot light.
  l = normalize(fs_in.frenetSpotPos - fs_in.frenetFragPos);
  ndotl = max(dot(l, normal), 0.0);
  shadow = shadowCalculation(fragPosSpotSpace, ndotl, spotShadowMap,
                             spotLight);
  if (showTube == false) {
    if (areaLights == true) {
      vec3 LoSphere =
          calcSphereGlossy(spotLight, fs_in.frenetSpotDir, fs_in.frenetSpotPos,
                           normal, v, F0, roughness, albedo, metallic, ndotv);
      LoSphere *= shadow;
      Lo += LoSphere;
    } else {
      Lo += shadow * calcLight(spotLight, fs_in.frenetSpotDir, normal, v, l, F0,
                               albedo, metallic, roughness, ndotl, ndotv);
    }
  }

  // Lo from tube light.
  l = normalize(fs_in.frenetTubePos - fs_in.frenetFragPos);
  ndotl = max(dot(l, normal), 0.0);
  shadow = shadowCalculation(fragPosTubeSpace, ndotl, tubeShadowMap,
                             tubeLight);
  if (showTube == true) {
    vec3 LoTube =
        calcTubeGlossy(tubeLight, fs_in.frenetP0, fs_in.frenetP1, normal, v, F0,
                       roughness, albedo, metallic, ndotv);
    LoTube *= shadow;
    Lo += LoTube;
  }

  vec3 ambient = vec3(0.04) * albedo * ao;
  vec3 color = ambient + Lo;

  // HDR Tonemapping
  color = color / (color + vec3(1.0));
  // Gamma correction
  color = pow(color, vec3(1.0 / gamma));

  FragColor = vec4(color, 1.0);
}