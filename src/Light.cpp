#include "Light.h"
#include <iostream>

// Define indices in light arrays
const unsigned int POS_ID = 0;
const unsigned int DIR_ID = 1;
const unsigned int AMB_ID = 2;
const unsigned int DIFF_ID = 3;
const unsigned int SPEC_ID = 4;

Light::Light() : IDs() {}

Light::Light(const std::string name, Shader shaderProg, bool directional,
    float width, float constant, float linear, float quadratic,
    float cutOff, float outerCutOff)
{
    initVals(name, shaderProg, directional, width, constant, linear,
        quadratic, cutOff, outerCutOff);
}

void Light::initVals(const std::string name, Shader shaderProg,
    bool directional, float width, float constant, float linear,
    float quadratic, float cutOff, float outerCutOff)
{
    shader = shaderProg;
    shader.use();
    shader.setUnifS(name + ".directional", directional);
    shader.setUnifS(name + ".width", width);
    shader.setUnifS(name + ".constant", constant);
    shader.setUnifS(name + ".linear", linear);
    shader.setUnifS(name + ".quadratic", quadratic);
    shader.setUnifS(name + ".cutOff", cutOff);
    shader.setUnifS(name + ".outerCutOff", outerCutOff);
    IDs[POS_ID] = shader.getUnif(name + ".position");
    IDs[DIR_ID] = shader.getUnif(name + ".direction");
    IDs[AMB_ID] = shader.getUnif(name + ".ambient");
    IDs[DIFF_ID] = shader.getUnif(name + ".diffuse");
    IDs[SPEC_ID] = shader.getUnif(name + ".specular");
}

void Light::setPos(const glm::vec3 position, glm::mat4 view) {
    glm::vec4 aux = view * glm::vec4(position, 1.0);
    glm::vec3 viewPos = glm::vec3(aux / aux.w);
    shader.setUnif(IDs[POS_ID], viewPos);
}

void Light::setDir(const glm::vec3 direction, glm::mat3 dirNormMatrix) {
    shader.setUnif(IDs[DIR_ID], dirNormMatrix * direction);
}

void Light::setColors(const glm::vec3 color, float ambientMult,
    float diffuseMult, float specularMult)
{
    shader.setUnif(IDs[AMB_ID], color * ambientMult);
    shader.setUnif(IDs[DIFF_ID], color * diffuseMult);
    shader.setUnif(IDs[SPEC_ID], color * specularMult);
}