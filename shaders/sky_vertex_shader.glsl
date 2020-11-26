#version 430 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform Matrices {
	mat4 view;
	mat4 projection;
};

uniform mat4 skyView;

void main() {
	TexCoords = aPos;
	gl_Position = (projection * skyView * vec4(aPos, 1.0f)).xyww;
}