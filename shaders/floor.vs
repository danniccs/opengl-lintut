#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out VS_OUT {
    vec3 worldFragPos;
    vec3 normal;
    vec2 texCoords;
    vec4 fragPosLightSpace;
    vec4 fragPosSpotSpace;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normMat;
uniform mat4 lightSpaceMat;
uniform mat4 spotSpaceMat;

void main() {
    vs_out.worldFragPos = vec3(model * vec4(aPos, 1.0f));
    vs_out.normal = normalize(normMat * aNormal);
    vs_out.texCoords = aTexCoords;
    vs_out.fragPosLightSpace = lightSpaceMat * vec4(vs_out.worldFragPos, 1.0);
    vs_out.fragPosSpotSpace = spotSpaceMat * vec4(vs_out.worldFragPos, 1.0);
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}