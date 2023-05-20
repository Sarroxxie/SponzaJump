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

#include "box2d/b2_world.h">
#include "physics/PhysicsComponent.h"
#include "scene/SceneSetup.h"

int main() {
    Window window = Window(DEFAULT_APPLICATION_WIDTH,
                           DEFAULT_APPLICATION_HEIGHT, DEFAULT_APPLICATION_NAME);

    ApplicationVulkanContext appContext;
    appContext.window = &window;
    initializeGraphicsApplication(appContext);

    RenderContext renderContext;
    initializeSimpleSceneRenderContext(appContext, renderContext);

    Scene scene(appContext.baseContext, renderContext);

    glm::vec3 staticHalfSize = glm::vec3(15, 1, 1);
    addPhysicsEntity(scene,
                     appContext.baseContext,
                     appContext.commandContext,
                     getCuboid(staticHalfSize),
                     Transformation (),
                     staticHalfSize,
                     false);

    glm::vec3 staticHalfSizeLeft = glm::vec3(20, 1, 1);
    addPhysicsEntity(scene,
                     appContext.baseContext,
                     appContext.commandContext,
                     getCuboid(staticHalfSizeLeft),
                     { glm::vec3(-15 - 10, 10, 0), glm::vec3(0, 0, -glm::half_pi<float>() / 2), glm::vec3(1) },
                     staticHalfSizeLeft,
                     false);

    glm::vec3 staticHalfSizeRight = glm::vec3(20, 1, 1);
    addPhysicsEntity(scene,
                     appContext.baseContext,
                     appContext.commandContext,
                     getCuboid(staticHalfSizeRight),
                     { glm::vec3(15 + 10, 10, 0), glm::vec3(0, 0, glm::half_pi<float>() / 2), glm::vec3(1) },
                     staticHalfSizeRight,
                     false);


    int numDynamicObjects = 30;
    for (int i = 0; i < numDynamicObjects; i++) {
        glm::vec3 halfSize = glm::vec3(1, 1, 1);
        addPhysicsEntity(scene,
                         appContext.baseContext,
                         appContext.commandContext,
                         getCuboid(halfSize, glm::vec3(static_cast<float>(i + 1) / (numDynamicObjects + 1.0f))),
                         {glm::vec3(-15 + ((static_cast<float>(i) / numDynamicObjects) * 30), 20 + 5 * i, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                         halfSize,
                         true,
                         false);
    }

    /*
    int halfCountPerDimension = 2;
    int spacing = 5;
    for (int i = -halfCountPerDimension; i <= halfCountPerDimension; i++) {
        for (int j = -halfCountPerDimension; j <= halfCountPerDimension; j++) {
            for (int k = -halfCountPerDimension; k <= halfCountPerDimension; k++) {
                EntityId groundEntity = scene.addEntity();

                auto *pObject = scene.assign<MeshComponent>(groundEntity);
                createMeshComponent(pObject, appContext.baseContext, appContext.commandContext, COLORED_CUBE_DEF);

                auto *pTransform = scene.assign<Transformation>(groundEntity);
                *pTransform = {
                        glm::vec3(j * spacing, k * spacing, i * spacing),
                        glm::vec3(0, 0, 0),
                        glm::vec3(0.5)};
            }
        }
    }
     */

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
            ImGui::SetWindowSize(ImVec2(0, 0), ImGuiCond_Once);
            ImGui::End();
        }

        renderer.render(scene);


        float timeStep = 1.0f / 800.0f;
        int32 velocityIterations = 6;
        int32 positionIterations = 2;
        scene.getWorld().Step(timeStep, velocityIterations, positionIterations);
        for (EntityId id: SceneView<Transformation, PhysicsComponent>(scene)) {
            auto *physicsComponent = scene.getComponent<PhysicsComponent>(id);
            if (physicsComponent->dynamic) {
                auto *transform = scene.getComponent<Transformation>(id);

                b2Vec2 newPos = physicsComponent->body->GetPosition();

                if (newPos.y < -5) {
                    b2Vec2 vel = physicsComponent->body->GetLinearVelocity();
                    physicsComponent->body->SetLinearVelocity(b2Vec2(vel.x, -vel.y));
                }
                transform->translation = glm::vec3(newPos.x, newPos.y, transform->translation.z);
                transform->rotation.z = physicsComponent->body->GetAngle();

            }
        }
    }
    vkDeviceWaitIdle(appContext.baseContext.device);

    scene.cleanup();
    renderer.cleanVulkanRessources();
    cleanupRenderContext(appContext.baseContext, renderContext);
    cleanupVulkanApplication(appContext);

    return 0;
}
