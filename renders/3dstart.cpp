#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

// Stb for image loading
#include <stb-master/stb_image.h>

// Include glm for matrix math
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h" // Camera class
#include "shader.h" // Shader class
#include "misc_sources.h" // framebuffer size callback and input processing

const unsigned int NUM_INSTANCES(10);

// Set initial values for times
float deltaTime(0.0f);
float lastFrame(0.0f);
float currentFrame(0.0f);

// create a pi constant
const double pi = 4 * atan(1);

// Set the initial camera positions
Camera cam(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);

// Set mouse initial position
float lastX = 400, lastY = 300;

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
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell stb_image to flip images on load (the image y axis tends to start from the top)
    stbi_set_flip_vertically_on_load(true);
    // Load an image and generate a texture
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load and generate the texture
    int width, height, nrChannels;
    unsigned char* data = stbi_load("container.jpg", &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cout << "Failed to load texture" << std::endl;
    }
    // IMPORTANT free image data
    stbi_image_free(data);

    // Load a second texture
    unsigned int texture2;
    glGenTextures(1, &texture2);
    glBindTexture(GL_TEXTURE_2D, texture2);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load and generate the texture
    data = stbi_load("awesomeface.png", &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cout << "Failed to load texture" << std::endl;
    }
    // IMPORTANT free image data
    stbi_image_free(data);

    // compile and link the shader program
    Shader sProg("basic_vertex_shader.glsl", "basic_fragment_shader.glsl");

    // Set vertex buffer indices and attribute locations
    //const int INDEX_VB(0);
    const unsigned int POS_TEXCOORD_VB(0);
    const unsigned int MODEL_VB(1);
    const unsigned int POS_LOC(0);
    const unsigned int TEXCOORD_LOC(1);
    const unsigned int MODEL_LOC(2);

    // Create a vertex array object and bind it
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Create several vertex buffer object, bind them and give them the necessary data
    unsigned int vertBuffers[2];
    glGenBuffers(2, vertBuffers);

    glBindBuffer(GL_ARRAY_BUFFER, vertBuffers[POS_TEXCOORD_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // Location attribute
    glVertexAttribPointer(POS_LOC, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(POS_LOC);
    // Texture coordinate attribute
    glVertexAttribPointer(TEXCOORD_LOC, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(TEXCOORD_LOC);

    // Model matrix attribute
    glBindBuffer(GL_ARRAY_BUFFER, vertBuffers[MODEL_VB]);
    for (unsigned int i = 0; i < 4; i++) {
        glVertexAttribPointer(MODEL_LOC + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(4 * static_cast <long long> (i) * sizeof(float)));
        glEnableVertexAttribArray(MODEL_LOC + i);
        glVertexAttribDivisor(MODEL_LOC + i, 1);
    }

    glBindVertexArray(0);

    // tell OpenGL which sampler belongs to which location
    sProg.use();
    sProg.setUnifS("tex1", 0);
    sProg.setUnifS("tex2", 1);

    // Make the model, view and projection matrices
    //glm::mat4 model; // Model matrix is no longer used a uniform due to instanced rendering
    glm::mat4 view;
    glm::mat4 projection;
    //view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
    //view = glm::rotate(view, static_cast <float> (pi) / 6.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    //projection = glm::perspective(glm::radians(fov), 800.0f / 600.0f, 0.1f, 100.0f);

    // Pass the matrices to the shaders
    //const int modelID = sProg.getUnif("model");
    const int viewID = sProg.getUnif("view");
    const int projID = sProg.getUnif("projection");

    // Define the clear color
    glClearColor(0.1f, 0.5f, 0.6f, 1.0f);

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
        // object 1
        // bind the appropriate texture objects
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2);

        // bind and load the model attribute
        glBindBuffer(GL_ARRAY_BUFFER, vertBuffers[MODEL_VB]);
        // Create the model matrices
        glm::mat4 models[NUM_INSTANCES];
        double timeVar = glfwGetTime();
        for (unsigned int i = 0; i < NUM_INSTANCES; i++) {
            models[i] = glm::mat4(1.0f);
            models[i] = glm::translate(models[i], cubePositions[i]);
            float angle = 20.0f * i;
            if (i % 3 == 0 || i == 1) {
                models[i] = glm::rotate(models[i], 0.5f * glm::radians(static_cast <float> (timeVar)),
                    glm::vec3(1.0f, 0.3f, 0.5f));
            }
            else {
                models[i] = glm::rotate(models[i], glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            }
        }

        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * NUM_INSTANCES, models, GL_DYNAMIC_DRAW);

        // set the uniform values and use the program
        sProg.use();
        // bind the appropriate array
        glBindVertexArray(VAO);

        // Update the camera view
        view = cam.GetViewMatrix();
        sProg.setUnif(viewID, view);

        // Update the projection matrix
        projection = glm::perspective(glm::radians(cam.Zoom), 800.0f / 600.0f, 0.1f, 100.0f);
        sProg.setUnif(projID, projection);

        // Draw everything
        glDrawArraysInstanced(GL_TRIANGLES, 0, 36, NUM_INSTANCES);

        // buffer swap and event poll
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(2, vertBuffers);
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    float xoffset = static_cast <float> (xpos) - lastX;
    float yoffset = lastY - static_cast <float> (ypos); // reversed since y-coordinates range from bottom to top
    lastX = static_cast <float> (xpos);
    lastY = static_cast <float> (ypos);

    cam.ProcessMouseMovement(xoffset, yoffset, true);
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
}