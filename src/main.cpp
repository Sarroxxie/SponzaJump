#include <iostream>
#include "GLFW/glfw3.h"

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    int WIDTH = 800, HEIGHT = 500;

    auto window = glfwCreateWindow(WIDTH, HEIGHT, "Graphics Praktikum", nullptr, nullptr);

    //glfwSetMouseButtonCallback(window, mouseButtonCallback);
    // glfwSetWindowUserPointer(window, this);
    // glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);



    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();


    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
