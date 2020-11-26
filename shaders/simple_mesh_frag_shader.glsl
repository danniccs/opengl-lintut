#version 430 core
struct Material {
    sampler2D diffuse0;
    sampler2D specular0;
    sampler2D emission0;
    samplerCube cubeMap0;

    vec3 simpleAmbient;
    vec3 simpleDiffuse;
    vec3 simpleSpecular;
    vec3 simpleEmissive;

    float shininess;
};

in vec2 texCoords;

out vec4 FragColor;

uniform Material material;

void main() {
    vec4 texColor = texture(material.diffuse0, texCoords);
    //if (texColor.a < 0.1)
    //    discard;
    FragColor = texture(material.diffuse0, texCoords);
}