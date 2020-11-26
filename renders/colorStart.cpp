#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

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
#include "misc_sources.h" // framebuffer size callback and input processing
#include "texture_loader.h" // Utility function for loading textures (generates texture)

// Set number of point lights
const unsigned int NR_POINT_LIGHTS = 4;

const glm::vec3 sunYellow(0.9765f, 0.8431f, 0.1098f);

// Define indices in light arrays
const unsigned int POS_ID = 0;
const unsigned int DIR_ID = 1;
const unsigned int AMB_ID = 2;
const unsigned int DIFF_ID = 3;
const unsigned int SPEC_ID = 4;

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

    // compile and link the shader programs
    Shader sProg("lighting_vert_shader.glsl", "lighting_frag_shader.glsl");
    Shader lProg("light_vert_shader.glsl", "light_frag_shader.glsl");

    // tell stb_image to flip images on load (the image y axis tends to start from the top)
    stbi_set_flip_vertically_on_load(true);
    // Load an image and generate a texture
    unsigned int diffuseMap = loadTexture("container2.png", GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    // Load an image and generate a texture
    unsigned int specularMap = loadTexture("container2_specular.png", GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    // Load an image and generate a texture
    unsigned int emissionMap = loadTexture("matrix.jpg", GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

    // Create a vertex array object and bind it
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Create several vertex buffer object, bind them and give them the necessary data
    const unsigned int POS_NORM_TEX_VB = 0;
    const unsigned int MOD_VB = 1;
    const unsigned int NORM_M_VB = 2;
    unsigned int vbos[3];
    glGenBuffers(3, vbos);
    // Attribute locations (objects)
    const unsigned int POS_LOC = 0;
    const unsigned int NORM_LOC = 1;
    const unsigned int TEX_LOC = 2;
    const unsigned int MOD_LOC = 3;
    const unsigned int NORM_M_LOC = 7;
    // Attribute locations (light sources)
    const unsigned int LPOS_LOC = 0;
    const unsigned int LCOL_LOC = 1;
    const unsigned int LMOD_LOC = 2;

    glBindBuffer(GL_ARRAY_BUFFER, vbos[POS_NORM_TEX_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // Location attribute
    glVertexAttribPointer(POS_LOC, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(POS_LOC);
    // Normal attribute
    glVertexAttribPointer(NORM_LOC, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(NORM_LOC);
    // Texture coordinate attribute
    glVertexAttribPointer(TEX_LOC, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(TEX_LOC);
    // Model matrices (assumes positions don't change, if they do, do this in render loop
    glBindBuffer(GL_ARRAY_BUFFER, vbos[MOD_VB]);
    glm::mat4 models[10];
    for (unsigned int i = 0; i < 10; i++) {
        models[i] = glm::mat4(1.0f);
        models[i] = glm::translate(models[i], cubePositions[i]);
        float angle = 20.0f * i;
        models[i] = glm::rotate(models[i], glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(models), models, GL_STATIC_DRAW);
    for (unsigned int i = 0; i < 4; i++) {
        glVertexAttribPointer(MOD_LOC + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * i));
        glEnableVertexAttribArray(MOD_LOC + i);
        glVertexAttribDivisor(MOD_LOC + i, 1);
    }
    // Normal matrices
    glBindBuffer(GL_ARRAY_BUFFER, vbos[NORM_M_VB]);
    glm::mat3 normMats[10];
    for (unsigned int i = 0; i < 3; i++) {
        glVertexAttribPointer(NORM_M_LOC + i, 3, GL_FLOAT, GL_FALSE, sizeof(glm::mat3), (void*)(sizeof(glm::vec3) * i));
        glEnableVertexAttribArray(NORM_M_LOC + i);
        glVertexAttribDivisor(NORM_M_LOC + i, 1);
    }

    unsigned int lightVAO;
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[POS_NORM_TEX_VB]);
    glVertexAttribPointer(LPOS_LOC, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(LPOS_LOC);

    // Create buffers for light colors and light model matrices
    unsigned int lightVBOs[2];
    glGenBuffers(2, lightVBOs);

    glBindBuffer(GL_ARRAY_BUFFER, lightVBOs[0]);
    glm::vec3 lightColors[NR_POINT_LIGHTS + 1];
    glVertexAttribPointer(LCOL_LOC, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(LCOL_LOC);
    glVertexAttribDivisor(LCOL_LOC, 1);

    glBindBuffer(GL_ARRAY_BUFFER, lightVBOs[1]);
    glm::mat4 lightModels[NR_POINT_LIGHTS + 1];
    for (unsigned int i = 0; i < 4; i++) {
        glVertexAttribPointer(LMOD_LOC + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * i));
        glEnableVertexAttribArray(LMOD_LOC + i);
        glVertexAttribDivisor(LMOD_LOC + i, 1);
    }

    glBindVertexArray(0);

    // Make the model, view and projection matrices
    glm::mat4 view;
    glm::mat4 projection;

    // Get the uniform IDs in the vertex shader
    const int sViewID = sProg.getUnif("view");
    const int sProjID = sProg.getUnif("projection");
    const int lModelID = lProg.getUnif("model");
    const int lViewID = lProg.getUnif("view");
    const int lProjID = lProg.getUnif("projection");

    // Set the material attributes
    sProg.use();
    sProg.setUnifS("material.diffuse", 0);
    sProg.setUnifS("material.specular", 1);
    sProg.setUnifS("material.emission", 2);
    sProg.setUnifS("material.shininess", 0.4f * 128.0f); // power of 2 mostly, the higher the less the reflected light diffuses

    // Set the directional light attributes
    Light dirLight("directionalLight", &sProg, true);

    // Set point light attributes
    Light pointLights[NR_POINT_LIGHTS];
    for (unsigned int i = 0; i < NR_POINT_LIGHTS; i++) {
        std::string unifName = "pointLights[";
        unifName.append(std::to_string(i)).append("]");
        pointLights[i].initVals(unifName, &sProg, false,
            pointLightAttenuationValues[i][0], pointLightAttenuationValues[i][1],
            pointLightAttenuationValues[i][2]);
    }

    // Set flashlight attributes
    float cutOff = glm::cos(glm::radians(10.0f));
    float outerCutOff = glm::cos(glm::radians(20.0f));
    Light flashLight("flashLight", &sProg, false, 1.0f, 0.027f, 0.0028f, cutOff, outerCutOff);

    // Set spotlight attributes
    cutOff = glm::cos(glm::radians(30.5f));
    outerCutOff = glm::cos(glm::radians(50.0f));
    Light spotLight("spotLight", &sProg, false, 1.0f, 0.14f, 0.07f, cutOff, outerCutOff);

    // Define the clear color
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    // Draw in Wireframe mode
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Draw in normal mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Enable the Z-buffer (Depth buffer)
    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) {

        currentFrame = static_cast <float> (glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        // render
        // Clear the color
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update the camera view
        view = cam.GetViewMatrix();
        // Create a matrix to maintain directions in view space
        glm::mat3 dirNormMat(glm::transpose(glm::inverse(view)));
        // Update the projection matrix
        projection = glm::perspective(glm::radians(cam.Zoom), 800.0f / 600.0f, 0.1f, 100.0f);

        // Set the positions of the point lights
        float sinLight = 7.0f * sin(currentFrame / 5.0f);
        float cosLight = 5.0f * cos(currentFrame / 5.0f);
        glm::vec3 pointLightColors[NR_POINT_LIGHTS];
        for (unsigned int i = 0; i < NR_POINT_LIGHTS; i++)
            pointLightColors[i] = glm::vec3(0.0f);

        // Set the position and direction of the spotlight
        glm::vec3 spotLightPos(0.0f, 3.0f, 0.0f);
        glm::vec3 spotLightDir(0.0f, -1.0f, 0.0f);
        // Set the colors of the spot light
        glm::vec3 spotLightColor(1.0f);

        // set the uniform values and use the program
        sProg.use();
        sProg.setUnif(sViewID, view);
        sProg.setUnif(sProjID, projection);
        // Update the normal matrices
        for (unsigned int i = 0; i < 10; i++)
            normMats[i] = glm::mat3(glm::transpose(glm::inverse(view * models[i])));
        glBindBuffer(GL_ARRAY_BUFFER, vbos[NORM_M_VB]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(normMats), normMats, GL_DYNAMIC_DRAW);
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
        spotLight.setColors(glm::vec3(0.7f), 0.2f, 0.45f, 0.7f);
        // bind the appropriate array
        glBindVertexArray(VAO);
        // Bind the texture(s)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, emissionMap);
        // Draw objects
        glDrawArraysInstanced(GL_TRIANGLES, 0, 36, 10);

        // Now do the same for the light source
        lProg.use();
        // Update color vectors
        for (unsigned int i = 0; i < NR_POINT_LIGHTS; i++) {
            lightColors[i] = pointLightColors[i];
        }
        lightColors[NR_POINT_LIGHTS] = spotLightColor;
        glBindBuffer(GL_ARRAY_BUFFER, lightVBOs[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(lightColors), lightColors, GL_DYNAMIC_DRAW);
        // Update model matrices
        for (unsigned int i = 0; i < NR_POINT_LIGHTS; i++) {
            lightModels[i] = glm::mat4(1.0f);
            lightModels[i] = glm::translate(lightModels[i], pointLightPositions[i]);
            lightModels[i] = glm::scale(lightModels[i], glm::vec3(0.2f));
        }
        lightModels[NR_POINT_LIGHTS] = glm::mat4(1.0f);
        lightModels[NR_POINT_LIGHTS] = glm::translate(lightModels[NR_POINT_LIGHTS], spotLightPos);
        lightModels[NR_POINT_LIGHTS] = glm::scale(lightModels[NR_POINT_LIGHTS], glm::vec3(0.2f));
        glBindBuffer(GL_ARRAY_BUFFER, lightVBOs[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(lightModels), lightModels, GL_DYNAMIC_DRAW);
        // we use the same view, projection and normal matrices
        lProg.setUnif(lViewID, view);
        lProg.setUnif(lProjID, projection);
        glBindVertexArray(lightVAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 36, NR_POINT_LIGHTS + 1);

        // buffer swap and event poll
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &lightVAO);
    glDeleteBuffers(3, vbos);
    glDeleteTextures(1, &diffuseMap);
    glDeleteTextures(1, &specularMap);
    glDeleteTextures(1, &emissionMap);
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
    /*
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cam.MovementSpeed /= 2.0f;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE)
        cam.MovementSpeed *= 2.0f;
    */
}