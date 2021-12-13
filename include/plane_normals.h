#include <glm/glm.hpp>

const glm::vec3 planeNormals[7]{
    glm::vec3(1.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f),
    glm::vec3(0.0f, 0.0f, 1.0f),
    glm::vec3(sqrt(3) / 3, sqrt(3) / 3, sqrt(3) / 3),
    glm::vec3(-sqrt(3) / 3, sqrt(3) / 3, sqrt(3) / 3),
    glm::vec3(-sqrt(3) / 3, -sqrt(3) / 3, sqrt(3) / 3),
    glm::vec3(sqrt(3) / 3, -sqrt(3) / 3, sqrt(3) / 3),
};