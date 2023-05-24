#include <iostream>
#include "window.h"
#include "vulkan/VulkanSetup.h"
#include "vulkan/VulkanRenderer.h"

#include "utils/FileUtils.h"
#include "scene/RenderableObject.h"
#include "rendering/RenderSetup.h"

#include "tiny_gltf.h"
#include "stb_image.h"

#include "scene/ModelLoader.h"

#define DEFAULT_APPLICATION_WIDTH 800
#define DEFAULT_APPLICATION_HEIGHT 600
#define DEFAULT_APPLICATION_NAME "GraphicsPraktikum"

int main() {
    // tinygltf test code
    const char*  modelPath = "res/assets/models/debug_model/debug_model.gltf";

    Window window = Window(DEFAULT_APPLICATION_WIDTH,
                           DEFAULT_APPLICATION_HEIGHT, DEFAULT_APPLICATION_NAME);

    ApplicationVulkanContext appContext;
    appContext.window = &window;
    initializeGraphicsApplication(appContext);

    RenderContext renderContext;
    initializeSimpleSceneRenderContext(appContext, renderContext);

    Scene scene(appContext.baseContext, renderContext);

    ModelLoader loader;
    loader.loadModel(modelPath, appContext.baseContext, appContext.commandContext);
    std::cout << "mesh count: " << loader.meshes.size() << "\n";
    int verticesCount = 0;
    int indicesCount    = 0;
    for(auto& mesh : loader.meshes) {
        verticesCount += mesh.verticesCount;
        indicesCount += mesh.indicesCount;
    }
    std::cout << "vertices count: " << verticesCount << "\n";

    std::cout << "triangle count: " << indicesCount / 3 << "\n";

    scene.addObject(loader);

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
    /*
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
    }*/

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

            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
            ImGui::Begin("Statistics", 0,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove
                             | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMouseInputs
                             | ImGuiWindowFlags_NoTitleBar);
            ImGui::Text("%.3f ms", 1000.0f / ImGui::GetIO().Framerate);
            ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
            ImGui::SetWindowSize(ImVec2(0,0), ImGuiCond_Once);
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
