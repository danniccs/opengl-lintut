#version 430 core
struct Material {
	samplerCube cubeMap0;
};

out vec4 FragColor;

in vec3 TexCoords;

uniform Material material;

void main() {
	FragColor = texture(material.cubeMap0, TexCoords);
}