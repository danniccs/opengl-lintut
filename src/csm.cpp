#include "csm.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <limits>
#include <utility>

#include "misc_sources.h"

constexpr float weight = 0.5f;
constexpr float g_near = 0.0f;
constexpr float g_far = 200.0f;
constexpr float g_extent = 100.0f;
constexpr float g_fov = glm::radians(90.0f);
constexpr float g_aspect = 1.0f;

void cascades::fitToFrustum(const glm::mat4 &VPMat, float numCascades,
                            float cameraNearPlane, float cameraFarPlane,
                            const Light &light, unsigned int shadowMapWidth,
                            unsigned int shadowMapHeight,
                            std::vector<glm::mat4> &lightMatrices) {
  float floatWidth = static_cast<float>(shadowMapWidth);
  float floatHeight = static_cast<float>(shadowMapHeight);

  std::vector<std::array<glm::vec4, 4>> worldCorners = getFrustumWorldCorners(
      VPMat, numCascades, cameraNearPlane, cameraFarPlane);

  for (unsigned int i = 0; i < numCascades; ++i) {
    glm::mat4 lightView =
        glm::lookAt(glm::vec3(0.0f) - light.direction * 0.5f * g_far,
                    glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 lightProj;
    if (light.directional) {
      lightProj =
          glm::ortho(-g_extent, g_extent, -g_extent, g_extent, g_near, g_far);
    } else {
      lightProj = glm::perspective(g_fov, g_aspect, g_near, g_far);
    }

    glm::mat4 lightSpaceMat = lightProj * lightView;

    // Get the view frustum coordinates in light view space.
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();
    glm::vec4 tmp;
    using std::max;
    using std::min;
    for (const auto &nearCorner : worldCorners[i]) {
      tmp = lightSpaceMat * nearCorner;
      tmp = tmp / tmp.w;
      minX = min(minX, tmp.x);
      maxX = max(maxX, tmp.x);
      minY = min(minY, tmp.y);
      maxY = max(maxY, tmp.y);
      minZ = min(minZ, tmp.z);
      maxZ = max(maxZ, tmp.z);
    }
    for (const auto &farCorner : worldCorners[i + 1]) {
      tmp = lightSpaceMat * farCorner;
      tmp = tmp / tmp.w;
      minX = min(minX, tmp.x);
      maxX = max(maxX, tmp.x);
      minY = min(minY, tmp.y);
      maxY = max(maxY, tmp.y);
      minZ = min(minZ, tmp.z);
      maxZ = max(maxZ, tmp.z);
    }

    // Move the near and far planes closer and further, respectively.
    constexpr float zMult = 0.2f;
    minZ -= std::abs(minZ) * zMult;
    maxZ += std::abs(maxZ) * zMult;

    float scaleZ = 1.0f / (maxZ - minZ);
    float offsetZ = -minZ * scaleZ;

    // Approximate and quantize scale.
    float scaleX = 2.0f / (maxX - minX);
    float scaleY = 2.0f / (maxY - minY);
    float scaleQuantizer = 64.0f;
    scaleX = 1.0f / std::ceil(1.0f / scaleX * scaleQuantizer) * scaleQuantizer;
    scaleY = 1.0f / std::ceil(1.0f / scaleY * scaleQuantizer) * scaleQuantizer;

    // Approximate and quantize offset.
    float offsetX = -0.5f * (maxX + minX) * scaleX;
    float offsetY = -0.5f * (maxY + minY) * scaleY;
    float halfTextureX = 0.5f * floatWidth;
    float halfTextureY = 0.5f * floatHeight;
    offsetX = ceil(offsetX * halfTextureX) / halfTextureX;
    offsetY = ceil(offsetY * halfTextureY) / halfTextureY;

    glm::mat4 cropMatrix(glm::vec4(scaleX, 0.0f, 0.0f, 0.0f),
                         glm::vec4(0.0f, scaleY, 0.0f, 0.0f),
                         glm::vec4(0.0f, 0.0f, scaleZ, 0.0f),
                         glm::vec4(offsetX, offsetY, offsetZ, 1.0f));

    if (lightMatrices.size() == numCascades)
      lightMatrices[i] = cropMatrix * lightSpaceMat;
    else if (lightMatrices.size() == i)
      lightMatrices.push_back(cropMatrix * lightSpaceMat);
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