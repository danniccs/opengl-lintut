#version 430 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 atexCoords;
layout(location = 4) in mat4 aModel;
layout(location = 8) in mat3 aNormMat;

out VS_OUT {
    vec3 norm;
    vec2 texCoords;
    vec3 viewObjPos;
    vec4 fragPosLightSpace;
    vec4 fragPosSpotSpace;
} vs_out;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMat;
uniform mat4 spotSpaceMat;

#define PI 3.1415926538

void main() {
	gl_Position = projection * view * aModel * vec4(aPos, 1.0f);
	vs_out.norm = normalize(aNormMat * aNorm);
    // Approximate texture coordinates of sphere using normal.
    // Taken from https://www.mvps.org/directx/articles/spheremap.htm
	vs_out.texCoords = vec2(asin(aNorm.x)/PI + 0.5, asin(aNorm.y)/PI + 0.5);
	vs_out.viewObjPos = vec3(view * aModel * vec4(aPos, 1.0f));
    vs_out.fragPosLightSpace = lightSpaceMat * aModel * vec4(aPos, 1.0);
    vs_out.fragPosSpotSpace = spotSpaceMat * aModel * vec4(aPos, 1.0);
}