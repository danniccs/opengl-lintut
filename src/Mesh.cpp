#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Mesh.h"
#include <iostream>

using namespace std;

Mesh::Mesh(vector<Vertex> vertices, vector<unsigned int> indices,
    vector<Texture> textures, Material materials) : VBOs(), simpId(0)
{
	this->vertices = vertices;
	this->indices = indices;
	this->textures = textures;
    this->material = materials;
	setupMesh();
}

void Mesh::freeMesh() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(NUM_VBS, VBOs);
    for (unsigned int i = 0; i < textures.size(); i++)
        glDeleteTextures(1, &textures[i].id);
}

void Mesh::Draw(Shader shader, unsigned int numInstances, glm::mat4* models,
                glm::mat3* normMats) const
{
    // Populate model matrices
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[MOD_VB]);
    glBufferData(GL_ARRAY_BUFFER, numInstances * sizeof(glm::mat4), models, GL_DYNAMIC_DRAW);
    if (normMats != NULL) {
        glBindBuffer(GL_ARRAY_BUFFER, VBOs[NORM_M_VB]);
        glBufferData(GL_ARRAY_BUFFER, numInstances * sizeof(glm::mat3), normMats, GL_DYNAMIC_DRAW);
    }

    if (!textures.empty()) {
        // Bind textures
        for (unsigned int i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE1 + i);
            shader.setUnif(textures[i].location, i + 1);
            if (textures[i].type != "cubeMap")
                glBindTexture(GL_TEXTURE_2D, textures[i].id);
            else
                glBindTexture(GL_TEXTURE_CUBE_MAP, textures[i].id);
        }
        shader.setUnif(simpId, false);
    }
    else {
        shader.setUnif(simpId, true);
        shader.setUnif(material.ambId, material.Ambient);
        shader.setUnif(material.diffId, material.Diffuse);
        shader.setUnif(material.specId, material.Specular);
        shader.setUnif(material.emisId, material.Emissive);
        shader.setUnif(material.shinId, material.Shininess);
    }

    // draw mesh
    glBindVertexArray(VAO);
    glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(indices.size()),
        GL_UNSIGNED_INT, NULL, numInstances);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
}

void Mesh::getTextureLocations(Shader shader) {
    unsigned int diffuseNr = 0;
    unsigned int specularNr = 0;
    unsigned int reflexNr = 0;
    unsigned int normalNr = 0;
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
            else if (name == "specular") {
                number = to_string(specularNr);
                specularNr++;
            }
            else if (name == "reflex") {
                number = to_string(reflexNr);
                reflexNr++;
            }
            else if (name == "normal") {
                number = to_string(normalNr);
                normalNr++;
            }
            else if (name == "cubeMap") {
                number = to_string(cubeNr);
                cubeNr++;
            }
            textures[i].location = shader.getUnif(mat + name + number);
        }
    }
    else {
        material.ambId = shader.getUnif(mat + "simpleAmbient");
        material.diffId = shader.getUnif(mat + "simpleDiffuse");
        material.specId = shader.getUnif(mat + "simpleSpecular");
        material.emisId = shader.getUnif(mat + "simpleEmissive");
        material.shinId = shader.getUnif(mat + "shininess");
    }
    simpId = shader.getUnif(mat + "isSimple");
}

void Mesh::setupMesh() {
    unsigned int err;
    // Create vertex array object
    glGenVertexArrays(1, &VAO);
    // Create element buffer object
    glGenBuffers(1, &EBO);
    // Create several vertex buffer object
    glGenBuffers(NUM_VBS, VBOs);

    // Bind the objects, tell them how the data is read and bind some data
    glBindVertexArray(VAO);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    while ((err = glGetError()) != GL_NO_ERROR) {
        cout << "Mesh EBO error: " << hex << err << '\n';
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBOs[POS_NORM_TEX_TAN_VB]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // Location attribute
    glEnableVertexAttribArray(POS_LOC);
    glVertexAttribPointer(POS_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    // Normal attribute
    glEnableVertexAttribArray(NORM_LOC);
    glVertexAttribPointer(NORM_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    // Texture coordinate attribute
    glEnableVertexAttribArray(TEX_LOC);
    glVertexAttribPointer(TEX_LOC, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    // Tangent attribute
    glEnableVertexAttribArray(TAN_LOC);
    glVertexAttribPointer(TAN_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
    // Check for errors.
    while ((err = glGetError()) != GL_NO_ERROR) {
        cout << "Mesh VBO error: " << hex << err << '\n';
    }

    // Model matrices
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[MOD_VB]);
    for (unsigned int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(MOD_LOC + i);
        glVertexAttribPointer(MOD_LOC + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * i));
        glVertexAttribDivisor(MOD_LOC + i, 1);
    }
    while ((err = glGetError()) != GL_NO_ERROR) {
        cout << "Mesh model matrix error: " << hex << err << '\n';
    }

    // Normal matrices
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[NORM_M_VB]);
    for (unsigned int i = 0; i < 3; i++) {
        glEnableVertexAttribArray(NORM_M_LOC + i);
        glVertexAttribPointer(NORM_M_LOC + i, 3, GL_FLOAT, GL_FALSE, sizeof(glm::mat3), (void*)(sizeof(glm::vec3) * i));
        glVertexAttribDivisor(NORM_M_LOC + i, 1);
    }
    while ((err = glGetError()) != GL_NO_ERROR) {
        cout << "Mesh normal matrix error: " << hex << err << '\n';
    }

    // Colors if they are supplied.
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[COL_VB]);
    glEnableVertexAttribArray(COL_LOC);
    glVertexAttribPointer(COL_LOC, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
    glVertexAttribDivisor(COL_LOC, 1);
    while ((err = glGetError()) != GL_NO_ERROR) {
        cout << "Mesh color buffer error: " << hex << err << '\n';
    }

    glBindVertexArray(0);
}
