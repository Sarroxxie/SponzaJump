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

    ApplicationContext appContext;
    appContext.window = &window;
    initializeGraphicsApplication(appContext);
    // VulkanContext context;
    // initializeVulkan(context, window);

    VulkanRenderer renderer(appContext);
    // passes reference to the renderer to the key callback function
    glfwSetWindowUserPointer(window.getWindowHandle(), (void*) &renderer);

    while(!glfwWindowShouldClose(window.getWindowHandle())) {
        glfwPollEvents();

        renderer.render();
    }
    vkDeviceWaitIdle(appContext.baseContext.device);

    renderer.cleanVulkanRessources();
    cleanupVulkan(appContext);

    return 0;
}
