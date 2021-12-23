#ifndef CSM_H
#define CSM_H

#include <glm/glm.hpp>
#include <vector>

#include "Light.h"

namespace cascades {

std::vector<glm::mat4> fitOrtho(glm::mat4 VPMat, float cascades, Light light,
                                float SMSize);

}

#endif