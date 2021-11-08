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

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

uniform sampler2D floorTexture;
uniform sampler2D randomAngles;
uniform sampler2DShadow shadowMap;
uniform sampler2DShadow spotShadowMap;
uniform vec3 viewPos;
uniform Light spotLight;
uniform Light dirLight;

vec2 poissonDisk[4] = vec2[](
  vec2( -0.94201624, -0.39906216 ),
  vec2( 0.94558609, -0.76890725 ),
  vec2( -0.094184101, -0.92938870 ),
  vec2( 0.34495938, 0.29387760 )
);
float poissonSpread = 700.0;

float shadowCalculation(vec4 pos, float ndotl, sampler2DShadow map) {
    // perform perspective divide
    vec3 projCoords = pos.xyz / pos.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // Get random coordinates to rotate poisson disk
    ivec2 angleTexCoords = ivec2(fs_in.fragPos.x, fs_in.fragPos.y);
    vec4 rotation = texture(randomAngles, angleTexCoords, 0);

    float shadow = 0.0;
    // Check if fragment is beyond far plane of frustum
    if (projCoords.z <= 1.0) {
        float bias = max(0.02 * (1.0 -  ndotl), 0.005);
        projCoords.z -= bias;

        // check whether fragment is in shadow or not
        for (int i = 0; i < 4; i++) {
            float sampleX = projCoords.x + (poissonDisk[i].x + rotation.x) / poissonSpread;
            float sampleY = projCoords.y + (poissonDisk[i].y + rotation.y) / poissonSpread;
            vec3 sampleCoords = vec3(sampleX, sampleY, projCoords.z);
            shadow += texture(map, sampleCoords);
        }
        shadow *= 0.25;
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

    float dirShadow = shadowCalculation(fs_in.fragPosLightSpace, ndotl, shadowMap);
    vec3 dirColor = ambient + dirShadow * (diffuse + specular);

    // Spot light
    float epsilon = spotLight.cutOff - spotLight.outerCutOff;
    lightDir = normalize(spotLight.position - fs_in.fragPos);
    float theta = dot(lightDir, normalize(-spotLight.direction));
    float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);
    float dist = length(spotLight.position - fs_in.fragPos);
    float attenuation = 1.0 / (spotLight.constant + spotLight.linear * dist + spotLight.quadratic * (dist * dist));
    ndotl = max(dot(lightDir, normal), 0.0);
    // diffuse
    diffuse = ndotl * color * spotLight.diffuse * attenuation * intensity;
    // specular
    halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    specular = spotLight.specular * spec * attenuation * intensity;

    float spotShadow = shadowCalculation(fs_in.fragPosSpotSpace, ndotl, spotShadowMap);
    vec3 spotColor = spotShadow * (diffuse + specular);

    FragColor = vec4(dirColor + spotColor, 1.0);
}