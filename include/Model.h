#ifndef MODEL_H
#define MODEL_H

#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include "Mesh.h"


class Model {
public:
    std::string directory;
    /*  Functions   */
    Model(std::string path, std::vector<std::string> cubeMapPaths = {});
    ~Model();
    void Draw(Shader shader, unsigned int numInstances, glm::mat4* models,
        glm::mat3* normMats);
    void getTextureLocations(Shader shader);
private:
    /*  Model Data  */
    std::vector<Mesh> meshes;
    std::unordered_map<std::string, Texture> loadedTextures;
    Texture cubeTex;
    unsigned int cubeMapID;
    unsigned int cubeMapLoc;
    /*  Functions   */
    void loadModel(std::string path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type,
        std::string typeName);
    Material loadMaterial(aiMaterial* mat);
};

#endif