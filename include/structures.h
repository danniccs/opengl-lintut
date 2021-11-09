#pragma once

#include <glm/glm.hpp>
#include <string>

// Buffer indices
const unsigned int POS_NORM_TEX_VB = 0;
const unsigned int MOD_VB = 1;
const unsigned int NORM_M_VB = 2;
const unsigned int COL_VB = 3;

// Attribute locations (objects)
const unsigned int POS_LOC = 0;
const unsigned int NORM_LOC = 1;
const unsigned int TEX_LOC = 2;
const unsigned int COL_LOC = 3;
const unsigned int MOD_LOC = 4;
const unsigned int NORM_M_LOC = 8;

// Material color indices
const unsigned int AMB = 0;
const unsigned int DIFF = 1;
const unsigned int SPEC = 2;

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
};

struct Texture {
	unsigned int id;
	unsigned int location;
	std::string type;
	std::string path;
};

struct Material {
	glm::vec3 Ambient;
	glm::vec3 Diffuse;
	glm::vec3 Specular;
	glm::vec3 Emissive;
	float Shininess;
	unsigned int ambId;
	unsigned int diffId;
	unsigned int specId;
	unsigned int emisId;
	unsigned int shinId;
};