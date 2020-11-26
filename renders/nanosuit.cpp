#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <filesystem>
#include <vector>

// Include glm for matrix math
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

// Include stb for image loading
#include <stb-master/stb_image.h>

#include "Camera.h" // Camera class
#include "Shader.h" // Shader class
#include "Light.h" // Light class
#include "Model.h" // Model class
#include "misc_sources.h" // framebuffer size callback and input processing
#include "texture_loader.h" // Utility function for loading textures (generates texture)

namespace fs = std::filesystem;

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

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); (for Mac)

    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // GLAD loader to be able to use OpenGL functions (gets function pointers from address)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Set the callback functions
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Capture mouse and listen to it
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(window, lastX, lastY);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // Define the clear color
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    // Draw in Wireframe mode
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    // Draw in normal mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    // Enable the Z-buffer (Depth buffer) and stencil test
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    // compile and link the shader programs
    Shader sProg("lighting_vert_shader.glsl", "lighting_frag_shader.glsl");
    Shader lProg("light_vert_shader.glsl", "light_frag_shader.glsl");
    Shader simpleShaders("lighting_vert_shader.glsl", "simple_frag_shader.glsl");

    // Load the models
    Model nanosuit("C:/Users/Bobberson/Downloads/OpenGL_Files/Models/nanosuit/nanosuit.obj");
    nanosuit.getTextureLocations(sProg);
    glm::mat3 normMats[NR_INSTANCES];
    glm::mat4 modelMats[NR_INSTANCES];

    Model bulb("C:/Users/Bobberson/Downloads/OpenGL_Files/Models/ESLamp_obj/ESLamp.obj");
    bulb.getTextureLocations(lProg);
    glm::mat4 bulbMods[NR_POINT_LIGHTS + NR_SPOT_LIGHTS];

    // Make the model, view and projection matrices
    glm::mat4 view;
    glm::mat4 projection;

    // Get the uniform IDs in the vertex shader
    const int sViewID = sProg.getUnif("view");
    const int sProjID = sProg.getUnif("projection");
    const int simViewID = simpleShaders.getUnif("view");
    const int simProjID = simpleShaders.getUnif("projection");
    const int lViewID = lProg.getUnif("view");
    const int lProjID = lProg.getUnif("projection");

    // Set the directional light attributes
    Light dirLight("directionalLight", sProg, true);

    // Set point light attributes
    Light pointLights[NR_POINT_LIGHTS];
    for (unsigned int i = 0; i < NR_POINT_LIGHTS; i++) {
        std::string unifName = "pointLights[";
        unifName.append(std::to_string(i)).append("]");
        pointLights[i].initVals(unifName, sProg, false,
            pointLightAttenuationValues[i][0], pointLightAttenuationValues[i][1],
            pointLightAttenuationValues[i][2]);
    }

    // Set flashlight attributes
    float cutOff = glm::cos(glm::radians(10.0f));
    float outerCutOff = glm::cos(glm::radians(20.0f));
    Light flashLight("flashLight", sProg, false, 1.0f, 0.027f, 0.0028f, cutOff, outerCutOff);

    // Set spotlight attributes
    cutOff = glm::cos(glm::radians(30.5f));
    outerCutOff = glm::cos(glm::radians(50.0f));
    Light spotLight("spotLight", sProg, false, 1.0f, 0.14f, 0.07f, cutOff, outerCutOff);

    while (!glfwWindowShouldClose(window)) {

        currentFrame = static_cast <float> (glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        // render
        // Clear the color
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        /*
        Update Matrices
        ---------------
        */
        // Update the camera view
        view = cam.GetViewMatrix();
        // Create a matrix to maintain directions in view space
        glm::mat3 dirNormMat(glm::transpose(glm::inverse(view)));
        // Update the projection matrix
        projection = glm::perspective(glm::radians(cam.Zoom), 800.0f / 600.0f, 0.1f, 100.0f);

        // Set the positions of the point lights
        glm::vec3 pointLightColors[NR_POINT_LIGHTS];
        for (unsigned int i = 0; i < NR_POINT_LIGHTS; i++)
            pointLightColors[i] = glm::vec3(0.0f);

        // Set the position and direction of the spotlight
        glm::vec3 spotLightPos(0.0f, 3.8f, 0.0f);
        glm::vec3 spotLightDir(0.0f, -1.0f, 0.0f);
        // Set the colors of the spot light
        glm::vec3 spotLightColor(0.0f);

        // use the program and set all uniform values before draw call
        sProg.use();
        sProg.setUnif(sViewID, view);
        sProg.setUnif(sProjID, projection);
        // Set directional light coordinates and color
        dirLight.setDir(glm::vec3(-0.4f, -1.0f, 0.0f), dirNormMat);
        dirLight.setColors(glm::vec3(0.0f), 0.2f, 0.5f, 0.8f);
        // Set point light attributes
        for (unsigned int i = 0; i < NR_POINT_LIGHTS; i++) {
            pointLights[i].setPos(pointLightPositions[i], view);
            pointLights[i].setColors(pointLightColors[i], 0.2f, 0.5f, 0.8f);
        }
        // Set flashlight colors and position/direction
        flashLight.setPos(cam.Position, view);
        flashLight.setDir(cam.Front, dirNormMat);
        flashLight.setColors(glm::vec3(1.0f), 0.0f, 0.5f, 0.85f);
        // Set spotlight colors and position/direction
        spotLight.setPos(spotLightPos, view);
        spotLight.setDir(spotLightDir, dirNormMat);
        spotLight.setColors(spotLightColor, 0.2f, 0.45f, 0.7f);
        // Update the model and normal matrices for nanosuits
        for (unsigned int i = 0; i < NR_INSTANCES; i++) {
            modelMats[i] = glm::mat4(1.0f);
            //modelMats[i] = glm::translate(modelMats[i], glm::vec3(2.0f * static_cast<float>(i) + 1.0f, 0.0f, i));
            modelMats[i] = glm::scale(modelMats[i], glm::vec3(0.2f));
            normMats[i] = glm::mat3(glm::transpose(glm::inverse(view * modelMats[i])));
        }

        glEnable(GL_STENCIL_TEST);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glStencilFunc(GL_ALWAYS, 1, 0xFF); // all fragments should update the stencil buffer
        glStencilMask(0xFF); // enable writing to the stencil buffer
        // Call the model draw function
        nanosuit.Draw(sProg, NR_INSTANCES, modelMats, normMats);
        /*
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);
        simpleShaders.use();
        simpleShaders.setUnif(simViewID, view);
        simpleShaders.setUnif(simProjID, projection);
        glDisable(GL_DEPTH_TEST);
        for (unsigned int i = 0; i < NR_INSTANCES; i++) {
            modelMats[i] = glm::scale(modelMats[i], glm::vec3(1.01f));
            normMats[i] = glm::mat3(glm::transpose(glm::inverse(view * modelMats[i])));
        }
        nanosuit.Draw(simpleShaders, NR_INSTANCES, modelMats, normMats);
        glStencilMask(0xFF);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        */

        // Now do the same for the light sources
        lProg.use();
        // we use the same view, projection and normal matrices
        lProg.setUnif(lViewID, view);
        lProg.setUnif(lProjID, projection);
        // Update color vectors
        bulb.meshes[2].material.Emissive = spotLightColor;
        // Update model matrices
        for (unsigned int i = 0; i < NR_POINT_LIGHTS; i++) {
            bulbMods[i] = glm::mat4(1.0f);
            bulbMods[i] = glm::translate(bulbMods[i], pointLightPositions[i]);
            bulbMods[i] = glm::rotate(bulbMods[i], glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            bulbMods[i] = glm::scale(bulbMods[i], glm::vec3(0.05f));
        }
        bulbMods[NR_POINT_LIGHTS] = glm::mat4(1.0f);
        bulbMods[NR_POINT_LIGHTS] = glm::translate(bulbMods[NR_POINT_LIGHTS], spotLightPos);
        bulbMods[NR_POINT_LIGHTS] = glm::rotate(bulbMods[NR_POINT_LIGHTS], glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        bulbMods[NR_POINT_LIGHTS] = glm::scale(bulbMods[NR_POINT_LIGHTS], glm::vec3(0.05f));
        bulb.Draw(lProg, NR_POINT_LIGHTS + NR_SPOT_LIGHTS, bulbMods, NULL);

        // buffer swap and event poll
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    nanosuit.freeModel();
    bulb.freeModel();
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
}