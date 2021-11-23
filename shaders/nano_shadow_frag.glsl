#version 430 core
struct Material {
    sampler2D diffuse0;
    sampler2D specular0;
    sampler2D emission0;

    vec3 simpleAmbient;
    vec3 simpleDiffuse;
    vec3 simpleSpecular;
    vec3 simpleEmissive;

    float shininess;
    bool isSimple;
};

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

in VS_OUT {
    vec3 norm;
    vec2 texCoords;
    vec3 viewObjPos;
    vec4 fragPosLightSpace;
    vec4 fragPosSpotSpace;
} fs_in;

layout (std140, binding = 0) uniform shadowBlock {
    vec2 poissonDisk[32];
    vec2 shadowTexelSize;
    int NUM_SEARCH_SAMPLES;
    int NUM_PCF_SAMPLES;
    float poissonSpread;
};

out vec4 FragColor;

uniform Material material;
uniform Light directionalLight;
uniform Light spotLight;
uniform sampler2D shadowMap;
uniform sampler2D spotShadowMap;

vec3 calcLight(Light light, vec3 normal, vec3 viewDir, float shadow);
vec3 calcSimpleLight(Light light, vec3 normal, vec3 viewDir, float shadow);
float shadowCalculation(vec4 pos, float ndotl, sampler2D map);

void main() {
    vec3 result = vec3(0.0);
    vec3 normal = normalize(fs_in.norm);
    vec3 viewDir = normalize(-fs_in.viewObjPos); // since we are in view coordinates, the camera is always at 0,0,0

    // Shadow from directional light
    vec3 l = -directionalLight.direction;
    float ndotl = max(dot(l, normal), 0.0);
    float dirShadow = shadowCalculation(fs_in.fragPosLightSpace, ndotl, shadowMap);

    // Shadow from spot light
    l = normalize(spotLight.position - fs_in.viewObjPos);
    ndotl = max(dot(l, normal), 0.0);
    float spotShadow = shadowCalculation(fs_in.fragPosSpotSpace, ndotl, spotShadowMap);

    if (material.isSimple == true) {
        result += calcSimpleLight(directionalLight, normal, viewDir, dirShadow);

        result += calcSimpleLight(spotLight, normal, viewDir, spotShadow);

        // Calculate emission lighting
        const float emissionStrength = 2.0;
        vec3 emission = emissionStrength * material.simpleEmissive;
        result += emission;
    }
    else {
        result += calcLight(directionalLight, normal, viewDir, dirShadow);

        result += calcLight(spotLight, normal, viewDir, spotShadow);
    }

    FragColor = vec4(result, 1.0f);
}

vec3 calcLight(Light light, vec3 normal, vec3 viewDir, float shadow) {
    vec3 lightVec;
    float theta;
    float intensity;
    float epsilon = light.cutOff - light.outerCutOff;
    float dist;
    float attenuation;

    if (light.directional == true) {
        lightVec = normalize(-light.direction);
        attenuation = 1.0;
    }
    else {
        lightVec = normalize(light.position - fs_in.viewObjPos);
        // Calculate attenuation
        dist = length(light.position - fs_in.viewObjPos);
        attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * (dist * dist));
    }

    // Calculate intensity based on the angle of the light with respect to the object
    if (light.cutOff >= 0) {
        theta = dot(lightVec, normalize(-light.direction));
        intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    }
    else {
        intensity = 1.0;
    }

    // Calculate ambient lighting
    vec3 ambient = light.ambient * texture(material.diffuse0, fs_in.texCoords).rgb;
    ambient *= attenuation * intensity;

    // Calculate diffuse lighting
    float diff = max(dot(normal, lightVec), 0.0);
    vec3 diffuse = light.diffuse * diff * texture(material.diffuse0, fs_in.texCoords).rgb;
    diffuse *= attenuation * intensity;

    // Calculate specular lighting
    vec3 halfwayDir = normalize(lightVec + viewDir);
    vec3 reflectDir = reflect(-lightVec, normal); // -lightVec because reflect expects it to point towards the surface
    float spec = 0.0;
    if (material.shininess > 0) {
        spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess * 4);
    }
    vec3 specular = light.specular * spec * texture(material.specular0, fs_in.texCoords).rgb;
    specular *= attenuation * intensity;

    return ambient + shadow * (diffuse + specular);
}

vec3 calcSimpleLight(Light light, vec3 normal, vec3 viewDir, float shadow) {

    vec3 lightVec;
    float theta;
    float intensity;
    float epsilon = light.cutOff - light.outerCutOff;
    float dist;
    float attenuation;

    if (light.directional == true) {
        lightVec = normalize(-light.direction);
        attenuation = 1.0;
    }
    else {
        lightVec = normalize(light.position - fs_in.viewObjPos);
        // Calculate attenuation
        dist = length(light.position - fs_in.viewObjPos);
        attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * (dist * dist));
    }

    // Calculate intensity based on the angle of the light with respect to the object
    if (light.cutOff >= 0) {
        theta = dot(lightVec, normalize(-light.direction));
        intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0f, 1.0f);
    }
    else {
        intensity = 1.0;
    }

    // Calculate ambient lighting
    vec3 ambient = light.ambient * material.simpleAmbient;
    ambient *= attenuation * intensity;

    // Calculate diffuse lighting
    float diff = max(dot(normal, lightVec), 0.0f);
    vec3 diffuse = light.diffuse * diff * material.simpleDiffuse;
    diffuse *= attenuation * intensity;

    // Calculate specular lighting
    vec3 halfwayDir = normalize(lightVec + viewDir);
    float spec = 0.0f;
    if (material.shininess > 0) {
        spec = pow(max(dot(normal, halfwayDir), 0.0f), material.shininess * 4);
    }
    vec3 specular = light.specular * spec * material.simpleSpecular;
    specular *= attenuation * intensity;

    return ambient + shadow * (diffuse + specular);
}

float shadowCalculation(vec4 pos, float ndotl, sampler2D map) {
    // perform perspective divide
    vec3 projCoords;
    if (pos.w != 0.0)
        projCoords = pos.xyz / pos.w;
    else
        projCoords = pos.xyz;

    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    float shadow = 0.0;
    // Check if fragment is beyond far plane of frustum
    if (projCoords.z <= 1.0) {
        // Use a bias to avoid shadow acne
        float bias = max(0.02 * (1.0 -  ndotl), 0.005);
        projCoords.z -= bias;

        for (int i = 0; i < 4; i++) {
            vec2 sampleCoords = vec2(projCoords.xy + poissonDisk[i] / poissonSpread);
            shadow += texture(map, sampleCoords).r < projCoords.z ? 0.0 : 1.0;
        }
        shadow *= 0.25;
    }
    else
        shadow = 1.0;

    return shadow;
}