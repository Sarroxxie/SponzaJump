#include <iostream>
#include <chrono>
#include "window.h"
#include "vulkan/VulkanSetup.h"
#include "vulkan/VulkanRenderer.h"

#include "scene/RenderableObject.h"
#include "rendering/RenderSetup.h"

#define DEFAULT_APPLICATION_WIDTH 800
#define DEFAULT_APPLICATION_HEIGHT 600
#define DEFAULT_APPLICATION_NAME "GraphicsPraktikum"

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

    RenderContext renderContext;
    auto          renderSetupDescription =
        initializeSimpleSceneRenderContext(appContext, renderContext);

    Scene               scene(appContext.baseContext, renderContext);
    GameContactListener contactListener;
    createSamplePhysicsScene(appContext, scene, contactListener);

    vkDestroyPipeline(appContext.baseContext.device,
                      renderContext.renderPasses.mainPass.renderPassContext.graphicsPipelines[0],
                      nullptr);

    vkDestroyPipelineLayout(
        appContext.baseContext.device,
        renderContext.renderPasses.mainPass.renderPassContext.pipelineLayouts[0], nullptr);

    cleanMainPassDescriptorLayouts(appContext.baseContext,
                                   renderContext.renderPasses.mainPass);

    createMainPassDescriptorSetLayouts(appContext, renderContext.renderPasses.mainPass,
                                       scene.getSceneData().textures);

    /*
    cleanVisualizationPipeline(appContext.baseContext, renderContext.renderPasses.mainPass);
    createVisualizationPipeline(appContext, renderContext, renderContext.renderPasses.mainPass);
    */

     // TODO: create graphics pipeline here (all texture data is only available from here on)
    createGraphicsPipeline(
        appContext, renderContext.renderPasses.mainPass.renderPassContext,
        renderContext.renderPasses.mainPass.renderPassContext.pipelineLayouts[0],
        renderContext.renderPasses.mainPass.renderPassContext.graphicsPipelines[0],
        renderContext.renderPasses.mainPass.renderPassContext.renderPassDescription,
        renderContext.renderPasses.mainPass.renderPassContext.descriptorSetLayouts);

    createMainPassResources(appContext, renderContext, scene.getSceneData().materials);

    createMainPassDescriptorSets(appContext, renderContext, scene.getSceneData().textures);

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
            ImGui::SetWindowSize(ImVec2(0, 0), ImGuiCond_Once);
            ImGui::End();
        }
        renderer.render(scene);
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
    vkDeviceWaitIdle(appContext.baseContext.device);

    scene.cleanup();
    renderer.cleanVulkanRessources();
    cleanupRenderContext(appContext.baseContext, renderContext);
    cleanupVulkanApplication(appContext);

    return 0;
}
