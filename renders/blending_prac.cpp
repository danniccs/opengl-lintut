#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <map>

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
#include "SimpleMesh.h" // Simple mesh
#include "misc_sources.h" // framebuffer size callback and input processing
#include "texture_loader.h" // Utility function for loading textures (generates texture)

// Matrix uniform block binding point
GLuint MAT_BINDING_POINT = 2;

// Define numbers of objects/lights
const unsigned int NR_CUBES = 2;
const unsigned int NR_GRASS = 5;

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

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* userParam);

using namespace std;


int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
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

    // During init, enable debug output
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE,
        GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

    // Draw in normal mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    // Draw in Wireframe mode
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    // Enable the Z-buffer (Depth buffer) and stencil test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    //glEnable(GL_STENCIL_TEST);
    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Enable face culling
    //glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    /*
    Framebuffer Generation
    ----------------------
    */
    // Generate and bind a framebuffer object
    unsigned int fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Generate a texture, reserving the space but not filling it
    unsigned int colBuffTexture;
    glGenTextures(1, &colBuffTexture);
    glBindTexture(GL_TEXTURE_2D, colBuffTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Attach the texture to the framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colBuffTexture, 0);

    // Generate a renderbuffer object and bind it
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    // Reserve space for the renderbuffer to use for the stencil and depth buffers
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 800, 600);
    // Attach the renderbuffer to the framebuffer
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    // Check if the framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "Framebuffer status error:" << glCheckFramebufferStatus(GL_FRAMEBUFFER) << '\n';
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // compile and link the shader programs
    Shader simpProg("simple_mesh_vert_shader.glsl", "simple_mesh_frag_shader.glsl");
    Shader reflexProg("reflex_vert.glsl", "reflex_frag.glsl");
    Shader modelReflexProg("model_reflex_vert.glsl", "model_reflex_frag.glsl");
    Shader basProg("basic_vertex_shader.glsl", "basic_fragment_shader.glsl");
    Shader skyProg("sky_vertex_shader.glsl", "sky_fragment_shader.glsl");

    GLuint matShaderIDs[4] = { // IDs of the shaders that use the matrix uniform block
        simpProg.ID,
        reflexProg.ID,
        modelReflexProg.ID,
        skyProg.ID,
    };

    // Load the models
    vector<string> texPath = { "marble.jpg:diffuse" };
    SimpleMesh floor(planeVertices, 6, texPath, false);
    floor.getTextureLocations(simpProg);
    glm::mat4 floorModel;

    texPath = { "metal.png:diffuse" };
    vector<string> cubeTexPaths = {
        "skybox/right.jpg",
        "skybox/left.jpg",
        "skybox/top.jpg",
        "skybox/bottom.jpg",
        "skybox/front.jpg",
        "skybox/back.jpg"
    };
    SimpleMesh cube(cubeVertices, 36, texPath, true, {}, cubeTexPaths);
    cube.getTextureLocations(reflexProg);
    glm::mat4* cubeModels = new glm::mat4[NR_CUBES];
    glm::mat3* cubeNormMats = new glm::mat3[NR_CUBES];

    SimpleMesh skybox(skyboxVertices, 36, {}, false, {}, cubeTexPaths);
    skybox.getTextureLocations(skyProg);
    glm::mat4 skyModel(1.0f);

    string nanoPath = string("C:/Users/Bobberson/Downloads/OpenGL_Files/") + "Models/nanosuit_reflection/nanosuit.obj";
    Model nanoSuit(nanoPath, cubeTexPaths);
    nanoSuit.getTextureLocations(modelReflexProg);
    glm::vec3 nanoPosition(2.0f, 0.0f, -3.0f);
    glm::mat4 nanoModel(1.0f);
    glm::mat3 nanoNormMat(1.0f);

    unsigned int windowVAO;
    unsigned int windowVBOs[2];
    glGenVertexArrays(1, &windowVAO);
    glGenBuffers(2, windowVBOs);
    glBindVertexArray(windowVAO);
    glBindBuffer(GL_ARRAY_BUFFER, windowVBOs[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, windowVBOs[1]);
    for (unsigned int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(4 + i);
        glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, 0, (void*)(i * sizeof(glm::vec4)));
        glVertexAttribDivisor(4 + i, 1);
    }
    glBindVertexArray(0);
    glm::mat4* windowModel = new glm::mat4;
    unsigned int texID = loadTexture("blending_transparent_window.png", true, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    unsigned int texLoc = simpProg.getUnif("material.diffuse0");

    // Quad for post-processing
    unsigned int postVAO;
    unsigned int postVBO;
    glGenVertexArrays(1, &postVAO);
    glGenBuffers(1, &postVBO);
    glBindVertexArray(postVAO);
    glBindBuffer(GL_ARRAY_BUFFER, postVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screenQuadVertices), screenQuadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    // Declare the model, view and projection matrices
    glm::mat4 view;
    glm::mat4 skyView;
    glm::mat4 projection;

    // Create uniform buffer object, bind it and uniform blocks and get uniform block information
    unsigned int matUBO;
    glGenBuffers(1, &matUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, matUBO);
    glBufferData(GL_UNIFORM_BUFFER, 3 * sizeof(glm::mat4), NULL, GL_DYNAMIC_DRAW);
    // Bind the buffer to the biding point
    glBindBufferBase(GL_UNIFORM_BUFFER, MAT_BINDING_POINT, matUBO);
    // Bind the uniform blocks to the binding point
    GLuint matUBID = 0;
    for (unsigned int i = 0; i < 4; i++) {
        matUBID = glGetProgramResourceIndex(matShaderIDs[i], GL_UNIFORM_BLOCK, "Matrices");
        glUniformBlockBinding(matShaderIDs[i], matUBID, MAT_BINDING_POINT);
    }
    // Get the offsets necessary for filling the buffer
    GLenum prop;
    GLsizei length;
    GLint numVars;
    prop = GL_NUM_ACTIVE_VARIABLES;
    glGetProgramResourceiv(matShaderIDs[3], GL_UNIFORM_BLOCK, matUBID, 1, &prop, 24, &length, &numVars);
    GLint* variableOffsets = new GLint[numVars];
    GLint* variableIndices = new GLint[numVars];
    map<string, GLuint> blockVarIndices;
    prop = GL_ACTIVE_VARIABLES;
    glGetProgramResourceiv(matShaderIDs[3], GL_UNIFORM_BLOCK, matUBID, 1, &prop, 24, &length, variableIndices);
    prop = GL_OFFSET;
    for (int i = 0; i < numVars; i++) {
        GLchar varName[100];
        glGetProgramResourceiv(matShaderIDs[3], GL_UNIFORM, variableIndices[i], 1, &prop, numVars, &length, variableOffsets + i);
        glGetProgramResourceName(matShaderIDs[3], GL_UNIFORM, variableIndices[i], 100, nullptr, varName);
        blockVarIndices[varName] = i;
    }
    delete[numVars] variableIndices;

    // Get the uniform IDs in the vertex shader
    const int camPosID = reflexProg.getUnif("cameraPos");
    const int modelCamPosID = modelReflexProg.getUnif("cameraPos");
    const int skyViewID = skyProg.getUnif("skyView");

    while (!glfwWindowShouldClose(window)) {

        currentFrame = static_cast <float> (glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        // render to my framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glEnable(GL_DEPTH_TEST);
        // Clear the color
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /*
        Update Matrices
        ---------------
        */
        // Update the camera view
        view = cam.GetViewMatrix();
        skyView = glm::mat4(glm::mat3(view));
        // Update the projection matrix
        projection = glm::perspective(glm::radians(cam.Zoom), 800.0f / 600.0f, 0.1f, 100.0f);
        // Insert view and projection values into matrix uniform buffer object
        glBindBuffer(GL_UNIFORM_BUFFER, matUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, variableOffsets[blockVarIndices["view"]], sizeof(glm::mat4), &view);
        glBufferSubData(GL_UNIFORM_BUFFER, variableOffsets[blockVarIndices["projection"]], sizeof(glm::mat4), &projection);

        // use the program and set all uniform values before draw call

        reflexProg.use();
        reflexProg.setUnif(camPosID, cam.Position);
        for (unsigned int i = 0; i < NR_CUBES; i++) {
            cubeModels[i] = glm::mat4(1.0f);
            cubeModels[i] = glm::translate(cubeModels[i], cubePositions[i]);
            cubeNormMats[i] = glm::mat3(glm::transpose(glm::inverse(cubeModels[i])));
        }
        glEnable(GL_CULL_FACE);
        cube.Draw(simpProg, NR_CUBES, cubeModels, cubeNormMats);
        glDisable(GL_CULL_FACE);

        modelReflexProg.use();
        modelReflexProg.setUnif(modelCamPosID, cam.Position);
        nanoModel = glm::mat4(1.0f);
        nanoModel = glm::translate(nanoModel, nanoPosition);
        nanoModel = glm::scale(nanoModel, glm::vec3(0.2f));
        nanoNormMat = glm::mat3(glm::transpose(glm::inverse(nanoModel)));
        nanoSuit.Draw(modelReflexProg, 1, &nanoModel, &nanoNormMat);

        /*
        simpProg.use();
        floorModel = glm::mat4(1.0f);
        floor.Draw(simpProg, 1, &floorModel, NULL);

        map<float, glm::vec3> sorted;
        for (unsigned int i = 0; i < NR_GRASS; i++) {
            float distance = glm::length(cam.Position - grassCoordinates[i]);
            sorted[distance] = grassCoordinates[i];
        }
        map<float, glm::vec3>::reverse_iterator iter;
        for (iter = sorted.rbegin(); iter != sorted.rend(); iter++) {
            *windowModel = glm::mat4(1.0f);
            *windowModel = glm::translate(*windowModel, iter->second);
            glBindBuffer(GL_ARRAY_BUFFER, windowVBOs[1]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4), windowModel, GL_DYNAMIC_DRAW);
            glBindTexture(GL_TEXTURE_2D, texID);
            glBindVertexArray(windowVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        */

        skyProg.use();
        skyProg.setUnif(skyViewID, skyView);
        skybox.Draw(skyProg, 1, NULL, NULL);

        // Draw to quad using texture bound to my framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        basProg.use();
        glBindVertexArray(postVAO);
        glBindTexture(GL_TEXTURE_2D, colBuffTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // buffer swap and event poll
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    delete[NR_CUBES] cubeModels;
    delete[NR_CUBES] cubeNormMats;
    delete windowModel;
    delete[numVars] variableOffsets;
    glDeleteVertexArrays(1, &windowVAO);
    glDeleteBuffers(2, windowVBOs);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &colBuffTexture);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteTextures(1, &texID);
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

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* userParam)
{
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
        type, severity, message);
}