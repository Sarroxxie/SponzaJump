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

    Scene scene;

    scene.addObject(createSampleObject(appContext.baseContext, appContext.commandContext, -0.5));
    scene.addObject(createSampleObject(appContext.baseContext, appContext.commandContext, 0.5));

    VulkanRenderer renderer(appContext);
    // passes reference to the renderer to the key callback function
    glfwSetWindowUserPointer(window.getWindowHandle(), (void*) &renderer);

    while(!glfwWindowShouldClose(window.getWindowHandle())) {
        glfwPollEvents();

        renderer.render(scene);
    }
    vkDeviceWaitIdle(appContext.baseContext.device);

    scene.cleanup(appContext.baseContext);
    renderer.cleanVulkanRessources();
    cleanupVulkan(appContext);

    return 0;
}
