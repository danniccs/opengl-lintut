#version 430 core
struct Material {
    sampler2D diffuse0;
    sampler2D specular0;
    sampler2D emission0;
    sampler2D reflex0;
    samplerCube cubeMap0;

    vec3 simpleAmbient;
    vec3 simpleDiffuse;
    vec3 simpleSpecular;
    vec3 simpleEmissive;

    float shininess;
};

in vec2 TexCoords;
in vec3 WorldObjPos;
in vec3 WorldNorm;

out vec4 FragColor;

uniform Material material;
uniform vec3 cameraPos;

void main() {
    vec4 diffColor = texture(material.diffuse0, TexCoords);
    vec4 specColor = texture(material.specular0, TexCoords);

    vec3 reflexInt = texture(material.reflex0, TexCoords).rgb;
    vec3 viewDir = normalize(WorldObjPos - cameraPos);
    vec3 cubeDir = reflect(viewDir, normalize(WorldNorm));
    vec3 reflexColor = reflexInt * texture(material.cubeMap0, cubeDir).rgb;

    FragColor = vec4(reflexColor, 1.0f);
    //FragColor = mix(diffColor, reflexColor, 0.5);
}