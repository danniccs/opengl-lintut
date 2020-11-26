#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "shader.h" // Shader class
#include "misc_sources.h"

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

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    float vertices[] = {
        -0.5f, 0.5f, 0.0f, 1.0f,
        -1.0f, -0.5f, 0.0f, 1.0f,
        0.0f, -0.5f, 0.0f, 1.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 0.0f,
        1.0f, -0.5f, 0.0f, 0.0f,
    };

    unsigned int indices[] = {
        0, 1, 2, // first triangle
        3, 4, 5, // second triangle
    };

    // compile and link the shader program
    Shader sProg("basic_vertex_shader.txt", "basic_fragment_shader.txt");

    // Create a vertex array object and bind it
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Create a vertex buffer object, bind it and give it the vertices as data
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Create an Element Buffer Object to hold the indices
    unsigned int EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Tell OpenGL how to interpret the vertices
    // Location attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 'Boolean' attribute
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Define the clear color
    glClearColor(0.1f, 0.5f, 0.6f, 1.0f);

    // Draw in Wireframe mode
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Draw in normal mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    while (!glfwWindowShouldClose(window)) {
        // input
        processInput(window);

        // render
        // Clear the color
        glClear(GL_COLOR_BUFFER_BIT);
        // set the uniform values and use the program
        float timeValue = glfwGetTime();
        float greenValue = (sin(timeValue) / 2.0f) + 0.5f;
        sProg.use();
        sProg.setUnifS("ourColor", 0.0f, greenValue, 0.0f, 1.0f);
        sProg.setUnifS("ourOtherColor", 0.7f, greenValue, 0.0f, 1.0f);
        // bind the appropriate array
        glBindVertexArray(VAO);
        // draw the elements
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);

        // buffer swap and event poll
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}