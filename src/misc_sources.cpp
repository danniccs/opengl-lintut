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

glm::mat4 sources::getAdjoint(glm::mat4 matrix) {
    glm::mat4 result;
    for (unsigned int i = 0; i < 4; i++) {
        for (unsigned int j = 0; j < 4; j++) {
            result[j][i] = getSubdet(matrix, i, j);
            result = (i + j) & 1 ? -result : result;
        }
    }
    return result;
}

glm::mat3 sources::getAdjoint(glm::mat3 matrix) {
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

void sources::printMatrix(glm::mat4 matrix) {
    for (unsigned int i = 0; i < 4; i++) {
        for (unsigned int j = 0; j < 4; j++) {
            std::cout << matrix[j][i] << '\t';
        }
        std::cout << '\n';
    }
}

void sources::printMatrix(glm::mat3 matrix) {
    for (unsigned int i = 0; i < 3; i++) {
        for (unsigned int j = 0; j < 3; j++) {
            std::cout << matrix[j][i] << '\t';
        }
        std::cout << '\n';
    }
}

const glm::vec3 sources::pointLightPositions[4] {
    glm::vec3(0.7f,  0.2f,  2.0f),
    glm::vec3(2.3f, -3.3f, -4.0f),
    glm::vec3(-4.0f,  2.0f, -12.0f),
    glm::vec3(0.0f,  0.0f, -3.0f)
};

const float sources::pointLightAttenuationValues[4][3] {
    // constant    linear    quadratic
    {1.0f,       0.14f,    0.07f,},
    {1.0f,       0.14f,    0.07f,},
    {1.0f,       0.14f,    0.07f,},
    {1.0f,       0.14f,    0.07f,},
};

const float sources::planeVertices[30] {
    // positions         // texture Coords
     5.0f, -0.5f, 5.0f,  2.0f, 0.0f,
    -5.0f, -0.5f, 5.0f,  0.0f, 0.0f,
    -5.0f, -0.5f, -5.0f, 0.0f, 2.0f,

     5.0f, -0.5f, 5.0f,  2.0f, 0.0f,
    -5.0f, -0.5f, -5.0f, 0.0f, 2.0f,
     5.0f, -0.5f, -5.0f, 2.0f, 2.0f
};

const float sources::cubeVertices[288] {
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

const float sources::skyboxVertices[108] {
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

const float sources::quadVertices[48] {
    // positions         // normals         // texture Coords
    -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
     0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  10.0f, 10.0f,
    -0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 10.0f,
     0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  10.0f, 10.0f,
    -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
     0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  10.0f, 0.0f,
};

const float sources::screenQuadVertices[24] {
    // positions   // texCoords
    -1.0f,  1.0f,  0.0f, 1.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f, -1.0f,  1.0f, 0.0f,

    -1.0f,  1.0f,  0.0f, 1.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
     1.0f,  1.0f,  1.0f, 1.0f
};

const glm::vec3 sources::cubePositions[2] {
    glm::vec3(-1.0f, 0.0001f, -1.0f),
    glm::vec3(2.0f, 0.0001f, 0.0f),
};

const glm::vec3 sources::grassCoordinates[5] {
    glm::vec3(-1.0f, 0.0f, -0.48f),
    glm::vec3(2.0f, 0.0f, 0.51f),
    glm::vec3(0.5f, 0.0f, 0.7f),
    glm::vec3(0.2f, 0.0f, -2.3f),
    glm::vec3(1.0f, 0.0f, -0.6f),
};

const glm::vec2 sources::poissonDisk[32] {
    glm::vec2(0.282571,   0.023957),
    glm::vec2(0.792657,   0.945738),
    glm::vec2(0.922361,   0.411756),
    glm::vec2(0.165838,   0.552995),
    glm::vec2(0.566027,   0.216651),
    glm::vec2(0.335398,   0.783654),
    glm::vec2(0.0190741,  0.318522),
    glm::vec2(0.647572,   0.581896),
    glm::vec2(0.916288,   0.0120243),
    glm::vec2(0.0278329,  0.866634),
    glm::vec2(0.398053,   0.4214),
    glm::vec2(0.00289926, 0.051149),
    glm::vec2(0.517624,   0.989044),
    glm::vec2(0.963744,   0.719901),
    glm::vec2(0.76867,    0.018128),
    glm::vec2(0.684194,   0.167302),
    glm::vec2(0.727103,   0.410871),
    glm::vec2(0.557482,   0.724143),
    glm::vec2(0.483352,   0.0527055),
    glm::vec2(0.162877,   0.351482),
    glm::vec2(0.959716,   0.180578),
    glm::vec2(0.140355,   0.112003),
    glm::vec2(0.796228,   0.223365),
    glm::vec2(0.187048,   0.787225),
    glm::vec2(0.55446,    0.35612),
    glm::vec2(0.449965,   0.640522),
    glm::vec2(0.438917,   0.194769),
    glm::vec2(0.791253,   0.565325),
    glm::vec2(0.719718,   0.794794),
    glm::vec2(0.0651875,  0.708609),
    glm::vec2(0.641987,   0.0233772),
    glm::vec2(0.376415,   0.944243),
};