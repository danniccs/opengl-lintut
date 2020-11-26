#version 430 core
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 atexCoords;
layout(location = 4) in mat4 aModel;

out vec2 texCoords;

uniform Matrices {
	mat4 view;
	mat4 projection;
};

void main() {
	gl_Position = projection * view * aModel * vec4(aPos, 1.0f);
	texCoords = atexCoords;
}