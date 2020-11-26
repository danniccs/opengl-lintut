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

in vec2 TexCoords;
in vec3 WorldObjPos;
in vec3 WorldNorm;

out vec4 FragColor;

uniform Material material;
uniform vec3 cameraPos;

void main() {
    /*
    vec4 texColor = texture(material.diffuse0, TexCoords);
    if (texColor.a < 0.1)
        discard;
    FragColor = texColor;
    */
    vec3 viewDir = normalize(WorldObjPos - cameraPos);
    vec3 cubeDir = reflect(viewDir, normalize(WorldNorm));
    FragColor = vec4(texture(material.cubeMap0, cubeDir));
}