#include "csm.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <limits>
#include <utility>

#include "misc_sources.h"

constexpr float weight = 0.5f;

void cascades::fitOrtho(const glm::mat4 &VPMat, float numCascades,
                        float cameraNearPlane, float cameraFarPlane,
                        const Light &light, unsigned int shadowMapWidth,
                        unsigned int shadowMapHeight,
                        std::vector<glm::mat4> &lightMatrices) {
  float floatWidth = static_cast<float>(shadowMapWidth);
  float floatHeight = static_cast<float>(shadowMapHeight);

  std::vector<std::array<glm::vec4, 4>> worldCorners = getFrustumWorldCorners(
      VPMat, numCascades, cameraNearPlane, cameraFarPlane);

  for (unsigned int i = 0; i < numCascades; ++i) {
    glm::vec4 center(0.0f);
    for (const auto &nearCorner : worldCorners[i]) {
      center += nearCorner;
    }
    for (const auto &farCorner : worldCorners[i + 1]) {
      center += farCorner;
    }
    center = center / 8.0f;

    glm::mat4 lightView =
        glm::lookAt(glm::vec3(center) - light.direction, glm::vec3(center),
                    glm::vec3(0.0f, 1.0f, 0.0f));

    // Get the longest radius in world space
    float radius = glm::length(center - worldCorners[i][0]);
    for (const auto &nearCorner : worldCorners[i]) {
      float distance = glm::length(nearCorner - center);
      radius = glm::max(radius, distance);
    }
    for (const auto &farCorner : worldCorners[i + 1]) {
      float distance = glm::length(farCorner - center);
      radius = glm::max(radius, distance);
    }
    radius = std::ceil(radius);
    float f = (radius * 2.0f) / floatWidth;

    glm::vec3 vRadius = glm::vec3(radius, radius, -radius);
    glm::vec3 maxOrtho = glm::vec3(center) + vRadius;
    glm::vec3 minOrtho = glm::vec3(center) - vRadius;
    minOrtho.x = floor(minOrtho.x / f) * f;
    minOrtho.y = floor(minOrtho.y / f) * f;

    maxOrtho.x = minOrtho.x + radius * 2.0f;
    maxOrtho.y = minOrtho.y + radius * 2.0f;

    // Store the far and near planes
    float far = maxOrtho.z;
    float near = minOrtho.z;

    // Modify maxZ and minZ, due to the OpenGL coordinate system.
    if (near < 0.0f) {
      float tmp = far;
      far = -near;
      near = -tmp;
    } else {
      near = -near;
      far = -far;
    }

    // Move the near and far planes closer and further, respectively.
    constexpr float zMult = 0.6f;
    near -= std::abs(near) * zMult;
    far += std::abs(far) * zMult;

    glm::mat4 lightProj =
        glm::ortho(minOrtho.x, maxOrtho.x, minOrtho.y, maxOrtho.y, near, far);

    /*
    // Create the rounding matrix, by projecting the world-space origin and
    // determining
    // the fractional offset in texel space
    glm::mat4 shadowMatrix = lightProj * lightView;
    glm::vec4 shadowOrigin = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    shadowOrigin = shadowMatrix * shadowOrigin;
    float storedW = shadowOrigin.w;
    shadowOrigin = shadowOrigin * (floatWidth * 2.0f) / 2.0f;

    glm::vec4 roundedOrigin = glm::round(shadowOrigin);
    glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
    roundOffset = roundOffset * 2.0f / (floatWidth * 2.0f);
    roundOffset.z = 0.0f;
    roundOffset.w = 0.0f;

    glm::mat4 shadowProj = lightProj;
    shadowProj[3] += roundOffset;
    lightProj = shadowProj;
    */

    if (lightMatrices.size() == numCascades)
      lightMatrices[i] = lightProj * lightView;
    else if (lightMatrices.size() == i)
      lightMatrices.push_back(lightProj * lightView);
  }
}

std::vector<std::array<glm::vec4, 4>> cascades::getFrustumWorldCorners(
    const glm::mat4 &VPMat, float numCascades, float cameraNearPlane,
    float cameraFarPlane) {
  std::vector<std::array<glm::vec4, 4>> corners;

  std::array<glm::vec4, 4> nearCorners;
  std::array<glm::vec4, 4> farCorners;
  std::array<glm::vec4, 4> frustumVectors;

  const glm::mat4 VPInv = glm::inverse(VPMat);
  const float NDCCorners[]{-1.0f, 1.0f};

  unsigned int vecIt = 0;
  for (float x : NDCCorners) {
    for (float y : NDCCorners) {
      glm::vec4 nearCorner = VPInv * glm::vec4(x, y, -1.0f, 1.0f);
      glm::vec4 farCorner = VPInv * glm::vec4(x, y, 1.0f, 1.0f);
      nearCorner = nearCorner / nearCorner.w;
      farCorner = farCorner / farCorner.w;
      nearCorners[vecIt] = nearCorner;
      farCorners[vecIt] = farCorner;
      frustumVectors[vecIt] = farCorner - nearCorner;
      ++vecIt;
    }
  }
  corners.push_back(nearCorners);

  std::array<glm::vec4, 4> cascadeCorners;
  for (unsigned int i = 0; i < numCascades - 1; ++i) {
    float f = static_cast<float>(i + 1) / static_cast<float>(numCascades);
    float logDist = cameraNearPlane * pow(cameraFarPlane / cameraNearPlane, f);
    float uniDist = cameraNearPlane + (cameraFarPlane - cameraNearPlane) * f;
    float splitDist = glm::mix(uniDist, logDist, weight);
    float distRatio = splitDist / (cameraFarPlane - cameraNearPlane);
    for (vecIt = 0; vecIt < 4; ++vecIt) {
      glm::vec4 nextCorner =
          nearCorners[vecIt] + frustumVectors[vecIt] * distRatio;
      cascadeCorners[vecIt] = nextCorner;
    }
    corners.push_back(cascadeCorners);
  }

  corners.push_back(farCorners);

  return corners;
}

std::vector<float> cascades::getCSMPlaneDistances(float numCascades,
                                                  float cameraNearPlane,
                                                  float cameraFarPlane) {
  std::vector<float> cascadePlaneDistances;

  for (unsigned int i = 0; i < numCascades; ++i) {
    float f = static_cast<float>(i + 1) / static_cast<float>(numCascades);
    float logDist = cameraNearPlane * pow(cameraFarPlane / cameraNearPlane, f);
    float uniDist = cameraNearPlane + (cameraFarPlane - cameraNearPlane) * f;
    float splitDist = glm::mix(uniDist, logDist, weight);
    cascadePlaneDistances.push_back(splitDist);
  }

  return cascadePlaneDistances;
}