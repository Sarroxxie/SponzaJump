#include <iostream>
#include "window.h"
#include "vulkan/VulkanSetup.h"
#include "vulkan/VulkanRenderer.h"

#include "utils/FileUtils.h"

#define DEFAULT_APPLICATION_WIDTH 800
#define DEFAULT_APPLICATION_HEIGHT 600
#define DEFAULT_APPLICATION_NAME "GraphicsPraktikum"

int main() {
    Window window = Window(DEFAULT_APPLICATION_WIDTH,
                           DEFAULT_APPLICATION_HEIGHT, DEFAULT_APPLICATION_NAME);

    VulkanContext context;
    initializeVulkan(context, window);

    VulkanRenderer renderer(context, window);
    // passes reference to the renderer to the key callback function
    glfwSetWindowUserPointer(window.getWindowHandle(), (void*) &renderer);

    while(!glfwWindowShouldClose(window.getWindowHandle())) {
        glfwPollEvents();

        renderer.render();
    }
    vkDeviceWaitIdle(context.device);

    renderer.cleanVulkanRessources();
    cleanupVulkan(context);

    return 0;
}
