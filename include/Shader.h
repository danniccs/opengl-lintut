#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <map>
#include <array>
#include <glm/glm.hpp>
#include "Light.h"

// Define indices in light arrays
const unsigned int POS_ID = 0;
const unsigned int DIR_ID = 1;


// class for a shader program (includes vertex and fragment shaders)
class Shader {
public:
	// program ID
	unsigned int ID;

	// constructor functions
	Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);
	Shader();

	// initialization function (in case of default constructor)
	void initVals(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);

	// function that sets the shader program as the one to use
	void use() const;

	// Set the uniform values of a light.
	void setLight(Light light);
	void setLightPos(const Light& light, const glm::mat4& transform) const;
	void setLightDir(const Light& light, const glm::mat3& dirNormMatrix) const;

	// utility functions to set the values of uniforms
	int getUnif(const std::string name) const;
	void setUnif(const int location, bool value) const;
	void setUnif(const int location, int value) const;
	void setUnif(const int location, unsigned int value) const;
	void setUnif(const int location, float value) const;
	void setUnif(const int location, float xVal, float yVal) const;
	void setUnif(const int location, float xVal, float yVal, float zVal) const;
	void setUnif(const int location, float xVal, float yVal, float zVal, float wVal) const;
	void setUnif(const int location, glm::vec2 vec) const;
	void setUnif(const int location, glm::vec3 vec) const;
	void setUnif(const int location, glm::vec4 vec) const;
	void setUnif(const int location, glm::mat2 mat) const;
	void setUnif(const int location, glm::mat3 mat) const;
	void setUnif(const int location, glm::mat4 mat) const;
	// These versions get the location and set the value, but it is slower to use them in the render loop.
	void setUnifS(const std::string& name, bool value) const;
	void setUnifS(const std::string& name, int value) const;
	void setUnifS(const std::string& name, unsigned int value) const;
	void setUnifS(const std::string& name, float value) const;
	void setUnifS(const std::string& name, float xVal, float yVal) const;
	void setUnifS(const std::string& name, float xVal, float yVal, float zVal) const;
	void setUnifS(const std::string& name, float xVal, float yVal, float zVal, float wVal) const;
	void setUnifS(const std::string& name, glm::vec2 vec) const;
	void setUnifS(const std::string& name, glm::vec3 vec) const;
	void setUnifS(const std::string& name, glm::vec4 vec) const;
	void setUnifS(const std::string& name, glm::mat2 mat) const;
	void setUnifS(const std::string& name, glm::mat3 mat) const;
	void setUnifS(const std::string& name, glm::mat4 mat) const;

private:
	std::map<std::string, std::array<int, 2>> lightIDs;
};

#endif