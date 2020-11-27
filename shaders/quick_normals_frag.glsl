#version 330 core

in vec3 Col;

out vec4 FragColor;

uniform float first;
uniform sampler2DMS depthText;

void main() {
	if (first != 1.0f) {
		ivec2 texCoords = ivec2(gl_FragCoord.x, gl_FragCoord.y);
		float depth = texelFetch(depthText, texCoords, 0).s;
		if (gl_FragCoord.z <= depth)
			discard;
	}
	FragColor = vec4(Col, 0.1f);
}