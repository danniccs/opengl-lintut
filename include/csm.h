#ifndef CSM_H
#define CSM_H

#include <array>
#include <glm/glm.hpp>
#include <vector>

#include "Light.h"

namespace cascades {

// lightMatrices must either be empty or have numCascades matrices.
void fitToFrustum(const glm::mat4 &VPMat, float numCascades,
                  float cameraNearPlane, float cameraFarPlane,
                  const Light &light, unsigned int shadowMapWidth,
                  unsigned int shadowMapHeight,
                  std::vector<glm::mat4> &lightMatrices);

std::vector<std::array<glm::vec4, 4>> getFrustumWorldCorners(
    const glm::mat4 &VPMat, float numCascades, float cameraNearPlane,
    float cameraFarPlane);

std::vector<float> getCSMPlaneDistances(float numCascades,
                                        float cameraNearPlane,
                                        float cameraFarPlane);

}  // namespace cascades

#endif