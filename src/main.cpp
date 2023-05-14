#include <iostream>
#include "window.h"
#include "vulkan/VulkanSetup.h"
#include "vulkan/VulkanRenderer.h"

#include "utils/FileUtils.h"
#include "scene/RenderableObject.h"
#include "rendering/RenderSetup.h"

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
    initializeSimpleSceneRenderContext(appContext, renderContext);

    Scene scene(appContext.baseContext, renderContext);

    /*
    scene.addObject(createObject(appContext.baseContext,
                                 appContext.commandContext,
                                 COLORED_PYRAMID,
                                 {glm::vec3(1, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0.5)}));

    scene.addObject(createObject(appContext.baseContext,
                                 appContext.commandContext,
                                 COLORED_CUBE_DEF,
                                 {glm::vec3(-1, 0, 0), glm::vec3(0, glm::radians(45.0f), 0), glm::vec3(0.5)}));
    */

    int halfCountPerDimension = 2;
    int spacing = 5;
    for (int i = -halfCountPerDimension; i <= halfCountPerDimension; i++) {
        for (int j = -halfCountPerDimension; j <= halfCountPerDimension; j++) {
            for (int k = -halfCountPerDimension; k <= halfCountPerDimension; k++) {
                scene.addObject(createObject(appContext.baseContext,
                                             appContext.commandContext,
                                             COLORED_CUBE_DEF,
                                             {glm::vec3(j * spacing, k * spacing, i * spacing),
                                              glm::vec3(0, glm::radians(45.0f), 0), glm::vec3(0.5)}));
            }
        }
    }

    VulkanRenderer renderer(appContext, renderContext);

    // passes reference to the renderer to the key callback function
    glfwSetWindowUserPointer(window.getWindowHandle(), (void *) &renderer);

    if (renderContext.usesImgui) {
        ImGui_ImplGlfw_InitForVulkan(window.getWindowHandle(), true);
    }

    while (!glfwWindowShouldClose(window.getWindowHandle())) {
        glfwPollEvents();

        if (renderContext.usesImgui) {
            // @IMGUI
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("Debug");
            ImGui::Text("Average Frametime: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
            ImGui::Text("Average Framerate: %.1f FPS", ImGui::GetIO().Framerate);
            ImGui::End();
        }

        renderer.render(scene);
    }
    vkDeviceWaitIdle(appContext.baseContext.device);

    scene.cleanup(appContext.baseContext);
    renderer.cleanVulkanRessources();
    cleanupRenderContext(appContext.baseContext, renderContext);
    cleanupVulkanApplication(appContext);

    return 0;
}
