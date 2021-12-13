#ifndef MODEL_H
#define MODEL_H

#include "Mesh.h"
#include <array>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

class Model {
public:
  std::string directory;
  /*  Functions   */
  Model(std::string path, bool bSRGB = false,
        std::vector<std::string> cubeMapPaths = {});
  ~Model();
  void Draw(const Shader &shader, unsigned int numInstances, glm::mat4 *models,
            glm::mat3 *normMats) const;
  void getTextureLocations(Shader shader);
  float getApproxWidth() const;

private:
  /*  Model Data  */
  float approxWidth;
  bool bSRGB;
  std::array<float, 14> boundingVolumeBounds;
  std::vector<Mesh> meshes;
  std::unordered_map<std::string, Texture> loadedTextures;
  Texture cubeTex;
  unsigned int cubeMapID;
  unsigned int cubeMapLoc;
  /*  Functions   */
  void buildBoundingVolume();
  void approximateWidth();
  void loadModel(std::string path);
  void processNode(aiNode *node, const aiScene *scene);
  Mesh processMesh(aiMesh *mesh, const aiScene *scene);
  std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type,
                                            std::string typeName,
                                            bool srgb = false);
  Material loadMaterial(aiMaterial *mat);
};

#endif