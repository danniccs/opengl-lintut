#version 430 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aTexCoords;
layout(location = 4) in mat4 aModel;
layout(location = 8) in mat3 aNormMat;

out vec2 TexCoords;
out vec3 WorldObjPos;
out vec3 WorldNorm;

uniform Matrices {
	mat4 view;
	mat4 projection;
};

void main() {
	gl_Position = projection * view * aModel * vec4(aPos, 1.0f);
	TexCoords = aTexCoords;
	WorldObjPos = vec3(aModel * vec4(aPos, 1.0f));
	WorldNorm = aNormMat * aNorm;
}