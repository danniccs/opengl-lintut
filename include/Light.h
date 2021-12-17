#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>
#include <string>

struct Light {
  Light(std::string aName, bool isDirectional = false, float aWidth = 0.0f,
        float aFalloffConstant = 1.0f, float aFalloffLinear = 1.0f,
        float aFalloffQuadratic = 1.0f, float aCutOff = -1.0f,
        float aOuterCutOff = -1.0f, glm::vec3 aCLight = glm::vec3(0.0f),
        glm::vec3 aPosition = glm::vec3(0.0f),
        glm::vec3 aDirection = glm::vec3(0.0f))
      : name(aName), directional(isDirectional), width(aWidth),
        falloffConstant(aFalloffConstant), falloffLinear(aFalloffLinear),
        falloffQuadratic(aFalloffQuadratic), cutOff(aCutOff),
        outerCutOff(aOuterCutOff), cLight(aCLight), position(aPosition),
        direction(aDirection) {}

  std::string name;
  bool directional;
  float width;
  float falloffConstant;
  float falloffLinear;
  float falloffQuadratic;
  float cutOff;
  float outerCutOff;
  glm::vec3 cLight;
  glm::vec3 position;
  glm::vec3 direction;
};

#endif