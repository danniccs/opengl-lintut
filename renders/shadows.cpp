#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <random>
#include <array>
#include <chrono>

// Include glm for matrix math
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

// Include stb for image loading
#include "stb_image.h"

#include "Camera.h" // Camera class
#include "Shader.h" // Shader class
#include "Light.h" // Light class
#include "Model.h" // Model class
#include "misc_sources.h" // framebuffer size callback and input processing
#include "texture_loader.h" // Utility function for loading textures (generates texture)
#include "SimpleMesh.h"

namespace fs = std::filesystem;
fs::path shaderPath(fs::current_path() / "shaders");

// Set number of point lights and instances
const unsigned int NR_POINT_LIGHTS = 4;
const unsigned int NR_SPOT_LIGHTS = 1;
const unsigned int NR_INSTANCES = 1;

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
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const unsigned int SHADOW_WIDTH = 1024;
const unsigned int SHADOW_HEIGHT = 1024;
const unsigned int gCubeNR = 4;

namespace toggles {  // Only changed by input processing
    bool bKeyPressed = false;
    bool nKeyPressed = false;
    bool lKeyPressed = false;
    bool g_showNorms{ false };
    bool g_blinn{ true };
    bool g_Wireframe{ false };
}

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

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // GLAD loader to be able to use OpenGL functions (gets function pointers from address)
    if (!gladLoadGL()) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Check if negative swap interval values are supported, and activate v-sync
    bool supported = static_cast<bool>(glfwExtensionSupported("WGL_EXT_swap_control_tear")) ||
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
    // Enable gamma correction
    glEnable(GL_FRAMEBUFFER_SRGB);

    // compile and link the shader programs
    const Shader sProg((shaderPath/"nano_shadow_vert.glsl").c_str(),
                       (shaderPath/"nano_shadow_frag.glsl").c_str());
    const Shader floorProg((shaderPath/"floor_shadows_vert.glsl").c_str(),
                           (shaderPath/"floor_shadows_frag.glsl").c_str());
    const Shader lightProg((shaderPath/"cube_light_vert.glsl").c_str(),
                           (shaderPath/"cube_light_frag.glsl").c_str());
    const Shader shadowProg((shaderPath/"shadow_map_vert.glsl").c_str(),
                            (shaderPath/"shadow_map_frag.glsl").c_str());

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
    unsigned int shadowMap[2];
    glGenTextures(2, shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT,
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    float borderColor[]{ 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glBindTexture(GL_TEXTURE_2D, shadowMap[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT,
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    // Bind the texture as the depth attachment
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create a texture with random rotations for Poisson disk sampling rotation
    std::array<std::array<glm::vec2, 32>, 32> randomAngles;
    using std::chrono::milliseconds;
    unsigned int seed = static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count());
    std::mt19937 randGenerator(seed);
    std::uniform_real_distribution<float> uniformDistribution(0.0f, glm::radians(180.0f));
    for (std::array<glm::vec2, 32>& a : randomAngles) {
        for (glm::vec2& v : a) {
            float randomAngle = uniformDistribution(randGenerator);
            v = glm::vec2(glm::cos(randomAngle) * 0.5 + 0.5, glm::sin(randomAngle) * 0.5 + 0.5);
        }
    }
    unsigned int randomTexture;
    glGenTextures(1, &randomTexture);
    glBindTexture(GL_TEXTURE_2D, randomTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 32, 32, 0, GL_RG, GL_FLOAT, &randomAngles);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Floor VAO, texture and model
    std::vector<std::string> floorTexPaths;
    floorTexPaths.push_back("wood.png");
    SimpleMesh floor{ sources::quadVertices, 6, floorTexPaths, true };
    glm::mat4 floorModel = glm::mat4(1.0f);
    floorModel = glm::translate(floorModel, glm::vec3(0.0f, -0.5f, 0.0f));
    floorModel = glm::rotate(floorModel, -glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    floorModel = glm::scale(floorModel, glm::vec3(20.0f, 20.0f, 1.0f));
    floorProg.use();
    floorProg.setUnifS("model", floorModel);
    floorProg.setUnifS("floorTexture", 0);

    // Load the light cube model
    std::vector<std::string> lightCubeTexPaths;
    lightCubeTexPaths.push_back("white_square.png");
    SimpleMesh lightCube{ sources::cubeVertices, 36, lightCubeTexPaths };

    // Load cube model
    std::vector<std::string> cubeTexPaths;
    cubeTexPaths.push_back("white_square.png");
    SimpleMesh cube{ sources::cubeVertices, 36, cubeTexPaths, true};
    cube.getTextureLocations(sProg);
    glm::vec3 cubePositions[] = {
        glm::vec3(1.0f, 0.5f, 0.5f),
        glm::vec3(4.0f, 0.5f, 0.5f),
        glm::vec3(-1.0f, 3.5f, 0.5f),
        glm::vec3(1.0f, 0.5f, -3.0f),
    };
    std::vector<glm::mat4> cubeModels(gCubeNR);
    std::vector<glm::mat3> cubeNormMats(gCubeNR);
    glm::mat4 model;
    for (unsigned int i = 0; i < gCubeNR; ++i) {
        model = glm::translate(glm::mat4(1.0f), cubePositions[i]);
        model = glm::rotate(model, glm::radians(20.0f * i), glm::vec3(1.0f, 0.0f, 0.0f));
        cubeModels[i] = model;
    }

    // Load the nanosuit model
    fs::path nanoPath(fs::current_path()/"nanosuit_reflection/nanosuit.obj");
    Model nanosuit(nanoPath.c_str(), true);
    nanosuit.getTextureLocations(sProg);

    // Declare the model, view and projection matrices
    glm::mat4 view;
    glm::mat4 projection;

    // Set the directional light attributes
    Light dirLight("directionalLight", sProg, true);
    Light floorDirLight("dirLight", floorProg, true);

    // Set spotlight attributes
    float cutOff = glm::cos(glm::radians(70.0f));
    constexpr float outerRadians = glm::radians(120.0f);
    float outerCutOff = glm::cos(outerRadians);
    Light spotLight("spotLight", sProg, false, 1.0f, 0.14f, 0.07f, cutOff, outerCutOff);
    Light floorSpotLight("spotLight", floorProg, false, 1.0f, 0.14f, 0.07f, cutOff, outerCutOff);
    // Set the position of the cube light
    glm::vec3 spotPos{ 1.0f, 3.0f, 2.0f };
    glm::mat4 lightCubeModel = glm::translate(glm::mat4(1.0f), spotPos);
    lightCubeModel = glm::scale(lightCubeModel, glm::vec3(0.5f));

    glm::vec3 dirLightDir{ 3.0f, -4.0f, 0.0f };
    glm::mat4 lightView = glm::lookAt(-dirLightDir,
                                      glm::vec3(0.0f, 0.0f, 0.0f),
                                      glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);
    glm::mat4 dirLightModel = glm::translate(glm::mat4(1.0f), -dirLightDir * 2.0f);

    float aspect = static_cast<float>(SHADOW_WIDTH) / static_cast<float>(SHADOW_HEIGHT);
    glm::mat4 spotProjection = glm::perspective(outerRadians, aspect, 1.0f, 20.0f);
    //glm::mat4 spotProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);

    sProg.use();
    sProg.setUnifS("shadowMap", 4);
    sProg.setUnifS("spotShadowMap", 5);
    floorProg.use();
    floorProg.setUnifS("shadowMap", 1);
    floorProg.setUnifS("spotShadowMap", 2);

    while (!glfwWindowShouldClose(window)) {

        currentFrame = static_cast <float> (glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Switch between wireframe
        if (toggles::g_Wireframe)
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
        projection = glm::perspective(glm::radians(cam.Zoom), 800.0f / 600.0f, 0.1f, 50.0f);

        // Set the attributes of the spotlight
        glm::vec3 spotLightDir(1.0f, -1.0f, -1.0f);
        glm::vec3 spotLightColor(1.0f, 1.0f, 1.0f);
        // Set the attributes of the directional light
        glm::vec3 dirLightColor{ 0.5f, 0.5f, 0.5f };

        glm::mat4 spotView = glm::lookAt(spotPos,
                                         spotPos + spotLightDir,
                                         glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 spotSpaceMat = spotProjection * spotView;

        glm::mat4 nanoMod = glm::mat4(1.0f);
        nanoMod = glm::translate(nanoMod, glm::vec3(3.0f, -0.52f, 1.0f));
        nanoMod = glm::scale(nanoMod, glm::vec3(0.2f));
        glm::mat3 nanoNorm = glm::mat3(glm::transpose(glm::inverse(view * nanoMod)));

        glm::mat4 lightSpaceMat = lightProjection * lightView;

        // Do a first pass to obtain the shadow map
        {
            glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
            glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
            //glEnable(GL_DEPTH_CLAMP); // Remember to disable after rendering shadow maps

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap[0], 0);
            glClear(GL_DEPTH_BUFFER_BIT);
            shadowProg.use();
            shadowProg.setUnifS("lightSpaceMatrix", lightSpaceMat);

            shadowProg.setUnifS("model", floorModel);
            floor.Draw(shadowProg, 1, nullptr, nullptr);

            glCullFace(GL_FRONT);
            shadowProg.setUnifS("model", nanoMod);
            nanosuit.Draw(shadowProg, 1, nullptr, nullptr);

            for (unsigned int i = 0; i < gCubeNR; ++i) {
                shadowProg.setUnifS("model", cubeModels[i]);
                cube.Draw(shadowProg, 1, nullptr, nullptr);
            }
            glCullFace(GL_BACK);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap[1], 0);
            glClear(GL_DEPTH_BUFFER_BIT);
            shadowProg.setUnifS("lightSpaceMatrix", spotSpaceMat);

            shadowProg.setUnifS("model", floorModel);
            floor.Draw(shadowProg, 1, nullptr, nullptr);

            glCullFace(GL_FRONT);

            shadowProg.setUnifS("model", nanoMod);
            nanosuit.Draw(shadowProg, 1, nullptr, nullptr);

            for (unsigned int i = 0; i < gCubeNR; ++i) {
                shadowProg.setUnifS("model", cubeModels[i]);
                cube.Draw(shadowProg, 1, nullptr, nullptr);
            }
            glCullFace(GL_BACK);
        }

        // Second pass
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
            // Clear the color
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            // Enable gamma correction. This should only be enabled before drawing to the display framebuffer
            glEnable(GL_FRAMEBUFFER_SRGB);

            // Render the floor
            floorProg.use();
            floorProg.setUnif(floorViewID, view);
            floorProg.setUnif(floorProjID, projection);
            floorProg.setUnifS("lightSpaceMat", lightSpaceMat);
            floorProg.setUnifS("spotSpaceMat", spotSpaceMat);
            floorProg.setUnifS("normMat", glm::mat3(glm::transpose(glm::inverse(floorModel))));
            floorProg.setUnifS("viewPos", cam.Position);
            // floor spot light
            floorSpotLight.setPos(spotPos, glm::mat4(1.0f));
            floorSpotLight.setDir(spotLightDir, glm::mat4(1.0f));
            floorSpotLight.setColors(spotLightColor, 0.2f, 0.45f, 0.7f);
            // floor directional light
            floorDirLight.setDir(dirLightDir, glm::mat4(1.0f));
            floorDirLight.setColors(dirLightColor, 0.2f, 0.45f, 0.7f);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, shadowMap[0]);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, shadowMap[1]);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, randomTexture);
            floor.Draw(floorProg, 1, nullptr, nullptr);

            // use the program and set all uniform values before draw call
            sProg.use();
            sProg.setUnif(sViewID, view);
            sProg.setUnif(sProjID, projection);
            //sProg.setUnifS("viewPos", cam.Position);
            sProg.setUnifS("lightSpaceMat", lightSpaceMat);
            sProg.setUnifS("spotSpaceMat", spotSpaceMat);
            // Set directional light coordinates and color
            dirLight.setDir(dirLightDir, dirNormMat);
            dirLight.setColors(dirLightColor, 0.2f, 0.45f, 0.7f);
            // Set spotlight colors and position/direction
            spotLight.setPos(spotPos, view);
            spotLight.setDir(spotLightDir, dirNormMat);
            spotLight.setColors(spotLightColor, 0.2f, 0.45f, 0.7f);
            // Update the model and normal matrices for nanosuits
            
            // Call the model draw function
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, shadowMap[0]);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, shadowMap[1]);
            nanosuit.Draw(sProg, NR_INSTANCES, &nanoMod, &nanoNorm);

            // Draw the cubes
            for (unsigned int i = 0; i < gCubeNR; ++i)
                cubeNormMats[i] = glm::mat3(glm::transpose(glm::inverse(view * cubeModels[i])));
            cube.Draw(sProg, gCubeNR, cubeModels.data(), cubeNormMats.data());

            // Draw the lights
            lightProg.use();
            lightProg.setUnif(lightViewID, view);
            lightProg.setUnif(lightProjID, projection);
            lightProg.setUnifS("model", lightCubeModel);
            lightProg.setUnifS("color", spotLightColor);
            //lightCube.Draw(lightProg, 1, nullptr, nullptr);
            lightProg.setUnifS("model", dirLightModel);
            //lightCube.Draw(lightProg, 1, nullptr, nullptr);
        }

        // buffer swap and event poll
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    double xoffset = xpos - lastX;
    double yoffset = lastY - ypos; // reversed since y-coordinates range from bottom to top
    lastX = (xpos);
    lastY = (ypos);

    cam.ProcessMouseMovement(static_cast <float> (xoffset), static_cast <float> (yoffset), true);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    cam.ProcessMouseScroll(static_cast <float> (yoffset));
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS) {
        cam.MovementSpeed /= 2.0f;
    }
    if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE) {
        cam.MovementSpeed *= 2.0f;
    }
}

void processInput(GLFWwindow* window) {
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
        toggles::g_Wireframe = !toggles::g_Wireframe;
        toggles::lKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_RELEASE) {
        toggles::lKeyPressed = false;
    }
}
