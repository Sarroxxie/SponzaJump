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

int main() {
    Window window = Window(DEFAULT_APPLICATION_WIDTH,
                           DEFAULT_APPLICATION_HEIGHT, DEFAULT_APPLICATION_NAME);

    ApplicationVulkanContext appContext;
    appContext.window = &window;
    initializeGraphicsApplication(appContext);

    RenderContext renderContext;
    initializeSimpleSceneRenderContext(appContext, renderContext);

    Scene scene(appContext.baseContext, renderContext);

    EntityId groundEntity = scene.addEntity();

    auto *pObject = scene.assign<MeshComponent>(groundEntity);
    createMeshComponent(pObject, appContext.baseContext, appContext.commandContext, getCuboid(glm::vec3(1, 1, 1)));

    auto *pTransform = scene.assign<Transformation>(groundEntity);
    *pTransform = {
            glm::vec3(0, 0, 0),
            glm::vec3(0, 0, 0),
            glm::vec3(1)};

    auto *groundBodyComponent = scene.assign<PhysicsComponent>(groundEntity);
    b2BodyDef groundBodyDef;
    groundBodyDef.position.Set(0, 0);
    groundBodyComponent->body = scene.getWorld().CreateBody(&groundBodyDef);
    groundBodyComponent->dynamic = false;

    b2PolygonShape groundBox;
    groundBox.SetAsBox(1, 1);
    groundBodyComponent->body->CreateFixture(&groundBox, 0.0);


    EntityId dynamicEntity = scene.addEntity();

    auto *meshComponent = scene.assign<MeshComponent>(dynamicEntity);
    createMeshComponent(meshComponent, appContext.baseContext, appContext.commandContext, getCuboid(glm::vec3(1, 1, 1), glm::vec3(0.8, 0.1, 0.1)));

    auto *pDynamicTransform = scene.assign<Transformation>(dynamicEntity);
    *pDynamicTransform = {
            glm::vec3(3, 20, 0),
            glm::vec3(0, 0, 0),
            glm::vec3(1)};

    auto *pDynamicPhysicsComponent = scene.assign<PhysicsComponent>(dynamicEntity);

    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.fixedRotation = true;
    bodyDef.position.Set(3.0f, 20.0f);
    pDynamicPhysicsComponent->body = scene.getWorld().CreateBody(&bodyDef);
    pDynamicPhysicsComponent->dynamic = true;

    b2PolygonShape dynamicBox;
    dynamicBox.SetAsBox(1.0f, 1.0f);

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &dynamicBox;
    fixtureDef.density = 1.0f;
    fixtureDef.friction = 0.3f;

    pDynamicPhysicsComponent->body->CreateFixture(&fixtureDef);


    EntityId dynamicEntity2 = scene.addEntity();

    auto *meshComponent2 = scene.assign<MeshComponent>(dynamicEntity2);
    createMeshComponent(meshComponent2, appContext.baseContext, appContext.commandContext, getCuboid(glm::vec3(1, 1, 1), glm::vec3(0.1, 0.1, 0.8)));

    auto *pDynamicTransform2 = scene.assign<Transformation>(dynamicEntity2);
    *pDynamicTransform2 = {
            glm::vec3(0, 20, 0),
            glm::vec3(0, 0, 0),
            glm::vec3(1)};

    auto *pDynamicPhysicsComponent2 = scene.assign<PhysicsComponent>(dynamicEntity2);

    b2BodyDef bodyDef2;
    bodyDef2.type = b2_dynamicBody;
    bodyDef2.fixedRotation = true;
    bodyDef2.position.Set(0.0f, 20.0f);
    pDynamicPhysicsComponent2->body = scene.getWorld().CreateBody(&bodyDef2);
    pDynamicPhysicsComponent2->dynamic = true;

    b2PolygonShape dynamicBox2;
    dynamicBox2.SetAsBox(1.0f, 1.0f);

    b2FixtureDef fixtureDef2;
    fixtureDef2.shape = &dynamicBox2;
    fixtureDef2.density = 1.0f;
    fixtureDef2.friction = 0.3f;

    pDynamicPhysicsComponent2->body->CreateFixture(&fixtureDef2);

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


        float timeStep = 1.0f / 4000.0f;
        int32 velocityIterations = 6;
        int32 positionIterations = 2;
        scene.getWorld().Step(timeStep, velocityIterations, positionIterations);
        for (EntityId id : SceneView<Transformation, PhysicsComponent>(scene)) {
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
