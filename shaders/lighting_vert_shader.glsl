#version 430 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 atexCoords;
layout(location = 4) in mat4 aModel;
layout(location = 8) in mat3 aNormMat;

out vec3 norm;
out vec2 texCoords;
out vec3 viewObjPos;

uniform mat4 view;
uniform mat4 projection;

void main() {
	gl_Position = projection * view * aModel * vec4(aPos, 1.0f);
	norm = aNormMat * aNorm;
	texCoords = atexCoords;
	viewObjPos = vec3(view * aModel * vec4(aPos, 1.0f));
}