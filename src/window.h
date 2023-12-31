#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <string>

class Window
{
  private:
    int         width;
    int         height;
    std::string application_name;
    GLFWwindow* window;

    bool m_resized = false;

  public:
    Window(int width, int height, std::string application_name);
    ~Window();

    void initGLFW();

    void setResized(bool resized);
    bool wasResized();

    int getWidth();
    int getHeight();

    GLFWwindow *getWindowHandle();
};


