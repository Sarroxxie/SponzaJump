#include <iostream>
#include <chrono>
#include "window.h"
#include "vulkan/VulkanSetup.h"
#include "vulkan/VulkanRenderer.h"

#include "scene/RenderableObject.h"
#include "rendering/RenderSetup.h"

#include "tiny_gltf.h"
#include "stb_image.h"

#include "scene/ModelLoader.h"

#define DEFAULT_APPLICATION_WIDTH 800
#define DEFAULT_APPLICATION_HEIGHT 600
#define DEFAULT_APPLICATION_NAME "GraphicsPraktikum"

#define CURRENT_MILLIS (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()))

#include "scene/SceneSetup.h"
#include "input/CallbackData.h"
#include "physics/GameContactListener.h"


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
    GameContactListener contactListener;
    createSamplePhysicsScene(appContext, scene, contactListener);

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

    VulkanRenderer renderer(appContext, renderContext);

    InputController inputController;
    scene.setInputController(&inputController);

    CallbackData callbackData;
    callbackData.renderer = &renderer;
    callbackData.inputController = &inputController;


    scene.getWorld().SetContactListener((b2ContactListener *) &contactListener);

    // passes reference to the renderer to the key callback function
    glfwSetWindowUserPointer(window.getWindowHandle(), (void *) &callbackData);

    if (renderContext.usesImgui) {
        ImGui_ImplGlfw_InitForVulkan(window.getWindowHandle(), true);
    }



    std::chrono::milliseconds lastUpdate = CURRENT_MILLIS;
    std::chrono::milliseconds delta = std::chrono::milliseconds(0);
    std::chrono::milliseconds accumulatedDelta = std::chrono::milliseconds(0);

    std::chrono::milliseconds targetPhysicsRate = std::chrono::milliseconds(20);


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
            ImGui::SetWindowSize(ImVec2(0, 0), ImGuiCond_Once);
            ImGui::End();
        }

        renderer.render(scene);

        delta = (CURRENT_MILLIS - lastUpdate);
        lastUpdate = CURRENT_MILLIS;

        accumulatedDelta += delta;
        if (accumulatedDelta >= targetPhysicsRate) {
            scene.doPhysicsUpdate(targetPhysicsRate.count());

            // accumulatedDelta -= targetPhysicsRate;

            int amountStepsInAcc = accumulatedDelta / targetPhysicsRate;
            if (amountStepsInAcc > 1) {
                std::cout << "Skipping " << amountStepsInAcc - 1 << " physics steps" << std::endl;
            }
            accumulatedDelta = accumulatedDelta % targetPhysicsRate;
        }

        scene.handleUserInput();
    }
    vkDeviceWaitIdle(appContext.baseContext.device);

    scene.cleanup();
    renderer.cleanVulkanRessources();
    cleanupRenderContext(appContext.baseContext, renderContext);
    cleanupVulkanApplication(appContext);

    return 0;
}
