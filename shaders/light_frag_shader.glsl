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

out vec4 FragColor;

uniform Material material;

void main() {
    //vec3 result = material.simpleAmbient + material.simpleDiffuse
    //    + material.simpleSpecular + material.simpleEmissive;
    vec3 result = material.simpleEmissive;
    FragColor = vec4(result, 1.0f);
}