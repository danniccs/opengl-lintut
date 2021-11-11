#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include "stb_image.h"
#include "texture_loader.h"

using namespace std;

// utility function for loading a 2D texture from file (generates the texture)
unsigned int loadTexture(string path, bool srgb, bool flip, GLenum sWrap,
    GLenum tWrap, GLenum minFilter, GLenum magFilter)
{
    // tell stb_image to flip images on load (the image y axis tends to start from the top)
    stbi_set_flip_vertically_on_load(flip);

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        GLenum iformat = GL_RGB;
        GLenum eformat = GL_RGB;
        if (nrComponents == 1)
            eformat = GL_RED;
        else if (nrComponents == 3)
            eformat = GL_RGB;
        else if (nrComponents == 4)
            eformat = GL_RGBA;

        // override the format with a user-defined format
        if (srgb) {
            if (nrComponents == 3)
                iformat = GL_SRGB;
            else if (nrComponents == 4)
                iformat = GL_SRGB_ALPHA;
        }
        else {
            if (nrComponents == 3)
                iformat = GL_RGB;
            else if (nrComponents == 4)
                iformat = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, iformat, width, height, 0, eformat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sWrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tWrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    }
    else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
    }
    stbi_image_free(data);

    return textureID;
}

unsigned int loadCubeMap(vector<string> paths, bool flip, GLenum sWrap,
    GLenum tWrap, GLenum rWrap, GLenum minFilter, GLenum magFilter)
{
    // tell stb_image to flip images on load (the image y axis tends to start from the top)
    stbi_set_flip_vertically_on_load(flip);

    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrComponents;
    for (unsigned int i = 0; i < paths.size(); i++) {
        unsigned char* data = stbi_load(paths[i].c_str(), &width, &height, &nrComponents, 0);
        if (data) {
            GLenum format = GL_RGB;
            if (nrComponents == 1)
                format = GL_RED;
            if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width,
                height, 0, format, GL_UNSIGNED_BYTE, data);
        }
        else {
            std::cout << "Texture failed to load at path: " << paths[i] << std::endl;
        }
        stbi_image_free(data);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, sWrap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, tWrap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, rWrap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, magFilter);

    return textureID;
}
