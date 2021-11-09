#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "Shader.h"
#include "structures.h"

// Number of vertex buffers
const unsigned int SIMP_NUM_VBS = 3;


class SimpleMesh {
public:
	std::vector<Texture> textures;
	SimpleMesh(const float* vertices, unsigned int numVertices,
		std::vector<std::string> texturePaths, bool bSRGB = false, bool hasNormals = true,
		std::vector<GLenum> texParams = {}, std::vector<std::string> cubeTexturePaths = {});
	~SimpleMesh();
	void Draw(Shader shader, unsigned int numInstances, glm::mat4* models,
		glm::mat3* normMats);
	void getTextureLocations(Shader shader);
private:
	// Render data
	unsigned int numVertices;
	unsigned int VAO;
	unsigned int VBOs[SIMP_NUM_VBS];
	bool bSRGB;
	std::vector<Texture> loadTextures(std::vector<std::string> texturePaths,
		std::vector<GLenum> texParams = {});
	Texture loadCubeMaps(std::vector<std::string> texturePaths);
};

