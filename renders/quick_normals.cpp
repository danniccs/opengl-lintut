#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include "Camera.h" // Camera class
#include "Shader.h" // Shader class
#include "misc_sources.h"

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

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
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

    glEnable(GL_DEPTH_TEST);

    // build and compile our shader program
    // ------------------------------------
    Shader normProg("../shaders/quick_normals_vert.glsl", "../shaders/quick_normals_frag.glsl");

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float norm_vertices[] = {
        -0.5f, -0.5f, -0.5f, // bottom left front
         0.5f, -0.5f, -0.5f, // bottom right front
        -0.5f,  0.5f, -0.5f, // top left front
         0.5f,  0.5f, -0.5f,  // top right front
        -0.5f,  0.5f,  0.5f, // top left back
         0.5f,  0.5f,  0.5f, // top right back
        -0.5f, -0.5f,  0.5f, // bottom left back
         0.5f, -0.5f,  0.5f,  // bottom right back
    };

    unsigned int indices[] = { // note that we start from 0!
        0, 1, 2,
        2, 1, 3,
        2, 3, 4,
        4, 3, 5,
        4, 5, 6,
        6, 5, 7,
        6, 7, 0,
        0, 7, 1,
        1, 7, 3,
        3, 7, 5,
        6, 0, 4,
        4, 0, 2,
    };

    /*float vertices2[] = {
     *    -0.5f, -0.5f, -0.5f,
     *     0.5f, -0.5f, -0.5f,
     *     0.5f,  0.5f, -0.5f,
     *     0.5f,  0.5f, -0.5f,
     *    -0.5f,  0.5f, -0.5f,
     *    -0.5f, -0.5f, -0.5f,

     *    -0.5f, -0.5f,  0.5f,
     *     0.5f, -0.5f,  0.5f,
     *     0.5f,  0.5f,  0.5f,
     *     0.5f,  0.5f,  0.5f,
     *    -0.5f,  0.5f,  0.5f,
     *    -0.5f, -0.5f,  0.5f,

     *    -0.5f,  0.5f,  0.5f,
     *    -0.5f,  0.5f, -0.5f,
     *    -0.5f, -0.5f, -0.5f,
     *    -0.5f, -0.5f, -0.5f,
     *    -0.5f, -0.5f,  0.5f,
     *    -0.5f,  0.5f,  0.5f,

     *     0.5f,  0.5f,  0.5f,
     *     0.5f,  0.5f, -0.5f,
     *     0.5f, -0.5f, -0.5f,
     *     0.5f, -0.5f, -0.5f,
     *     0.5f, -0.5f,  0.5f,
     *     0.5f,  0.5f,  0.5f,

     *    -0.5f, -0.5f, -0.5f,
     *     0.5f, -0.5f, -0.5f,
     *     0.5f, -0.5f,  0.5f,
     *     0.5f, -0.5f,  0.5f,
     *    -0.5f, -0.5f,  0.5f,
     *    -0.5f, -0.5f, -0.5f,

     *    -0.5f,  0.5f, -0.5f,
     *     0.5f,  0.5f, -0.5f,
     *     0.5f,  0.5f,  0.5f,
     *     0.5f,  0.5f,  0.5f,
     *    -0.5f,  0.5f,  0.5f,
     *    -0.5f,  0.5f, -0.5f,
     *};
     */

    float colors[] = {
        42.0f,  42.0f,  170.0f,
        179.0f, 75.0f,  231.0f,
        75.0f,  179.0f, 231.0f,
        212.0f, 212.0f, 170.0f,
        42.0f,  212.0f, 84.0f,
        179.0f, 179.0f, 23.0f,
        75.0f,  75.0f,  23.0f,
        212.0f, 42.0f,  84.0f,
    };

    for (unsigned int i = 0; i < 24; i++)
        colors[i] = colors[i] / 255;

    unsigned int VBO, VBO2, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &VBO2);
    glGenBuffers(1, &EBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(norm_vertices), norm_vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, VBO2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    //glBindBuffer(GL_ARRAY_BUFFER, 0);

    // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);


    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw our first triangle
        normProg.use();
        
        // pass projection matrix to shader (note that in this case it could change every frame)
        glm::mat4 projection = glm::perspective(glm::radians(cam.Zoom), 800.0f / 600.0f, 0.1f, 100.0f);
        normProg.setUnifS("projection", projection);

        // camera/view transformation
        glm::mat4 view = cam.GetViewMatrix();
        normProg.setUnifS("view", view);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -5.0f));
        normProg.setUnifS("model", model);

        glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        //glDrawArrays(GL_POINTS, 0, 4);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0); // no need to unbind it every time 

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &VBO2);
    glDeleteBuffers(1, &EBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
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
