#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <array>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <vector>

// Include glm for matrix math
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

// Include stb for image loading
#include "stb_image.h"

#include "Camera.h" // Camera class
#include "Light.h"  // Light class
#include "Model.h"  // Model class
#include "Shader.h" // Shader class
#include "SimpleMesh.h"
#include "misc_sources.h" // framebuffer size callback and input processing
#include "texture_loader.h" // Utility function for loading textures (generates texture)

namespace fs = std::filesystem;
fs::path shaderPath(fs::current_path() / "shaders");
fs::path resourcePath(fs::current_path() / "resources");
fs::path pbrTexturePath = resourcePath / "pbr_textures";

const glm::vec3 sunYellow(0.9765f, 0.8431f, 0.1098f);

// Set the initial camera positions
Camera cam(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f));

// Current mouse positions
double lastX(400.0), lastY(300.0);

// Set initial values for times
float deltaTime(0.0f);
float lastFrame(0.0f);
float currentFrame(0.0f);

// Declare callbacks and input processing function
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const unsigned int SHADOW_WIDTH = 1024;
const unsigned int SHADOW_HEIGHT = 1024;
const float SHADOW_MULT = 0.5;
const unsigned int NUM_SEARCH_SAMPLES = 16;
const unsigned int NUM_PCF_SAMPLES = 32;
const unsigned int NUM_SPHERES = 4;

namespace toggles { // Only changed by input processing
bool bKeyPressed = false;
bool nKeyPressed = false;
bool lKeyPressed = false;
bool pKeyPressed = false;
bool tKeyPressed = false;

bool g_showNorms{false};
bool g_wireframe{false};
bool g_areaLights{true};
bool g_showTube{false};
} // namespace toggles

int main() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  // 4 samples for multisampling
  glfwWindowHint(GLFW_SAMPLES, 4);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  GLFWwindow *window =
      glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
  if (!window) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  // GLAD loader to be able to use OpenGL functions (gets function pointers from
  // address)
  if (!gladLoadGL()) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // Check if negative swap interval values are supported, and activate v-sync
  bool supported =
      static_cast<bool>(glfwExtensionSupported("WGL_EXT_swap_control_tear")) ||
      static_cast<bool>(glfwExtensionSupported("GLX_EXT_swap_control_tear"));
  if (supported)
    glfwSwapInterval(-1);
  else
    glfwSwapInterval(1);

  unsigned int err;
  while ((err = glGetError()) != GL_NO_ERROR) {
    std::cout << "GLAD error: " << std::hex << err << '\n';
  }
  std::cout << std::dec;

  // Set the callback functions
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // Capture mouse and listen to it
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPos(window, lastX, lastY);
  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetKeyCallback(window, key_callback);

  // Draw in normal mode
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  // Enable the Z-buffer (Depth buffer) and stencil test
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_STENCIL_TEST);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  // Enable multisampling
  glEnable(GL_MULTISAMPLE);
  // Enable face culling
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  // Enable gamma correction (disabled for HDR).
  // glEnable(GL_FRAMEBUFFER_SRGB);

  // Define lifetime of objects so arrays and buffers are freed before
  // glfwTerminate is called.
  {
    // compile and link the shader programs
    Shader sProg((shaderPath / "object.vs").c_str(),
                 (shaderPath / "object.fs").c_str());
    Shader floorProg((shaderPath / "floor.vs").c_str(),
                     (shaderPath / "object.fs").c_str());
    Shader lightProg((shaderPath / "light_sphere.vs").c_str(),
                     (shaderPath / "light_sphere.fs").c_str());
    Shader shadowProg((shaderPath / "shadow_map.vs").c_str(),
                      (shaderPath / "shadow_map.fs").c_str());

    // Get the uniform IDs in the vertex shader
    const int sViewID = sProg.getUnif("view");
    const int sProjID = sProg.getUnif("projection");
    const int floorViewID = floorProg.getUnif("view");
    const int floorProjID = floorProg.getUnif("projection");
    const int lightViewID = lightProg.getUnif("view");
    const int lightProjID = lightProg.getUnif("projection");

    // Create shadow map generation framebuffer
    unsigned int shadowFBO;
    glGenFramebuffers(1, &shadowFBO);
    // Shadow maps.
    unsigned int shadowMaps[3];
    glGenTextures(3, shadowMaps);
    // Configure the shadow maps.
    float borderColor[]{1.0f, 1.0f, 1.0f, 1.0f};
    glBindTexture(GL_TEXTURE_2D, shadowMaps[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH,
                 SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindTexture(GL_TEXTURE_2D, shadowMaps[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH,
                 SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindTexture(GL_TEXTURE_2D, shadowMaps[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH,
                 SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    // Bind the texture as the depth attachment
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create a texture with random rotations for Poisson disk sampling rotation
    std::array<std::array<glm::vec2, 32>, 32> randomAngles;
    using std::chrono::milliseconds;
    unsigned int seed = static_cast<unsigned int>(
        std::chrono::system_clock::now().time_since_epoch().count());
    std::mt19937 randGenerator(seed);
    std::uniform_real_distribution<float> uniformDistribution(
        0.0f, glm::radians(180.0f));
    for (std::array<glm::vec2, 32> &a : randomAngles) {
      for (glm::vec2 &v : a) {
        float randomAngle = uniformDistribution(randGenerator);
        v = glm::vec2(glm::cos(randomAngle), glm::sin(randomAngle));
      }
    }
    unsigned int randomTexture;
    glGenTextures(1, &randomTexture);
    glBindTexture(GL_TEXTURE_2D, randomTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 32, 32, 0, GL_RG, GL_FLOAT,
                 &randomAngles);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Create a UBO for global shadow information and bind it.
    glm::vec2 shadowTexelSize(1.0f / SHADOW_WIDTH, 1.0f / SHADOW_HEIGHT);
    unsigned int shadowUBO;
    glGenBuffers(1, &shadowUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, shadowUBO);
    size_t UBOSize =
        32 * sizeof(glm::vec4) + sizeof(glm::vec2) + 3 * sizeof(float);
    glBufferData(GL_UNIFORM_BUFFER, UBOSize, NULL, GL_STATIC_DRAW);
    for (unsigned int i = 0; i < 32; ++i)
      glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(glm::vec4),
                      sizeof(glm::vec2), &sources::poissonDisk[i]);
    glBufferSubData(GL_UNIFORM_BUFFER, 32 * sizeof(glm::vec4),
                    sizeof(glm::vec2), &shadowTexelSize);
    glBufferSubData(GL_UNIFORM_BUFFER,
                    32 * sizeof(glm::vec4) + sizeof(glm::vec2), 4,
                    &NUM_SEARCH_SAMPLES);
    glBufferSubData(GL_UNIFORM_BUFFER,
                    32 * sizeof(glm::vec4) + sizeof(glm::vec2) + 4, 4,
                    &NUM_PCF_SAMPLES);
    glBufferSubData(GL_UNIFORM_BUFFER,
                    32 * sizeof(glm::vec4) + sizeof(glm::vec2) + 8, 4,
                    &SHADOW_MULT);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, shadowUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Load Sphere PBR maps.
    unsigned int albedoMaps[NUM_SPHERES];
    unsigned int normalMaps[NUM_SPHERES];
    unsigned int metallicMaps[NUM_SPHERES];
    unsigned int roughnessMaps[NUM_SPHERES];
    unsigned int aoMaps[NUM_SPHERES];
    unsigned int heightMaps[NUM_SPHERES];
    // Rusted iron sphere.
    std::string prefix = "rustediron1-alt2-Unreal-Engine/rustediron2_";
    albedoMaps[0] =
        loadTexture((pbrTexturePath / (prefix + "basecolor.png")).string());
    normalMaps[0] = loadTexture(
        (pbrTexturePath / (prefix + "normal.png")).string(), false, true, true);
    metallicMaps[0] =
        loadTexture((pbrTexturePath / (prefix + "metallic.png")).string());
    roughnessMaps[0] =
        loadTexture((pbrTexturePath / (prefix + "roughness.png")).string());
    aoMaps[0] = loadTexture(
        (pbrTexturePath / "streaky-metal1-ue/streaky-metal1_ao.png").string());
    // Streaky metal sphere.
    prefix = "streaky-metal1-ue/streaky-metal1_";
    albedoMaps[1] =
        loadTexture((pbrTexturePath / (prefix + "albedo.png")).string());
    normalMaps[1] =
        loadTexture((pbrTexturePath / (prefix + "normal-dx.png")).string(),
                    false, true, true);
    metallicMaps[1] =
        loadTexture((pbrTexturePath / (prefix + "metallic.png")).string());
    roughnessMaps[1] =
        loadTexture((pbrTexturePath / (prefix + "roughness.png")).string());
    aoMaps[1] = loadTexture((pbrTexturePath / (prefix + "ao.png")).string());
    // Worn metal sphere.
    prefix = "worn-metal4-ue/worn_metal4_";
    albedoMaps[2] =
        loadTexture((pbrTexturePath / (prefix + "albedo.png")).string());
    normalMaps[2] =
        loadTexture((pbrTexturePath / (prefix + "Normal-dx.png")).string(),
                    false, true, true);
    metallicMaps[2] =
        loadTexture((pbrTexturePath / (prefix + "Metallic.png")).string());
    roughnessMaps[2] =
        loadTexture((pbrTexturePath / (prefix + "Roughness.png")).string());
    aoMaps[2] = loadTexture((pbrTexturePath / (prefix + "ao.png")).string());
    heightMaps[2] =
        loadTexture((pbrTexturePath / (prefix + "Height.png")).string());
    // Gray granite sphere.
    prefix = "gray-granite-flecks-ue/gray-granite-flecks-";
    albedoMaps[3] =
        loadTexture((pbrTexturePath / (prefix + "albedo.png")).string());
    normalMaps[3] =
        loadTexture((pbrTexturePath / (prefix + "Normal-dx.png")).string(),
                    false, true, true);
    metallicMaps[3] =
        loadTexture((pbrTexturePath / (prefix + "Metallic.png")).string());
    roughnessMaps[3] =
        loadTexture((pbrTexturePath / (prefix + "Roughness.png")).string());
    aoMaps[3] = loadTexture((pbrTexturePath / (prefix + "ao.png")).string());

    // Load floor PBR maps.
    prefix = "rich-brown-tile-variation-ue/rich-brown-tile-variation_";
    unsigned int floorAlbedo =
        loadTexture((pbrTexturePath / (prefix + "albedo.png")).string());
    unsigned int floorNormal =
        loadTexture((pbrTexturePath / (prefix + "normal-dx.png")).string(),
                    false, true, true);
    unsigned int floorMetallic =
        loadTexture((pbrTexturePath / (prefix + "metallic.png")).string());
    unsigned int floorRoughness =
        loadTexture((pbrTexturePath / (prefix + "roughness.png")).string());
    unsigned int floorAO =
        loadTexture((pbrTexturePath / (prefix + "ao.png")).string());
    unsigned int floorHeight =
        loadTexture((pbrTexturePath / (prefix + "height.png")).string());

    // Load boulder PBR maps.
    prefix = "sharp-boulder2-bl/sharp-boulder2-";
    unsigned int boulderAlbedo = loadTexture(
        (pbrTexturePath / (prefix + "albedo.png")).string(), false, false);
    unsigned int boulderNormal = loadTexture(
        (pbrTexturePath / (prefix + "normal_ogl.png")).string(), false, false);
    unsigned int boulderMetallic = loadTexture(
        (pbrTexturePath / (prefix + "metallic.png")).string(), false, false);
    unsigned int boulderRoughness = loadTexture(
        (pbrTexturePath / (prefix + "roughness.png")).string(), false, false);
    unsigned int boulderAO = loadTexture(
        (pbrTexturePath / (prefix + "ao.png")).string(), false, false);

    // Load the floor model.
    SimpleMesh floor(sources::quadVertices, 6, std::vector<std::string>(),
                     false, true);
    glm::mat4 floorModel = glm::mat4(1.0f);
    floorModel = glm::translate(floorModel, glm::vec3(0.0f, -0.3f, 0.0f));
    floorModel = glm::rotate(floorModel, -glm::radians(90.0f),
                             glm::vec3(1.0f, 0.0f, 0.0f));
    floorModel = glm::scale(floorModel, glm::vec3(20.0f, 20.0f, 1.0f));
    floorProg.use();
    floorProg.setUnifS("model", floorModel);
    floorProg.setUnifS("normMat",
                       glm::mat3(glm::transpose(glm::inverse(floorModel))));

    // Load the sphere model.
    fs::path spherePath((resourcePath / "sphere2.obj").c_str());
    Model sphere(spherePath, false);
    float wSphere = sphere.getApproxWidth();
    float lightSphereScaling = 0.6f;
    float pbrSphereScaling = 0.4f;
    // Set sphere positions.
    glm::vec3 spherePos[NUM_SPHERES]{
        glm::vec3(0.0f, 2.0f, -1.0f),
        glm::vec3(1.0f, 2.0f, -1.0f),
        glm::vec3(1.0f, 1.0f, -1.0f),
        glm::vec3(0.0f, 1.0f, -1.0f),
    };
    glm::mat4 sphereModelMats[NUM_SPHERES];
    glm::mat3 sphereNormMats[NUM_SPHERES];
    // Set the model matrices for the spheres.
    for (unsigned int i = 0; i < NUM_SPHERES; ++i) {
      sphereModelMats[i] = glm::translate(glm::mat4(1.0f), spherePos[i]);
      sphereModelMats[i] =
          glm::scale(sphereModelMats[i], glm::vec3(pbrSphereScaling));
      sphereNormMats[i] =
          glm::mat3(glm::transpose(glm::inverse(sphereModelMats[i])));
    }

    // Load the boulder model.
    fs::path boulderPath(
        (pbrTexturePath / "sharp-boulder2-bl/sharp-boulder2.obj").c_str());
    Model boulder(boulderPath, false);
    // Set boulder position.
    glm::mat4 boulderModelMat =
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, 0.0f));
    glm::mat3 boulderNormMat =
        glm::mat3(glm::transpose(glm::inverse(boulderModelMat)));

    // Declare the model, view and projection matrices.
    glm::mat4 view;
    glm::mat4 projection;

    // Set the directional light attributes
    Light dirLight("dirLight", true);
    dirLight.cLight = glm::vec3{0.3f, 0.3f, 0.3f};
    dirLight.direction = glm::vec3{3.0f, -4.0f, 0.0f};
    glm::mat4 dirView =
        glm::lookAt(-dirLight.direction, glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 dirProjection =
        glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 50.0f);
    glm::mat4 dirSpaceMat = dirProjection * dirView;

    // Set spotlight attributes.
    float cutOff = -1.0f;
    constexpr float outerRadians = glm::radians(120.0f);
    float outerCutOff = -1.0f;
    Light spotLight("spotLight", false, wSphere * lightSphereScaling, 0.0f,
                    1.0f, 0.14f, 0.07f, cutOff, outerCutOff);
    spotLight.position = glm::vec3(1.0f, 3.0f, 2.0f);
    spotLight.direction = glm::vec3(0.0f, -1.0f, -1.0f);
    spotLight.cLight = glm::vec3(10.0f);
    glm::mat4 lightSphereModel =
        glm::translate(glm::mat4(1.0f), spotLight.position);
    lightSphereModel =
        glm::scale(lightSphereModel, glm::vec3(lightSphereScaling));
    float aspect =
        static_cast<float>(SHADOW_WIDTH) / static_cast<float>(SHADOW_HEIGHT);
    glm::mat4 spotProjection =
        glm::perspective(outerRadians, aspect, 1.0f, 20.0f);
    glm::mat4 spotView = glm::lookAt(spotLight.position,
                                     spotLight.position + spotLight.direction,
                                     glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 spotSpaceMat = spotProjection * spotView;

    // Set tube light attributes.
    float tubeLength = 3.0f;
    float tubeWidth = 0.15f;
    Light tubeLight("tubeLight", false, tubeWidth, tubeLength);
    tubeLight.position = spotLight.position;
    tubeLight.cLight = glm::vec3(10.0f);
    // Define the orientation of the tube light by defining a tangent.
    glm::vec3 tubeTan(1.0f, 0.0f, 0.0f);
    // Find the end points of the tube.
    glm::vec3 tubeP0 = tubeLight.position - tubeLength / 2.0f * tubeTan;
    glm::vec3 tubeP1 = tubeLight.position + tubeLength / 2.0f * tubeTan;
    /*
    glm::mat4 lightTubeModel =
        glm::translate(glm::mat4(1.0f), tubeLight.position);
    */
    glm::mat4 tubeProjection =
        glm::perspective(glm::radians(90.0f), aspect, 1.0f, 20.0f);
    glm::mat4 tubeView = glm::lookAt(tubeLight.position, glm::vec3(0.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 tubeSpaceMat = tubeProjection * tubeView;

    // Set light uniforms in shaders.
    sProg.setLight(dirLight);
    sProg.setLight(spotLight);
    sProg.setLight(tubeLight);
    floorProg.setLight(dirLight);
    floorProg.setLight(spotLight);
    floorProg.setLight(tubeLight);

    // Set indices for textures.
    sProg.use();
    sProg.setUnifS("shadowMap", 0);
    sProg.setUnifS("spotShadowMap", 1);
    sProg.setUnifS("tubeShadowMap", 2);
    sProg.setUnifS("randomAngles", 3);
    sProg.setUnifS("albedoMap", 4);
    sProg.setUnifS("normalMap", 5);
    sProg.setUnifS("metallicMap", 6);
    sProg.setUnifS("roughnessMap", 7);
    sProg.setUnifS("aoMap", 8);
    sProg.setUnifS("heightMap", 9);
    // Set positions and directions for normal mapping.
    sProg.setUnifS("dirLightDir", dirLight.direction);
    sProg.setUnifS("spotLightPos", spotLight.position);
    sProg.setUnifS("spotLightDir", spotLight.direction);
    sProg.setUnifS("tubeLightPos", tubeLight.position);
    sProg.setUnifS("tubeP0", tubeP0);
    sProg.setUnifS("tubeP1", tubeP1);

    floorProg.use();
    floorProg.setUnifS("shadowMap", 0);
    floorProg.setUnifS("spotShadowMap", 1);
    floorProg.setUnifS("tubeShadowMap", 2);
    floorProg.setUnifS("randomAngles", 3);
    floorProg.setUnifS("albedoMap", 4);
    floorProg.setUnifS("normalMap", 5);
    floorProg.setUnifS("metallicMap", 6);
    floorProg.setUnifS("roughnessMap", 7);
    floorProg.setUnifS("aoMap", 8);
    floorProg.setUnifS("heightMap", 9);
    floorProg.setUnifS("dirLightDir", dirLight.direction);
    floorProg.setUnifS("spotLightPos", spotLight.position);
    floorProg.setUnifS("spotLightDir", spotLight.direction);
    floorProg.setUnifS("tubeLightPos", tubeLight.position);
    floorProg.setUnifS("tubeP0", tubeP0);
    floorProg.setUnifS("tubeP1", tubeP1);

    /*
    Set light directions and positions. Should be inside the render loop if
    lights could move around.
    */
    sProg.use();
    sProg.setLightDir(dirLight, glm::mat4(1.0f));
    sProg.setLightPos(spotLight, glm::mat4(1.0f));
    sProg.setLightDir(spotLight, glm::mat4(1.0f));
    sProg.setLightPos(tubeLight, glm::mat4(1.0f));
    floorProg.use();
    floorProg.setLightDir(dirLight, glm::mat4(1.0f));
    floorProg.setLightPos(spotLight, glm::mat4(1.0f));
    floorProg.setLightDir(spotLight, glm::mat4(1.0f));
    floorProg.setLightPos(tubeLight, glm::mat4(1.0f));

    while (!glfwWindowShouldClose(window)) {

      currentFrame = static_cast<float>(glfwGetTime());
      deltaTime = currentFrame - lastFrame;
      lastFrame = currentFrame;

      // Switch between wireframe
      if (toggles::g_wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

      // input
      processInput(window);

      // render
      /*
      Update Matrices
      ---------------
      */
      // Update the camera view
      view = cam.GetViewMatrix();
      // Create a matrix to maintain directions in view space
      glm::mat3 dirNormMat(glm::transpose(glm::inverse(view)));
      // Update the projection matrix
      projection = glm::perspective(glm::radians(cam.Zoom), 800.0f / 600.0f,
                                    0.1f, 100.0f);

      // Do a first pass to obtain the shadow maps
      {
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

        glCullFace(GL_FRONT);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D, shadowMaps[0], 0);
        glClear(GL_DEPTH_BUFFER_BIT);
        shadowProg.use();
        shadowProg.setUnifS("lightSpaceMatrix", dirSpaceMat);

        for (unsigned int i = 0; i < NUM_SPHERES; ++i) {
          shadowProg.setUnifS("model", sphereModelMats[i]);
          sphere.Draw(shadowProg, 1, nullptr, nullptr);
        }
        shadowProg.setUnifS("model", boulderModelMat);
        boulder.Draw(shadowProg, 1, nullptr, nullptr);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D, shadowMaps[1], 0);
        glClear(GL_DEPTH_BUFFER_BIT);
        shadowProg.setUnifS("lightSpaceMatrix", spotSpaceMat);

        for (unsigned int i = 0; i < NUM_SPHERES; ++i) {
          shadowProg.setUnifS("model", sphereModelMats[i]);
          sphere.Draw(shadowProg, 1, nullptr, nullptr);
        }
        shadowProg.setUnifS("model", boulderModelMat);
        boulder.Draw(shadowProg, 1, nullptr, nullptr);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D, shadowMaps[2], 0);
        glClear(GL_DEPTH_BUFFER_BIT);
        shadowProg.setUnifS("lightSpaceMatrix", tubeSpaceMat);

        for (unsigned int i = 0; i < NUM_SPHERES; ++i) {
          shadowProg.setUnifS("model", sphereModelMats[i]);
          sphere.Draw(shadowProg, 1, nullptr, nullptr);
        }
        shadowProg.setUnifS("model", boulderModelMat);
        boulder.Draw(shadowProg, 1, nullptr, nullptr);

        glCullFace(GL_BACK);
      }

      // Second pass.
      {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        // Clear the buffers.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                GL_STENCIL_BUFFER_BIT);

        // Render the floor.
        {
          floorProg.use();
          floorProg.setUnifS("viewPos", cam.Position);
          floorProg.setUnif(floorViewID, view);
          floorProg.setUnif(floorProjID, projection);
          floorProg.setUnifS("dirSpaceMat", dirSpaceMat);
          floorProg.setUnifS("spotSpaceMat", spotSpaceMat);
          floorProg.setUnifS("tubeSpaceMat", tubeSpaceMat);

          // Draw area or point light.
          floorProg.setUnifS("areaLights", toggles::g_areaLights);
          floorProg.setUnifS("showTube", toggles::g_showTube);

          // Floor Maps
          glActiveTexture(GL_TEXTURE0);
          glBindTexture(GL_TEXTURE_2D, shadowMaps[0]);
          glActiveTexture(GL_TEXTURE1);
          glBindTexture(GL_TEXTURE_2D, shadowMaps[1]);
          glActiveTexture(GL_TEXTURE2);
          glBindTexture(GL_TEXTURE_2D, shadowMaps[2]);
          glActiveTexture(GL_TEXTURE3);
          glBindTexture(GL_TEXTURE_2D, randomTexture);
          glActiveTexture(GL_TEXTURE4);
          glBindTexture(GL_TEXTURE_2D, floorAlbedo);
          glActiveTexture(GL_TEXTURE5);
          glBindTexture(GL_TEXTURE_2D, floorNormal);
          glActiveTexture(GL_TEXTURE6);
          glBindTexture(GL_TEXTURE_2D, floorMetallic);
          glActiveTexture(GL_TEXTURE7);
          glBindTexture(GL_TEXTURE_2D, floorRoughness);
          glActiveTexture(GL_TEXTURE8);
          glBindTexture(GL_TEXTURE_2D, floorAO);
          glActiveTexture(GL_TEXTURE9);
          glBindTexture(GL_TEXTURE_2D, floorHeight);

          floor.Draw(floorProg, 1, nullptr, nullptr);
        }

        // Use the program and set all uniform values before draw call.
        {
          sProg.use();
          sProg.setUnifS("viewPos", cam.Position);
          sProg.setUnif(sViewID, view);
          sProg.setUnif(sProjID, projection);
          sProg.setUnifS("dirSpaceMat", dirSpaceMat);
          sProg.setUnifS("spotSpaceMat", spotSpaceMat);
          sProg.setUnifS("tubeSpaceMat", tubeSpaceMat);

          // Draw area or point light.
          sProg.setUnifS("areaLights", toggles::g_areaLights);
          sProg.setUnifS("showTube", toggles::g_showTube);

          glActiveTexture(GL_TEXTURE0);
          glBindTexture(GL_TEXTURE_2D, shadowMaps[0]);
          glActiveTexture(GL_TEXTURE1);
          glBindTexture(GL_TEXTURE_2D, shadowMaps[1]);
          glActiveTexture(GL_TEXTURE2);
          glBindTexture(GL_TEXTURE_2D, shadowMaps[2]);
          glActiveTexture(GL_TEXTURE3);
          glBindTexture(GL_TEXTURE_2D, randomTexture);

          // Draw the spheres.
          for (unsigned int i = 0; i < NUM_SPHERES; ++i) {
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, albedoMaps[i]);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, normalMaps[i]);
            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_2D, metallicMaps[i]);
            glActiveTexture(GL_TEXTURE7);
            glBindTexture(GL_TEXTURE_2D, roughnessMaps[i]);
            glActiveTexture(GL_TEXTURE8);
            glBindTexture(GL_TEXTURE_2D, aoMaps[i]);
            if (i == 2) {
              glActiveTexture(GL_TEXTURE9);
              glBindTexture(GL_TEXTURE_2D, heightMaps[i]);
            }
            // Call the model draw function for the spheres.
            sphere.Draw(sProg, 1, &sphereModelMats[i], &sphereNormMats[i]);
          }

          // Draw the boulder.
          glActiveTexture(GL_TEXTURE4);
          glBindTexture(GL_TEXTURE_2D, boulderAlbedo);
          glActiveTexture(GL_TEXTURE5);
          glBindTexture(GL_TEXTURE_2D, boulderNormal);
          glActiveTexture(GL_TEXTURE6);
          glBindTexture(GL_TEXTURE_2D, boulderMetallic);
          glActiveTexture(GL_TEXTURE7);
          glBindTexture(GL_TEXTURE_2D, boulderRoughness);
          glActiveTexture(GL_TEXTURE8);
          glBindTexture(GL_TEXTURE_2D, boulderAO);
          boulder.Draw(sProg, 1, &boulderModelMat, &boulderNormMat);
        }

        // Draw the lights.
        lightProg.use();
        lightProg.setUnif(lightViewID, view);
        lightProg.setUnif(lightProjID, projection);
        lightProg.setUnifS("model", lightSphereModel);
        lightProg.setUnifS("color", spotLight.cLight);
        sphere.Draw(lightProg, 1, nullptr, nullptr);
      }

      // buffer swap and event poll
      glfwSwapBuffers(window);
      glfwPollEvents();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteTextures(2, shadowMaps);
    glDeleteFramebuffers(1, &shadowFBO);
    glDeleteTextures(1, &randomTexture);
    glDeleteBuffers(1, &shadowUBO);
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
  double xoffset = xpos - lastX;
  double yoffset =
      lastY - ypos; // reversed since y-coordinates range from bottom to top
  lastX = (xpos);
  lastY = (ypos);

  cam.ProcessMouseMovement(static_cast<float>(xoffset),
                           static_cast<float>(yoffset), true);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  cam.ProcessMouseScroll(static_cast<float>(yoffset));
}

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS) {
    cam.MovementSpeed /= 2.0f;
  }
  if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE) {
    cam.MovementSpeed *= 2.0f;
  }
}

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    cam.ProcessKeyboard(Camera_Movement::FORWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    cam.ProcessKeyboard(Camera_Movement::BACKWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    cam.ProcessKeyboard(Camera_Movement::LEFT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    cam.ProcessKeyboard(Camera_Movement::RIGHT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    cam.ProcessKeyboard(Camera_Movement::UP, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
    cam.ProcessKeyboard(Camera_Movement::DOWN, deltaTime);

  if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS && !toggles::nKeyPressed) {
    toggles::g_showNorms = !toggles::g_showNorms;
    toggles::nKeyPressed = true;
  }
  if (glfwGetKey(window, GLFW_KEY_N) == GLFW_RELEASE) {
    toggles::nKeyPressed = false;
  }

  if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !toggles::lKeyPressed) {
    toggles::g_wireframe = !toggles::g_wireframe;
    toggles::lKeyPressed = true;
  }
  if (glfwGetKey(window, GLFW_KEY_L) == GLFW_RELEASE) {
    toggles::lKeyPressed = false;
  }

  if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !toggles::pKeyPressed) {
    toggles::g_areaLights = !toggles::g_areaLights;
    toggles::pKeyPressed = true;
  }
  if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
    toggles::pKeyPressed = false;
  }

  if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !toggles::tKeyPressed) {
    toggles::g_showTube = !toggles::g_showTube;
    toggles::tKeyPressed = true;
  }
  if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) {
    toggles::tKeyPressed = false;
  }
}