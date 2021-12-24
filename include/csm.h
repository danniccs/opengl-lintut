#ifndef CSM_H
#define CSM_H

#include <glm/glm.hpp>
#include <vector>
#include <array>

#include "Light.h"

namespace cascades {

std::vector<glm::mat4> fitOrtho(const glm::mat4& VPMat, float cascades, Light& light,
                                unsigned int SMSize);

std::vector<std::array<glm::vec4, 4>> getFrustumWorldCorners(
    const glm::mat4 &VPMat, float cascades);

std::vector<float> getCSMPlaneDistances(float cascades, float cameraFarPlane);

}  // namespace cascades

#endif