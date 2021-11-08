#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "SimpleMesh.h"
#include "texture_loader.h"

using namespace std;

SimpleMesh::SimpleMesh(const float* vertices, unsigned int numVertices,
    vector<string> texturePaths, bool bSRGB, bool hasNormals,
    vector<GLenum> texParams, vector<string> cubeTexturePaths)
{
    this->numVertices = numVertices;
    this->bSRGB= bSRGB;

    // Create vertex array object
    glGenVertexArrays(1, &VAO);
    // Create several vertex buffer object
    glGenBuffers(SIMP_NUM_VBS, VBOs);

    // Bind the objects, tell them how the data is read and bind some data
    glBindVertexArray(VAO);

    size_t posSize = 3 * sizeof(float);
    size_t normSize = 0;
    size_t coordSize = 0;

    if (hasNormals)
        normSize = 3 * sizeof(float);
    if (!texturePaths.empty())
        coordSize = 2 * sizeof(float);

    GLsizei totalSize = static_cast<GLsizei>(posSize + normSize + coordSize);

    glBindBuffer(GL_ARRAY_BUFFER, VBOs[POS_NORM_TEX_VB]);
    glBufferData(GL_ARRAY_BUFFER, numVertices * static_cast<size_t>(totalSize), vertices, GL_STATIC_DRAW);

    // Location attribute
    glEnableVertexAttribArray(POS_LOC);
    glVertexAttribPointer(POS_LOC, 3, GL_FLOAT, GL_FALSE, totalSize, (void*)0);

    if (hasNormals) {
        // Normal attribute
        glEnableVertexAttribArray(NORM_LOC);
        glVertexAttribPointer(NORM_LOC, 3, GL_FLOAT, GL_FALSE, totalSize, (void*)(posSize));
    }

    if (!texturePaths.empty()) {
        // Texture coordinate attribute
        glEnableVertexAttribArray(TEX_LOC);
        glVertexAttribPointer(TEX_LOC, 2, GL_FLOAT, GL_FALSE, totalSize, (void*)(posSize + normSize));
    }

    // Model matrices
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[MOD_VB]);
    for (unsigned int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(MOD_LOC + i);
        glVertexAttribPointer(MOD_LOC + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * i));
        glVertexAttribDivisor(MOD_LOC + i, 1);
    }

    if (hasNormals) {
        // Normal matrices
        glBindBuffer(GL_ARRAY_BUFFER, VBOs[NORM_M_VB]);
        for (unsigned int i = 0; i < 3; i++) {
            glEnableVertexAttribArray(NORM_M_LOC + i);
            glVertexAttribPointer(NORM_M_LOC + i, 3, GL_FLOAT, GL_FALSE, sizeof(glm::mat3), (void*)(sizeof(glm::vec3) * i));
            glVertexAttribDivisor(NORM_M_LOC + i, 1);
        }
    }

    if (!cubeTexturePaths.empty()) {
        Texture tex = loadCubeMaps(cubeTexturePaths);
        textures.push_back(tex);
    }

    if (!texturePaths.empty()) {
        vector<Texture> texs = loadTextures(texturePaths, texParams);
        textures.insert(textures.end(), texs.begin(), texs.end());
    }

    glBindVertexArray(0);

    unsigned int err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        cout << hex << err << '\n';
    }
}

SimpleMesh::~SimpleMesh() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(SIMP_NUM_VBS, VBOs);
    for (unsigned int i = 0; i < textures.size(); i++)
        glDeleteTextures(1, &textures[i].id);
}

void SimpleMesh::Draw(Shader shader, unsigned int numInstances, glm::mat4* models,
    glm::mat3* normMats)
{
    // Populate model matrices
    if (models != NULL) {
        glBindBuffer(GL_ARRAY_BUFFER, VBOs[MOD_VB]);
        glBufferData(GL_ARRAY_BUFFER, numInstances * sizeof(glm::mat4), models, GL_DYNAMIC_DRAW);
    }

    if (normMats != NULL) {
        glBindBuffer(GL_ARRAY_BUFFER, VBOs[NORM_M_VB]);
        glBufferData(GL_ARRAY_BUFFER, numInstances * sizeof(glm::mat3), normMats, GL_DYNAMIC_DRAW);
    }

    if (!textures.empty()) {
        // Bind textures
        for (unsigned int i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            shader.setUnif(textures[i].location, i);
            if (textures[i].type != "cubeMap")
                glBindTexture(GL_TEXTURE_2D, textures[i].id);
            else
                glBindTexture(GL_TEXTURE_CUBE_MAP, textures[i].id);
        }
    }

    // draw mesh
    glBindVertexArray(VAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, numVertices, numInstances);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
}

void SimpleMesh::getTextureLocations(Shader shader) {
    unsigned int diffuseNr = 0;
    unsigned int specularNr = 0;
    unsigned int cubeNr = 0;
    string mat = "material.";

    // Bind textures
    if (!textures.empty()) {
        for (unsigned int i = 0; i < textures.size(); i++) {
            string name = textures[i].type;
            string number;
            if (name == "diffuse") {
                number = to_string(diffuseNr);
                diffuseNr++;
            }
            else if (name == "cubeMap") {
                number = to_string(cubeNr);
                cubeNr++;
            }
            textures[i].location = shader.getUnif(mat + name + number);
        }
    }
}

vector<Texture> SimpleMesh::loadTextures(vector<string> texturePaths,
    vector<GLenum> texParams)
{
    vector<Texture> textures;
    for (unsigned int i = 0; i < texturePaths.size(); i++) {
        string path;
        path = texturePaths[i];
        Texture texture;
        if (texParams.empty())
            texture.id = loadTexture(path, bSRGB);
        else {
            GLenum sWrap = texParams[1];
            GLenum tWrap = texParams[2];
            GLenum minFilter = texParams[3];
            GLenum magFilter = texParams[4];
            texture.id = loadTexture(path, bSRGB, true, sWrap, tWrap, minFilter, magFilter);
        }
        texture.location = 0;
        texture.path = path;
        texture.type = "diffuse";
        textures.push_back(texture);
    }
    return textures;
}

Texture SimpleMesh::loadCubeMaps(vector<string> texturePaths) {
    Texture texture;
    texture.id = loadCubeMap(texturePaths, false);
    texture.location = 0;
    texture.path = texturePaths[0].substr(0, texturePaths[0].find_first_of('/'));
    texture.type = "cubeMap";
    return texture;
}
