#version 430 core
layout(location = 0) in vec3 aPos;

uniform mat4 VPI;

void main() { gl_Position = VPI * vec4(aPos, 1.0); }