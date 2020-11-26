#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <iostream>
#include "Model.h"
#include "texture_loader.h"

using namespace std;

unsigned int aiProcessSteps = aiProcess_Triangulate | aiProcess_FlipUVs |
    aiProcess_GenNormals | aiProcess_OptimizeMeshes;

Model::Model(string path, vector<string> cubeMapPaths) {
    if (!cubeMapPaths.empty()) {
        cubeTex.id = loadCubeMap(cubeMapPaths, false);
        cubeTex.location = 0;
        cubeTex.path = cubeMapPaths[0].substr(0, cubeMapPaths[0].find_first_of('/'));
        cubeTex.type = "cubeMap";
    }
    else {
        cubeTex.id = 0;
    }

    loadModel(path);
}

Model::~Model() {
    for (unsigned int i = 0; i < meshes.size(); i++)
        meshes[i].freeMesh();
}

void Model::Draw(Shader shader, unsigned int numInstances, glm::mat4* models,
    glm::mat3* normMats)
{
    if (cubeMapID != 0) {
        glActiveTexture(GL_TEXTURE0);
        shader.setUnif(cubeMapLoc, 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapID);
    }
    for (unsigned int i = 0; i < meshes.size(); i++)
        meshes[i].Draw(shader, numInstances, models, normMats);
}

void Model::getTextureLocations(Shader shader) {
    cubeMapLoc = shader.getUnif("material.cubeMap0");
    for (unsigned int i = 0; i < meshes.size(); i++)
        meshes[i].getTextureLocations(shader);
}

void Model::loadModel(string path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcessSteps);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode)
    {
        cout << "ERROR::ASSIMP::" << importer.GetErrorString() << endl;
        return;
    }
    directory = path.substr(0, path.find_last_of('/'));

    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene) {
    // process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }
    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    // Data to fill
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;
    Material myMaterial;

    // Copy vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        glm::vec3 auxVert;

        auxVert.x = mesh->mVertices[i].x;
        auxVert.y = mesh->mVertices[i].y;
        auxVert.z = mesh->mVertices[i].z;
        vertex.position = auxVert;

        auxVert.x = mesh->mNormals[i].x;
        auxVert.y = mesh->mNormals[i].y;
        auxVert.z = mesh->mNormals[i].z;
        vertex.normal = auxVert;

        if (mesh->mTextureCoords[0]) {
            vertex.texCoord.x = mesh->mTextureCoords[0][i].x;
            vertex.texCoord.y = mesh->mTextureCoords[0][i].y;
        }
        else
            vertex.texCoord = glm::vec2(0.0f);

        vertices.push_back(vertex);
    }

    // Copy indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // Copy textures
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        vector<Texture> diffuseMaps = loadMaterialTextures(material,
            aiTextureType_DIFFUSE, "diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        vector<Texture> specularMaps = loadMaterialTextures(material,
            aiTextureType_SPECULAR, "specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        vector<Texture> reflexMaps = loadMaterialTextures(material,
            aiTextureType_AMBIENT, "reflex");
        textures.insert(textures.end(), reflexMaps.begin(), reflexMaps.end());
        myMaterial = loadMaterial(material);
    }
    textures.push_back(cubeTex);

    Mesh newMesh(vertices, indices, textures, myMaterial);
    return newMesh;
}

vector<Texture> Model::loadMaterialTextures(aiMaterial* mat,
    aiTextureType type, string typeName)
{
    vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        string path(directory + '/' + string(str.C_Str()));

        unordered_map<string, Texture>::const_iterator iter = loadedTextures.find(path);
        if (iter != loadedTextures.end()) {
            textures.push_back(iter->second);
        }
        else {
            Texture texture;
            texture.id = loadTexture(path);
            texture.location = 0;
            texture.path = path;
            texture.type = typeName;
            textures.push_back(texture);
            loadedTextures[path] = texture;
        }
    }
    return textures;
}

Material Model::loadMaterial(aiMaterial* mat) {
    Material material;
    aiColor3D color(0.f, 0.f, 0.f);
    float shininess;

    mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
    material.Diffuse = glm::vec3(color.r, color.b, color.g);

    mat->Get(AI_MATKEY_COLOR_AMBIENT, color);
    material.Ambient = glm::vec3(color.r, color.b, color.g);

    mat->Get(AI_MATKEY_COLOR_SPECULAR, color);
    material.Specular = glm::vec3(color.r, color.b, color.g);

    mat->Get(AI_MATKEY_COLOR_EMISSIVE, color);
    material.Emissive = glm::vec3(color.r, color.b, color.g);

    mat->Get(AI_MATKEY_SHININESS, shininess);
    material.Shininess = shininess;

    return material;
}