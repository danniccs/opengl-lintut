#ifndef MESH_H
#define MESH_H

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "Shader.h"
#include "structures.h"

// Number of vertex buffers
const unsigned int NUM_VBS = 4;


class Mesh {
public:
	// Mesh Data
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;
	Material material;
	unsigned int simpId;
	// Functions
	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices,
		std::vector<Texture> textures, Material material);
	void freeMesh();
	void Draw(Shader shader, unsigned int numInstances, glm::mat4* models,
			  glm::mat3* normMats) const;
	void getTextureLocations(Shader shader);
private:
	// Render Data
	unsigned int VAO, EBO;
	unsigned int VBOs[NUM_VBS];
	// Functions
	void setupMesh();
};

#endif