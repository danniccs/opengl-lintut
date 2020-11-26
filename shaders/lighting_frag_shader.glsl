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

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

const int NR_POINT_LIGHTS = 4;

in vec3 norm;
in vec2 texCoords;
in vec3 viewObjPos;

out vec4 FragColor;

uniform Material material;
uniform Light directionalLight;
uniform Light pointLights[NR_POINT_LIGHTS];
uniform Light flashLight;
uniform Light spotLight;

vec3 calcLight(Light light, vec3 normal, vec3 viewDir);
vec3 calcSimpleLight(Light light, vec3 normal, vec3 viewDir);

void main() {
    vec3 result = vec3(0.0f);
    vec3 normal = normalize(norm);
    vec3 viewDir = normalize(vec3(0.0f) - viewObjPos); // since we are in view coordinates, the camera is always at 0,0,0

    if (material.isSimple == true) {
        result += calcSimpleLight(directionalLight, normal, viewDir);

        for (int i = 0; i < NR_POINT_LIGHTS; i++)
            result += calcSimpleLight(pointLights[i], normal, viewDir);

        result += calcSimpleLight(flashLight, normal, viewDir);

        result += calcSimpleLight(spotLight, normal, viewDir);

        // Calculate emission lighting
        const float emissionStrength = 2.0f;
        vec3 emission = emissionStrength * material.simpleEmissive;
        result += emission;
    }
    else {
        result += calcLight(directionalLight, normal, viewDir);

        for (int i = 0; i < NR_POINT_LIGHTS; i++)
            result += calcLight(pointLights[i], normal, viewDir);

        result += calcLight(flashLight, normal, viewDir);

        result += calcLight(spotLight, normal, viewDir);
    }

    FragColor = vec4(result, 1.0f);
}

vec3 calcLight(Light light, vec3 normal, vec3 viewDir) {
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
        lightVec = normalize(light.position - viewObjPos);
        // Calculate attenuation
        dist = length(light.position - viewObjPos);
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
    vec3 ambient = light.ambient * texture(material.diffuse0, texCoords).rgb;
    ambient *= attenuation * intensity;

    // Calculate diffuse lighting
    float diff = max(dot(normal, lightVec), 0.0f);
    vec3 diffuse = light.diffuse * diff * texture(material.diffuse0, texCoords).rgb;
    diffuse *= attenuation * intensity;

    // Calculate specular lighting
    vec3 reflectDir = reflect(-lightVec, normal); // -lightVec because reflect expects it to point towards the surface
    float spec = 0.0f;
    if (material.shininess > 0) {
        spec = pow(max(dot(viewDir, reflectDir), 0.0f), material.shininess);
    }
    vec3 specular = light.specular * spec * texture(material.specular0, texCoords).rgb;
    specular *= attenuation * intensity;

    return ambient + diffuse + specular;
}

vec3 calcSimpleLight(Light light, vec3 normal, vec3 viewDir) {

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
        lightVec = normalize(light.position - viewObjPos);
        // Calculate attenuation
        dist = length(light.position - viewObjPos);
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
    vec3 reflectDir = reflect(-lightVec, normal); // -lightVec because reflect expects it to point towards the surface
    float spec = 0.0f;
    if (material.shininess > 0) {
        spec = pow(max(dot(viewDir, reflectDir), 0.0f), material.shininess);
    }
    vec3 specular = light.specular * spec * material.simpleSpecular;
    specular *= attenuation * intensity;

    return ambient + diffuse + specular;
}