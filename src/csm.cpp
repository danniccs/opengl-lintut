#include "csm.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <limits>
#include <utility>

#include "misc_sources.h"

const float logBase = 3.0f;
const float initialDivisor = 2.0f;

std::vector<glm::mat4> cascades::fitOrtho(const glm::mat4 &VPMat,
                                          float cascades, Light &light,
                                          unsigned int SMSize) {
  std::vector<glm::mat4> lightMatrices;
  std::vector<std::array<glm::vec4, 4>> worldCorners =
      getFrustumWorldCorners(VPMat, cascades);

  for (unsigned int i = 0; i < cascades; ++i) {
    glm::vec3 center(0.0f);
    for (const auto &nearCorner : worldCorners[i]) {
      center += glm::vec3(nearCorner);
    }
    for (const auto &farCorner : worldCorners[i + 1]) {
      center += glm::vec3(farCorner);
    }
    center = center / 8.0f;

    glm::mat4 lightView = glm::lookAt(center - 20.0f * light.direction, center,
                                      glm::vec3(0.0f, 1.0f, 0.0f));
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();
    for (const auto &corner : worldCorners[i]) {
      glm::vec4 trf = lightView * corner;
      minX = std::min(minX, trf.x);
      maxX = std::max(maxX, trf.x);
      minY = std::min(minY, trf.y);
      maxY = std::max(maxY, trf.y);
      minZ = std::min(minZ, trf.z);
      maxZ = std::max(maxZ, trf.z);
    }
    for (const auto &corner : worldCorners[i + 1]) {
      glm::vec4 trf = lightView * corner;
      minX = std::min(minX, trf.x);
      maxX = std::max(maxX, trf.x);
      minY = std::min(minY, trf.y);
      maxY = std::max(maxY, trf.y);
      minZ = std::min(minZ, trf.z);
      maxZ = std::max(maxZ, trf.z);
    }

    // Modify maxZ and minZ, due to the OpenGL coordinate system.
    if (minZ < 0.0f) {
      float tmp = maxZ;
      maxZ = -minZ;
      minZ = -tmp;
    }
    else {
      minZ = -minZ;
      maxZ = -maxZ;
    }
    // Move the near and far planes closer and further, respectively.
    constexpr float zMult = 1.0f;

    /*
    Move the X and Y values in texel sized increments to avoid shimmering
    edges.
    */
    glm::vec3 boundDiagonal =
        glm::vec3(minX, minY, minZ) - glm::vec3(maxX, maxY, maxZ);
    float worldUnitsPerTexel = glm::length(boundDiagonal) / SMSize;
    minX /= worldUnitsPerTexel;
    minX = floor(minX);
    minX *= worldUnitsPerTexel;
    maxX /= worldUnitsPerTexel;
    maxX = floor(maxX);
    maxX *= worldUnitsPerTexel;
    minY /= worldUnitsPerTexel;
    minY = floor(minY);
    minY *= worldUnitsPerTexel;
    maxY /= worldUnitsPerTexel;
    maxY = floor(maxY);
    maxY *= worldUnitsPerTexel;
    minZ /= worldUnitsPerTexel;
    minZ = floor(minZ);
    minZ *= worldUnitsPerTexel;
    maxZ /= worldUnitsPerTexel;
    maxZ = floor(maxZ);
    maxZ *= worldUnitsPerTexel;

    glm::mat4 lightProj = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

    lightMatrices.push_back(lightProj * lightView);
  }

  return lightMatrices;
}

std::vector<std::array<glm::vec4, 4>> cascades::getFrustumWorldCorners(
    const glm::mat4 &VPMat, float cascades) {
  std::vector<std::array<glm::vec4, 4>> corners;

  std::array<glm::vec4, 4> nearCorners;
  std::array<glm::vec4, 4> farCorners;
  std::array<glm::vec4, 4> frustumVectors;

  const glm::mat4 VPInv = glm::inverse(VPMat);
  const float NDCCorners[]{-1.0f, 1.0f};

  unsigned int vectorCounter = 0;
  for (float x : NDCCorners) {
    for (float y : NDCCorners) {
      glm::vec4 nearCorner = VPInv * glm::vec4(x, y, -1.0f, 1.0f);
      glm::vec4 farCorner = VPInv * glm::vec4(x, y, 1.0f, 1.0f);
      nearCorner = nearCorner / nearCorner.w;
      farCorner = farCorner / farCorner.w;
      nearCorners[vectorCounter] = nearCorner;
      farCorners[vectorCounter] = farCorner;
      frustumVectors[vectorCounter] = farCorner - nearCorner;
      ++vectorCounter;
    }
  }
  corners.push_back(nearCorners);

  std::array<glm::vec4, 4> cascadeCorners;
  float power = pow(logBase, static_cast<float>(cascades - 2));
  float cascadeMultiplier = 1.0f / (initialDivisor * power);
  for (unsigned int i = 0; i < cascades - 2; ++i) {
    for (vectorCounter = 0; vectorCounter < 4; ++vectorCounter) {
      glm::vec4 nextCorner = nearCorners[vectorCounter] +
                             frustumVectors[vectorCounter] * cascadeMultiplier;
      cascadeCorners[vectorCounter] = nextCorner;
    }
    corners.push_back(cascadeCorners);
    cascadeMultiplier *= logBase;
  }

  corners.push_back(farCorners);

  return corners;
}

std::vector<float> cascades::getCSMPlaneDistances(float cascades,
                                                  float cameraFarPlane) {
  std::vector<float> cascadePlaneDistances;
  float power = pow(logBase, static_cast<float>(cascades - 2));
  float cascadeMultiplier = 1.0f / (initialDivisor * power);

  for (unsigned int i = 0; i < cascades - 1; ++i) {
    cascadePlaneDistances.push_back(cameraFarPlane * cascadeMultiplier);
    cascadeMultiplier *= logBase;
  }
  cascadePlaneDistances.push_back(cameraFarPlane);

  return cascadePlaneDistances;
}