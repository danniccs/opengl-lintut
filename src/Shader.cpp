#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "Shader.h"

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
	initVals(vertexPath, fragmentPath);
}

Shader::Shader() : ID(0) {};

void Shader::initVals(const char* vertexPath, const char* fragmentPath) {
	// 1. Retrieve the vertex and fragment codes from the paths
	std::string vertexCode;
	std::string fragmentCode;
	std::ifstream vShaderFile;
	std::ifstream fShaderFile;
	// ensure ifstream objects can throw exceptions:
	vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try {
		// open files
		vShaderFile.open(vertexPath);
		fShaderFile.open(fragmentPath);
		std::stringstream vShaderStream, fShaderStream;
		// read file's buffer contents into streams
		vShaderStream << vShaderFile.rdbuf();
		fShaderStream << fShaderFile.rdbuf();
		// close file streams
		vShaderFile.close();
		fShaderFile.close();
		// copy stream content into string
		vertexCode = vShaderStream.str();
		fragmentCode = fShaderStream.str();
	}
	catch (std::ifstream::failure e) {
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
	}

	const char* vShaderCode = vertexCode.c_str();
	const char* fShaderCode = fragmentCode.c_str();

	// 2. Compile and link the shaders
	unsigned int vertex, fragment;
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
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	// fragment shader
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fShaderCode, NULL);
	glCompileShader(fragment);
	// check for compile errors
	glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragment, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	// Link them in a shader program
	ID = glCreateProgram();
	glAttachShader(ID, vertex);
	glAttachShader(ID, fragment);
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
}

void Shader::use() {
	glUseProgram(ID);
}

int Shader::getUnif(const std::string name) const {
	return glGetUniformLocation(ID, name.c_str());
}
	
void Shader::setUnif(const int location, bool value) const {
	glUniform1i(location, static_cast <int> (value));
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

void Shader::setUnif(const int location, float xVal, float yVal, float zVal) const {
	glUniform3f(location, xVal, yVal, zVal);
}

void Shader::setUnif(const int location, float xVal, float yVal, float zVal, float wVal) const {
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

void Shader::setUnifS(const std::string name, bool value) const {
	glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::setUnifS(const std::string name, int value) const {
	glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setUnifS(const std::string name, unsigned int value) const {
	glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setUnifS(const std::string name, float value) const {
	glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setUnifS(const std::string name, float xVal, float yVal) const {
	glUniform2f(glGetUniformLocation(ID, name.c_str()), xVal, yVal);
}

void Shader::setUnifS(const std::string name, float xVal, float yVal, float zVal) const {
	glUniform3f(glGetUniformLocation(ID, name.c_str()), xVal, yVal, zVal);
}

void Shader::setUnifS(const std::string name, float xVal, float yVal, float zVal, float wVal) const {
	glUniform4f(glGetUniformLocation(ID, name.c_str()), xVal, yVal, zVal, wVal);
}

void Shader::setUnifS(const std::string name, glm::vec2 vec) const {
	glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(vec));
}

void Shader::setUnifS(const std::string name, glm::vec3 vec) const {
	glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(vec));
}

void Shader::setUnifS(const std::string name, glm::vec4 vec) const {
	glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(vec));
}

void Shader::setUnifS(const std::string name, glm::mat2 mat) const {
	glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setUnifS(const std::string name, glm::mat3 mat) const {
	glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setUnifS(const std::string name, glm::mat4 mat) const {
	glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}
