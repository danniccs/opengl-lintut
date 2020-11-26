#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <stb-master/stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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

    float vertices[] = {
        // position         // texture coordinates
        -0.5f, 0.5f, 0.0f,      0.0f, 1.0f, // top left
        0.5f, 0.5f, 0.0f,       1.0f, 1.0f, // top right
        -0.5f, -0.5f, 0.0f,     0.0f, 0.0f, // bottom left
        0.5f, -0.5f, 0.0f,      1.0f, 0.0f, // bottom right
    };

    unsigned int indices[] = {
        0, 1, 2, // first triangle
        1, 2, 3, // second triangle
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Define the clear color
    glClearColor(0.1f, 0.5f, 0.6f, 1.0f);

    // Draw in Wireframe mode
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Draw in normal mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // tell OpenGL which sampler belongs to which location
    sProg.use();
    sProg.setUnifS("tex1", 0);

    while (!glfwWindowShouldClose(window)) {
        // input
        processInput(window);

        // render
        // Clear the color
        glClear(GL_COLOR_BUFFER_BIT);
        // object 1
        // bind the appropriate texture objects
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        // set the uniform values and use the program
        sProg.use();
        // create and set the transformation matrix
        glm::mat4 trans = glm::mat4(1.0f);
        double theTime = glfwGetTime();
        double sineTrans = (sin(theTime) / 3.0f);
        double cosTrans = (cos(theTime) / 3.0f);
        trans = glm::translate(trans, glm::vec3(2 * cosTrans, 6 * sineTrans * cosTrans, 0.0f));
        trans = glm::rotate(trans, glm::radians(-50.0f * (float)theTime), glm::vec3(0.0f, 0.0f, 1.0f));
        trans = glm::scale(trans, glm::vec3(0.5f, 0.7f, 0.5f));
        sProg.setUnifS("transform", trans);
        // bind the appropriate array
        glBindVertexArray(VAO);
        // draw the elements
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);

        // object 2
        // bind the appropriate texture objects
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture2);
        // create and set the transformation matrix
        trans = glm::mat4(1.0f);
        sineTrans = abs(sin(theTime)) + 0.3;
        trans = glm::translate(trans, glm::vec3(-0.5f, 0.5f, 0.0f));
        trans = glm::scale(trans, glm::vec3(sineTrans, sineTrans, 0.5f));
        sProg.setUnifS("transform", trans);
        // draw the elements
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);

        // buffer swap and event poll
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glfwTerminate();
    return 0;
}