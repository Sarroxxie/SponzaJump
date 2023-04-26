#include "window.h"

Window::Window(int width, int height, std::string application_name)
    : width(width)
    , height(height)
    , application_name(application_name) {
    initGLFW();
}

Window::~Window() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Window::initGLFW() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, application_name.c_str(), nullptr, nullptr);
    // glfwSetMouseButtonCallback(window, mouseButtonCallback);
    //  glfwSetWindowUserPointer(window, this);
    //  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void Window::render() {
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}