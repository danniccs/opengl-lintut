#ifndef LIGHT_H
#define LIGHT_H

#include <string>
#include <glm/glm.hpp>
#include "Shader.h"


class Light {
public:
	Shader shader;

	// Default constructor
	Light();
	// Constructor sets some initial values and gets IDs
	Light(const std::string name, Shader shaderProg,
		bool directional = false, float constant = 1.0f, float linear = 1.0f,
		float quadratic = 1.0f, float cutOff = -1.0f, float outerCutOff = -1.0f);

	// Initializer function for use after construction
	void initVals(const std::string name, Shader shaderProg,
		bool directional = false, float constant = 1.0f, float linear = 1.0f,
		float quadratic = 1.0f, float cutOff = -1.0f, float outerCutOff = -1.0f);

	// Functions for setting parameters
	void setPos(glm::vec3 position, glm::mat4 view);
	void setDir(glm::vec3 direction, glm::mat3 dirNormMatrix);
	void setColors(glm::vec3 color, float ambientMult, float diffuseMult, float specularMult);

private:
	// array of IDs
	int IDs[5];
};

#endif