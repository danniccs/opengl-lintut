#ifndef TEXTURE_LOADER_H
#define TEXTURE_LOADER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>

unsigned int loadTexture(std::string path, bool srgb = false, bool flip = true,
    GLenum sWrap = GL_REPEAT, GLenum tWrap = GL_REPEAT,
    GLenum minFilter = GL_LINEAR_MIPMAP_LINEAR, GLenum magFilter = GL_LINEAR);

unsigned int loadCubeMap(std::vector<std::string> paths, bool flip = true,
    GLenum sWrap = GL_CLAMP_TO_EDGE, GLenum tWrap = GL_CLAMP_TO_EDGE,
    GLenum rWrap = GL_CLAMP_TO_EDGE, GLenum minFilter = GL_LINEAR,
    GLenum magFilter = GL_LINEAR);
#endif