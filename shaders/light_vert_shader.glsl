#version 430 core
layout(location = 0) in vec3 aPos;
layout(location = 3) in vec3 aColor;
layout(location = 4) in mat4 aModel;

out vec3 lightColor;

uniform mat4 view;
uniform mat4 projection;

void main() {
	gl_Position = projection * view * aModel * vec4(aPos, 1.0f);
	lightColor = aColor;
}