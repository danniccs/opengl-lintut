#include "csm.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <utility>

const float logBase = 3.0f;
const float initialDivisor = 2.0f;

std::vector<glm::mat4> cascades::fitOrtho(const glm::mat4 &VPMat,
                                          float cascades, Light light,
                                          const unsigned int SMSize) {
  std::vector<glm::mat4> lightMatrices;
  std::vector<std::array<glm::vec4, 4>> worldCorners =
      getFrustumWorldCorners(VPMat, cascades);

  for (unsigned int i = 0; i < cascades; ++i) {
    glm::vec3 center;
    for (const auto &nearCorner : worldCorners[i])
      center += glm::vec3(nearCorner);
    for (const auto &farCorner : worldCorners[i + i])
      center += glm::vec3(farCorner);
    center /= 8;

    glm::mat4 lightView = glm::lookAt(center - light.direction, center,
                                      glm::vec3(0.0f, 1.0f, 0.0f));

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::min();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::min();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::min();
    for (const auto &corner : worldCorners[i]) {
      const auto trf = lightView * corner;
      minX = std::min(minX, trf.x);
      maxX = std::max(maxX, trf.x);
      minY = std::min(minY, trf.y);
      maxY = std::max(maxY, trf.y);
      minZ = std::min(minZ, trf.z);
      maxZ = std::max(maxZ, trf.z);
    }

    // Move the near and far planes closer and further, respectively.
    constexpr float zMult = 1.5f;
    if (minZ < 0.0f)
      minZ *= zMult;
    else
      minZ /= zMult;

    if (maxZ < 0.0f)
      maxZ /= zMult;
    else
      maxZ *= zMult;

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

    glm::mat4 lightProj = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

    lightMatrices.push_back(lightProj * lightView);
  }

  return lightMatrices;
}

std::vector<std::array<glm::vec4, 4>> getFrustumWorldCorners(
    const glm::mat4 &VPMat, float cascades) {
  std::vector<std::array<glm::vec4, 4>> corners;

  std::array<glm::vec4, 4> nearCorners;
  std::array<glm::vec4, 4> frustumVectors;
  int vectorCounter = 0;

  const glm::mat4 VPInv = glm::inverse(VPMat);
  const float NDCCorners[]{-1.0f, 1.0f};

  std::array<glm::vec4, 4> cascadeCorners;
  for (float x : NDCCorners) {
    for (float y : NDCCorners) {
      glm::vec4 nearCorner = VPInv * glm::vec4(x, y, -1.0f, 1.0f);
      glm::vec4 farCorner = VPInv * glm::vec4(x, y, 1.0f, 1.0f);
      nearCorner = nearCorner / nearCorner.w;
      farCorner = farCorner / farCorner.w;
      nearCorners[vectorCounter] = nearCorner;
      cascadeCorners[vectorCounter] = nearCorner;
      frustumVectors[vectorCounter] = farCorner - nearCorner;
      ++vectorCounter;
    }
  }
  corners.push_back(cascadeCorners);

  float power = pow(logBase, static_cast<float>(cascades - 2));
  float cascadeMultiplier = 1.0f / (initialDivisor * power);
  for (unsigned int i = 0; i < cascades - 1; ++i) {
    for (vectorCounter = 0; vectorCounter < 4; ++vectorCounter) {
      glm::vec4 nextCorner = nearCorners[vectorCounter] +
                             frustumVectors[vectorCounter] * initialDivisor;
      cascadeCorners[vectorCounter] = nextCorner;
    }
    corners.push_back(cascadeCorners);
    cascadeMultiplier *= 3.0f;
  }

  for (vectorCounter = 0; vectorCounter < 4; ++vectorCounter) {
    glm::vec4 nextCorner =
        nearCorners[vectorCounter] + frustumVectors[vectorCounter];
    cascadeCorners[vectorCounter] = nextCorner;
  }
  corners.push_back(cascadeCorners);

  return corners;
}