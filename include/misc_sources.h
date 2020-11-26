#ifndef MISC_SOURCES_H
#define MISC_SOURCES_H

// Include glm for matrix defs
#include <glm/glm.hpp>

glm::mat4 getAdjoint(glm::mat4 matrix);

glm::mat3 getAdjoint(glm::mat3 matrix);

void printMatrix(glm::mat4 matrix);

void printMatrix(glm::mat3 matrix);

extern glm::vec3 pointLightPositions[4];

extern float pointLightAttenuationValues[4][3];

extern float planeVertices[30];

/*
    Remember: to specify vertices in a counter-clockwise winding order you need to visualize the triangle
    as if you're in front of the triangle and from that point of view, is where you set their order.

    To define the order of a triangle on the right side of the cube for example, you'd imagine yourself looking
    straight at the right side of the cube, and then visualize the triangle and make sure their order is specified
    in a counter-clockwise order. This takes some practice, but try visualizing this yourself and see that this
    is correct.
*/

extern float cubeVertices[288];

extern float skyboxVertices[108];

extern float quadVertices[30];

extern float screenQuadVertices[24];

extern glm::vec3 cubePositions[2];

extern glm::vec3 grassCoordinates[5];

#endif