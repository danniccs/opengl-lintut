#ifndef LIGHT_H
#define LIGHT_H

#include <string>
#include <glm/glm.hpp>
#include "Shader.h"

struct Light {
	std::string name;
	bool directional = false;
	float width = 0.0f;
	float falloffConstant = 1.0f;
	float falloffLinear = 1.0f;
	float falloffQuadratic = 1.0f;
	float cutOff = -1.0f;
	float outerCutOff = -1.0f;
	glm::vec3 cLight = glm::vec3(0.0f);
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 direction = glm::vec3(0.0f);
	float ambientMult = 1.0f;
	float diffuseMult = 1.0f;
	float specularMult = 1.0f;
};

#endif