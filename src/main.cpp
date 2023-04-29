#include <iostream>
#include "window.h"
#include "vulkan/VulkanSetup.h"
#include "vulkan/VulkanRenderer.h"

#define DEFAULT_APPLICATION_WIDTH 800
#define DEFAULT_APPLICATION_HEIGHT 600
#define DEFAULT_APPLICATION_NAME "GraphicsPraktikum"

int main() {
    // macro is defined by cmake
    std::cout << "glslangValidator path: " << VULKAN_GLSLANG_VALIDATOR_PATH << "\n";

    Window window = Window(DEFAULT_APPLICATION_WIDTH,
                           DEFAULT_APPLICATION_HEIGHT, DEFAULT_APPLICATION_NAME);

    VulkanContext context;
    initializeVulkan(context, window);

    // window.render();

    VulkanRenderer renderer(context, window);

    while(!glfwWindowShouldClose(window.getWindowHandle())) {
        glfwPollEvents();

       renderer.render();
    }
    vkDeviceWaitIdle(context.device);

    renderer.cleanVulkanRessources();
    cleanupVulkan(context);

    return 0;
}
