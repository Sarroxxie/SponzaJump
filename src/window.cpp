#include "window.h"
#include <iostream>
#include "vulkan/VulkanRenderer.h"
#include "vulkan/VulkanSetup.h"
#include "input/CallbackData.h"

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* callbackData = (CallbackData*) glfwGetWindowUserPointer(window);

    // VulkanRenderer* renderer = (VulkanRenderer*)glfwGetWindowUserPointer(window);
    // compile shaders
    if(key == GLFW_KEY_C && action == GLFW_PRESS) {
        callbackData->renderer->recompileToSecondaryPipeline();
    } else {
        callbackData->inputController->handleKeyEvent(key, action);
    }
}

static void framebufferResizeCallback(GLFWwindow* glfWwindow, int width, int height) {
    auto* callbackData = (CallbackData*) glfwGetWindowUserPointer(glfWwindow);

    // VulkanRenderer* renderer = (VulkanRenderer*) glfwGetWindowUserPointer(glfWwindow);
    callbackData->renderer->getContext().window->setResized(true);
}


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
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, application_name.c_str(), nullptr, nullptr);

    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    // glfwSetMouseButtonCallback(window, mouseButtonCallback);
    //  glfwSetWindowUserPointer(window, this);
    //  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

GLFWwindow *Window::getWindowHandle() {
    return window;
}

void Window::setResized(bool resized) {
    m_resized = resized;
}

bool Window::wasResized() {
    return m_resized;
}

int Window::getWidth() {
    return width;
}

int Window::getHeight() {
    return height;
}
