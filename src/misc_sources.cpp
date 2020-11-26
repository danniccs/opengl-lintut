#include "misc_sources.h"
#include <iostream>

float getSubdet(glm::mat4 matrix, unsigned int col, unsigned int row) {
    float result;
    glm::mat3 auxMat;
    unsigned int skipc, skipr;
    skipc = 0;
    skipr = 0;
    for (unsigned int k = 0; k < 3; k++) {
        for (unsigned int l = 0; l < 3; l++) {
            if (k != col && l != row)
                auxMat[k][l] = matrix[k+skipc][l+skipr];
            else {
                if (k == col)
                    skipc = 1;
                else
                    skipr = 1;
            }
        }
    }
    result = glm::determinant(auxMat);
	return result;
}

float getSubdet(glm::mat3 matrix, unsigned int col, unsigned int row) {
    float result;
    glm::mat2 auxMat;
    unsigned int skipc, skipr;
    skipc = 0;
    skipr = 0;
    for (unsigned int k = 0; k < 2; k++) {
        for (unsigned int l = 0; l < 2; l++) {
            if (k != col && l != row)
                auxMat[k][l] = matrix[k + skipc][l + skipr];
            else {
                if (k == col)
                    skipc = 1;
                else
                    skipr = 1;
            }
        }
    }
    result = glm::determinant(auxMat);
    return result;
}

glm::mat4 getAdjoint(glm::mat4 matrix) {
	glm::mat4 result;
	for (unsigned int i = 0; i < 4; i++) {
		for (unsigned int j = 0; j < 4; j++) {
			result[j][i] = getSubdet(matrix, i, j);
            result = (i + j) & 1 ? -result:result;
		}
	}
	return result;
}

glm::mat3 getAdjoint(glm::mat3 matrix) {
    glm::mat3 result;
    for (unsigned int i = 0; i < 3; i++) {
        for (unsigned int j = 0; j < 3; j++) {
            result[j][i] = getSubdet(matrix, i, j);
            if ((i + j) & 1)
                result = -result;
        }
    }
    return result;
}

void printMatrix(glm::mat4 matrix) {
    for (unsigned int i = 0; i < 4; i++) {
        for (unsigned int j = 0; j < 4; j++) {
            std::cout << matrix[j][i] << '\t';
        }
        std::cout << '\n';
    }
}

void printMatrix(glm::mat3 matrix) {
    for (unsigned int i = 0; i < 3; i++) {
        for (unsigned int j = 0; j < 3; j++) {
            std::cout << matrix[j][i] << '\t';
        }
        std::cout << '\n';
    }
}

float planeVertices[] = {
	// positions         // texture Coords
	 5.0f, -0.5f, 5.0f,  2.0f, 0.0f,
	-5.0f, -0.5f, 5.0f,  0.0f, 0.0f,
	-5.0f, -0.5f, -5.0f, 0.0f, 2.0f,

	 5.0f, -0.5f, 5.0f,  2.0f, 0.0f,
	-5.0f, -0.5f, -5.0f, 0.0f, 2.0f,
	 5.0f, -0.5f, -5.0f, 2.0f, 2.0f
};

float cubeVertices[] = {
    // Back face                         Orientation when looking straight at the face
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f, // Bottom-right
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, // top-right
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f, // top-left
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f, // top-left
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f, // bottom-left
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f, // bottom-right
    // Front face
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f, // bottom-left
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f, // bottom-right
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f, // top-right
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f, // top-right
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, // top-left
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f, // bottom-left
    // Left face
    -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f, 0.0f,  1.0f, 0.0f, // top-right
    -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f, 0.0f,  1.0f, 1.0f, // top-left
    -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f, 0.0f,  0.0f, 1.0f, // bottom-left
    -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f, 0.0f,  0.0f, 1.0f, // bottom-left
    -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f, 0.0f,  0.0f, 0.0f, // bottom-right
    -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f, 0.0f,  1.0f, 0.0f, // top-right
    // Right face
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // top-left
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // bottom-right
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // top-right         
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // bottom-right
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // top-left
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // bottom-left     
    // Bottom face
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f, // top-right
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f, // top-left
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f, // bottom-left
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f, // bottom-left
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f, // bottom-right
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f, // top-right
    // Top face
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // top-left
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // bottom-right
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f, // top-right     
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // bottom-right
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // top-left
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f  // bottom-left        
};

float skyboxVertices[] = {
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

float quadVertices[] = {
    // positions          // texture Coords
    -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
     0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
     0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
     0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
    -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
};

float screenQuadVertices[] = {
    // positions   // texCoords
    -1.0f,  1.0f,  0.0f, 1.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f, -1.0f,  1.0f, 0.0f,

    -1.0f,  1.0f,  0.0f, 1.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
     1.0f,  1.0f,  1.0f, 1.0f
};

glm::vec3 cubePositions[] = {
    glm::vec3(-1.0f, 0.0001f, -1.0f),
    glm::vec3(2.0f, 0.0001f, 0.0f),
};

float pointLightAttenuationValues[4][3] = {
	// constant    linear    quadratic
	  {1.0f,       0.14f,    0.07f,},
	  {1.0f,       0.14f,    0.07f,},
	  {1.0f,       0.14f,    0.07f,},
	  {1.0f,       0.14f,    0.07f,},
};

glm::vec3 pointLightPositions[] = {
	glm::vec3(0.7f,  0.2f,  2.0f),
	glm::vec3(2.3f, -3.3f, -4.0f),
	glm::vec3(-4.0f,  2.0f, -12.0f),
	glm::vec3(0.0f,  0.0f, -3.0f)
};

glm::vec3 grassCoordinates[] = {
	glm::vec3(-1.0f, 0.0f, -0.48f),
	glm::vec3(2.0f, 0.0f, 0.51f),
	glm::vec3(0.5f, 0.0f, 0.7f),
	glm::vec3(0.2f, 0.0f, -2.3f),
	glm::vec3(1.0f, 0.0f, -0.6f),
};