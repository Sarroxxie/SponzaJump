#include <iostream>
#include <chrono>
#include "window.h"
#include "vulkan/VulkanSetup.h"
#include "vulkan/VulkanRenderer.h"

#include "scene/RenderableObject.h"
#include "rendering/RenderSetup.h"

#define DEFAULT_APPLICATION_WIDTH 1920
#define DEFAULT_APPLICATION_HEIGHT 1080
#define DEFAULT_APPLICATION_NAME "Sponza Jump"

#define CURRENT_MILLIS                                                         \
    (std::chrono::duration_cast<std::chrono::milliseconds>(                    \
        std::chrono::system_clock::now().time_since_epoch()))

#include "scene/SceneSetup.h"
#include "input/CallbackData.h"
#include "physics/GameContactListener.h"

int main() {
    Window window = Window(DEFAULT_APPLICATION_WIDTH,
                           DEFAULT_APPLICATION_HEIGHT, DEFAULT_APPLICATION_NAME);

    ApplicationVulkanContext appContext;
    appContext.window = &window;
    initializeGraphicsApplication(appContext);

    Scene               scene(appContext);
    GameContactListener contactListener;
    createSamplePhysicsScene(appContext, scene, contactListener);

    RenderContext renderContext;
    auto          renderSetupDescription =
        initializeSimpleSceneRenderContext(appContext, renderContext, scene);

    VulkanRenderer renderer(appContext, renderContext);

    InputController inputController;
    scene.setInputController(&inputController);

    CallbackData callbackData;
    callbackData.renderer        = &renderer;
    callbackData.inputController = &inputController;

    scene.getWorld().SetContactListener((b2ContactListener*)&contactListener);

    // passes reference to the renderer to the key callback function
    glfwSetWindowUserPointer(window.getWindowHandle(), (void*)&callbackData);

    if(renderContext.usesImgui) {
        ImGui_ImplGlfw_InitForVulkan(window.getWindowHandle(), true);
    }

    std::chrono::milliseconds lastUpdate       = CURRENT_MILLIS;
    std::chrono::milliseconds delta            = std::chrono::milliseconds(0);
    std::chrono::milliseconds accumulatedDelta = std::chrono::milliseconds(0);

    std::chrono::milliseconds targetPhysicsRate = std::chrono::milliseconds(20);

    while(!glfwWindowShouldClose(window.getWindowHandle())) {
        glfwPollEvents();

        if(renderContext.usesImgui) {
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
            ImGui::Text("%i geometry pass draw calls", renderContext.imguiData.meshDrawCalls);
            ImGui::Text("%i shadow pass draw calls", renderContext.imguiData.shadowPassDrawCalls);
            // lights get drawn once into stencil buffer and once for shading
            ImGui::Text("%i light draw calls", renderContext.imguiData.lightDrawCalls * 2);
            ImGui::SetWindowSize(ImVec2(0, 0), ImGuiCond_Once);
            ImGui::End();
        }
        renderer.render(scene);

        if (scene.gameplayActive()) {
            delta      = (CURRENT_MILLIS - lastUpdate);
            lastUpdate = CURRENT_MILLIS;

            accumulatedDelta += delta;
            if(accumulatedDelta >= targetPhysicsRate) {
                scene.doPhysicsUpdate(targetPhysicsRate.count());

                // accumulatedDelta -= targetPhysicsRate;
                int amountStepsInAcc = accumulatedDelta / targetPhysicsRate;
                if(amountStepsInAcc > 1) {
                    std::cout << "Skipping " << amountStepsInAcc - 1
                              << " physics steps" << std::endl;
                }
                accumulatedDelta = accumulatedDelta % targetPhysicsRate;
            }

            scene.handleUserInput();
        }
    }
    vkDeviceWaitIdle(appContext.baseContext.device);

    scene.cleanup();
    renderer.cleanVulkanRessources();
    cleanupRenderContext(appContext.baseContext, renderContext);
    cleanupVulkanApplication(appContext);

    return 0;
}
