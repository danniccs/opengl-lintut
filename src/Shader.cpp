#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "Shader.h"

Shader::Shader(const char *vertexPath, const char *fragmentPath,
               const char *geometryPath) {
  initVals(vertexPath, fragmentPath, geometryPath);
}

Shader::Shader() : ID(0){};

void Shader::initVals(const char *vertexPath, const char *fragmentPath,
                      const char *geometryPath) {
  // 1. Retrieve the vertex and fragment codes from the paths
  std::string vertexCode;
  std::string fragmentCode;
  std::string geometryCode;
  std::ifstream vShaderFile;
  std::ifstream fShaderFile;
  std::ifstream gShaderFile;
  // ensure ifstream objects can throw exceptions:
  vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  try {
    // open files
    vShaderFile.open(vertexPath);
    fShaderFile.open(fragmentPath);
    std::stringstream vShaderStream, fShaderStream, gShaderStream;
    // read file's buffer contents into streams
    vShaderStream << vShaderFile.rdbuf();
    fShaderStream << fShaderFile.rdbuf();
    // close file streams
    vShaderFile.close();
    fShaderFile.close();
    // copy stream content into string
    vertexCode = vShaderStream.str();
    fragmentCode = fShaderStream.str();

    if (geometryPath != nullptr) {
      gShaderFile.open(geometryPath);
      gShaderStream << gShaderFile.rdbuf();
      gShaderFile.close();
      geometryCode = gShaderStream.str();
    }
  } catch (std::ifstream::failure e) {
    std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
  }

  const char *vShaderCode = vertexCode.c_str();
  const char *fShaderCode = fragmentCode.c_str();
  const char *gShaderCode = geometryCode.c_str();

  // 2. Compile and link the shaders
  unsigned int vertex, fragment, geometry;
  int success;
  char infoLog[512];

  // vertex shader
  vertex = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex, 1, &vShaderCode, NULL);
  glCompileShader(vertex);
  // check for compile errors
  glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertex, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
              << infoLog << std::endl;
  }

  // fragment shader
  fragment = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment, 1, &fShaderCode, NULL);
  glCompileShader(fragment);
  // check for compile errors
  glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragment, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
              << infoLog << std::endl;
  }

  // geometry shader
  if (geometryPath != nullptr) {
    geometry = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(geometry, 1, &gShaderCode, NULL);
    glCompileShader(geometry);
    // check for compile errors
    glGetShaderiv(geometry, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(geometry, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::GEOMETRY::COMPILATION_FAILED\n"
                << infoLog << std::endl;
    }
  }

  // Link them in a shader program
  ID = glCreateProgram();
  glAttachShader(ID, vertex);
  glAttachShader(ID, fragment);
  if (geometryPath != nullptr)
    glAttachShader(ID, geometry);
  glLinkProgram(ID);
  // check for link errors
  glGetProgramiv(ID, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(ID, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::LINKING_FAILED\n" << infoLog << std::endl;
  }

  // delete the shaders
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  if (geometryPath != nullptr)
    glDeleteShader(geometry);
}

void Shader::use() const { glUseProgram(ID); }

void Shader::setLight(Light light) {
  this->use();
  const std::string &name = light.name;
  setUnifS(name + ".directional", light.directional);
  setUnifS(name + ".width", light.width);
  setUnifS(name + ".len", light.length);
  setUnifS(name + ".constant", light.falloffConstant);
  setUnifS(name + ".linear", light.falloffLinear);
  setUnifS(name + ".quadratic", light.falloffQuadratic);
  setUnifS(name + ".cutOff", light.cutOff);
  setUnifS(name + ".outerCutOff", light.outerCutOff);
  setUnifS(name + ".cLight", light.cLight);

  std::array<int, 2> &IDs = lightIDs[name];
  IDs[POS_ID] = getUnif(name + ".position");
  IDs[DIR_ID] = getUnif(name + ".direction");
}

void Shader::setLightPos(const Light &light, const glm::mat4 &transform) const {
  glm::vec4 aux = transform * glm::vec4(light.position, 1.0);
  glm::vec3 finalPos = glm::vec3(aux / aux.w);
  try {
    const std::array<int, 2> &IDs = lightIDs.at(light.name);
    setUnif(IDs[POS_ID], finalPos);
  } catch (const std::out_of_range &oor) {
    std::cerr << "Setting light position before calling setLight()\n";
  }
}

void Shader::setLightDir(const Light &light,
                         const glm::mat3 &dirNormMatrix) const {
  try {
    const std::array<int, 2> &IDs = lightIDs.at(light.name);
    setUnif(IDs[DIR_ID], dirNormMatrix * light.direction);
  } catch (const std::out_of_range &oor) {
    std::cerr << "Setting light direction before calling setLight()\n";
  }
}

int Shader::getUnif(const std::string name) const {
  return glGetUniformLocation(ID, name.c_str());
}

void Shader::setUnif(const int location, bool value) const {
  glUniform1i(location, static_cast<int>(value));
}

void Shader::setUnif(const int location, int value) const {
  glUniform1i(location, value);
}

void Shader::setUnif(const int location, unsigned int value) const {
  glUniform1i(location, value);
}

void Shader::setUnif(const int location, float value) const {
  glUniform1f(location, value);
}

void Shader::setUnif(const int location, float xVal, float yVal) const {
  glUniform2f(location, xVal, yVal);
}

void Shader::setUnif(const int location, float xVal, float yVal,
                     float zVal) const {
  glUniform3f(location, xVal, yVal, zVal);
}

void Shader::setUnif(const int location, float xVal, float yVal, float zVal,
                     float wVal) const {
  glUniform4f(location, xVal, yVal, zVal, wVal);
}

void Shader::setUnif(const int location, glm::vec2 vec) const {
  glUniform2fv(location, 1, glm::value_ptr(vec));
}

void Shader::setUnif(const int location, glm::vec3 vec) const {
  glUniform3fv(location, 1, glm::value_ptr(vec));
}

void Shader::setUnif(const int location, glm::vec4 vec) const {
  glUniform4fv(location, 1, glm::value_ptr(vec));
}

void Shader::setUnif(const int location, glm::mat2 mat) const {
  glUniformMatrix2fv(location, 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setUnif(const int location, glm::mat3 mat) const {
  glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setUnif(const int location, glm::mat4 mat) const {
  glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setUnifS(const std::string &name, bool value) const {
  glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::setUnifS(const std::string &name, int value) const {
  glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setUnifS(const std::string &name, unsigned int value) const {
  glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setUnifS(const std::string &name, float value) const {
  glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setUnifS(const std::string &name, float xVal, float yVal) const {
  glUniform2f(glGetUniformLocation(ID, name.c_str()), xVal, yVal);
}

void Shader::setUnifS(const std::string &name, float xVal, float yVal,
                      float zVal) const {
  glUniform3f(glGetUniformLocation(ID, name.c_str()), xVal, yVal, zVal);
}

void Shader::setUnifS(const std::string &name, float xVal, float yVal,
                      float zVal, float wVal) const {
  glUniform4f(glGetUniformLocation(ID, name.c_str()), xVal, yVal, zVal, wVal);
}

void Shader::setUnifS(const std::string &name, glm::vec2 vec) const {
  glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(vec));
}

void Shader::setUnifS(const std::string &name, glm::vec3 vec) const {
  glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(vec));
}

void Shader::setUnifS(const std::string &name, glm::vec4 vec) const {
  glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(vec));
}

void Shader::setUnifS(const std::string &name, glm::mat2 mat) const {
  glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE,
                     glm::value_ptr(mat));
}

void Shader::setUnifS(const std::string &name, glm::mat3 mat) const {
  glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE,
                     glm::value_ptr(mat));
}

void Shader::setUnifS(const std::string &name, glm::mat4 mat) const {
  glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE,
                     glm::value_ptr(mat));
}