#include <iostream>
#include "window.h"
#include "vulkan/VulkanSetup.h"
#include "vulkan/VulkanRenderer.h"

#include "utils/FileUtils.h"
#include "scene/RenderableObject.h"

#define DEFAULT_APPLICATION_WIDTH 800
#define DEFAULT_APPLICATION_HEIGHT 600
#define DEFAULT_APPLICATION_NAME "GraphicsPraktikum"

int main() {
    Window window = Window(DEFAULT_APPLICATION_WIDTH,
                           DEFAULT_APPLICATION_HEIGHT, DEFAULT_APPLICATION_NAME);

    ApplicationVulkanContext appContext;
    appContext.window = &window;
    initializeGraphicsApplication(appContext);

    RenderContext renderContext;
    initializeRenderContext(appContext, renderContext);

    Scene scene;

    /*
    scene.addObject(createObject(appContext.baseContext, appContext.commandContext, COLORED_SQUARE_DEF, glm::vec3(0.5, 0.5, 0.0)));
    scene.addObject(createObject(appContext.baseContext, appContext.commandContext, COLORED_SQUARE_DEF, glm::vec3(-0.5, -0.5, 0)));
    scene.addObject(createObject(appContext.baseContext, appContext.commandContext, COLORED_TRIANGLE_DEF, glm::vec3(-0.5, 0.5, 0)));
    scene.addObject(createObject(appContext.baseContext, appContext.commandContext, COLORED_TRIANGLE_DEF, glm::vec3(0.5, -0.5, 0)));
     */
    scene.addObject(createObject(appContext.baseContext, appContext.commandContext, COLORED_TRIANGLE_DEF, glm::vec3(0.0, 0, 0.1)));

    VulkanRenderer renderer(appContext, renderContext);

    // passes reference to the renderer to the key callback function
    glfwSetWindowUserPointer(window.getWindowHandle(), (void*) &renderer);

    while(!glfwWindowShouldClose(window.getWindowHandle())) {
        glfwPollEvents();

        renderer.render(scene);
    }
    vkDeviceWaitIdle(appContext.baseContext.device);

    scene.cleanup(appContext.baseContext);
    renderer.cleanVulkanRessources();
    cleanupRenderContext(appContext.baseContext, renderContext.vulkanRenderContext);
    cleanupVulkanApplication(appContext);

    return 0;
}
