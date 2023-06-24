#include "Scene.h"
#include "rendering/RenderContext.h"
#include "physics/PhysicsComponent.h"
#include "game/PlayerComponent.h"
#include "rendering/host_device.h"
#include <glm/gtc/type_ptr.hpp>

Scene::Scene(ApplicationVulkanContext &vulkanContext, RenderContext& renderContext, Camera camera)
    : m_Camera(camera)
    , m_World(b2World(b2Vec2(0, -30.0)))
    , m_Context(vulkanContext) {}

void Scene::cleanup() {
    for(auto pair : componentPools) {
        pair.second.clean();
    }

    for(auto& mesh : sceneData.meshes) {
        mesh.cleanup(m_Context.baseContext);
    }

    for(auto& texture : sceneData.textures) {
        texture.cleanup(m_Context.baseContext);
    }

    sceneData.cubemap.cleanup(m_Context.baseContext);
}

ModelLoadingOffsets Scene::getModelLoadingOffsets() {
    ModelLoadingOffsets offsets;
    offsets.meshesOffset    = sceneData.meshes.size();
    offsets.meshPartsOffset = sceneData.meshParts.size();
    offsets.texturesOffset  = sceneData.textures.size();
    offsets.materialsOffset = sceneData.materials.size();
    offsets.modelsOffset    = sceneData.models.size();
    return offsets;
}


SceneData& Scene::getSceneData() {
    return sceneData;
}

Camera& Scene::getCameraRef() {
    return m_Camera;
}

b2World& Scene::getWorld() {
    return m_World;
}

void Scene::registerSceneImgui(RenderContext& renderContext) {
    ImGui::SetNextWindowSize(ImVec2(0, 0));

    auto width = static_cast<float>(m_Context.swapchainContext.swapChainExtent.width);
    constexpr float padding = 5;


    ImGui::SetNextWindowPos(ImVec2(width - padding, 5), ImGuiCond_Always, ImVec2(1, 0));
    ImGui::Begin("Scene");

    if(ImGui::CollapsingHeader("Camera Controls")) {
        ImGui::SliderFloat3("Camera Pos",
                            glm::value_ptr(m_Camera.getWorldPosRef()), 0, 100);

        ImGui::SliderFloat3("Camera Dir", glm::value_ptr(m_Camera.getViewDirRef()),
                            -glm::pi<float>(), glm::pi<float>());

        ImGui::Checkbox("Lock Camera to Player", &renderContext.imguiData.lockCamera);
    }
    if(ImGui::CollapsingHeader("Shadow Controls")) {

        ImGui::SliderFloat3(
            "Light Camera Dir",
            glm::value_ptr(
                renderContext.renderSettings.shadowMappingSettings.lightCamera.getViewDirRef()),
            -1, 1);

        ImGui::SliderFloat("Light Camera Dist",
                           &renderContext.renderSettings.shadowMappingSettings.lightCameraDist,
                           -100, 100);
        ImGui::Checkbox("Lock shadow to player",
                        &renderContext.renderSettings.shadowMappingSettings.snapToPlayer);

        ImGui::Checkbox("Visualize Shadow Buffer", &renderContext.imguiData.visualizeShadowBuffer);

        ImGui::SliderFloat(
            "Ortho Dim",
            &renderContext.renderSettings.shadowMappingSettings.projection.widthHeightDim,
            0, 200);
        ImGui::SliderFloat(
            "Ortho zNear",
            &renderContext.renderSettings.shadowMappingSettings.projection.zNear,
            0, renderContext.renderSettings.shadowMappingSettings.projection.zFar);
        ImGui::SliderFloat(
            "Ortho zFar",
            &renderContext.renderSettings.shadowMappingSettings.projection.zFar,
            renderContext.renderSettings.shadowMappingSettings.projection.zNear, 400);
    }


    if(ImGui::CollapsingHeader("Depth Controls")) {
        ImGui::SliderFloat("Depth Bias Constant",
                           &renderContext.imguiData.depthBiasConstant, -5, 5);

        ImGui::SliderFloat("Depth Bias Slope",
                           &renderContext.imguiData.depthBiasSlope, -5, 5);
    }
    ImGui::End();
}

EntityId Scene::addEntity() {
    if(freeEntities.empty()) {
        entities.emplace_back();
        return static_cast<EntityId>(entities.size() - 1);
    }
    EntityId id = freeEntities.back();
    freeEntities.pop_back();
    entities[id] = {true, ComponentMask()};
    return id;
}

bool Scene::removeEntity(EntityId id) {
    if(entities.size() <= id || !entities[id].active) {
        return false;
    }

    entities[id] = {false, ComponentMask()};
    freeEntities.push_back(id);

    return true;
}

std::vector<Entity>& Scene::getEntities() {
    return entities;
}

void Scene::doPhysicsUpdate(uint64_t deltaMillis) {
    float timeStep           = static_cast<float>(deltaMillis) / 1000.0f;
    int32 velocityIterations = 6;
    int32 positionIterations = 2;
    m_World.Step(timeStep, velocityIterations, positionIterations);
    for(EntityId id : SceneView<Transformation, PhysicsComponent>(*this)) {
        auto* physicsComponent = getComponent<PhysicsComponent>(id);
        if(physicsComponent->dynamic) {
            auto* transform = getComponent<Transformation>(id);

            b2Vec2 newPos = physicsComponent->body->GetPosition();

            transform->translation =
                glm::vec3(newPos.x, newPos.y, transform->translation.z);
            transform->rotation.z = physicsComponent->body->GetAngle();
        }
    }
}

void Scene::handleUserInput() {
    if(m_InputController == nullptr)
        return;
    bool movingLeft  = m_InputController->isPressed(GLFW_KEY_A);
    bool movingRight = m_InputController->isPressed(GLFW_KEY_D);

    bool wantsToJump = m_InputController->getSinglePress(GLFW_KEY_SPACE);

    float speed = 10;

    for(auto id : SceneView<PlayerComponent, PhysicsComponent>(*this)) {
        auto* physicsComponent = getComponent<PhysicsComponent>(id);
        auto* playerComponent  = getComponent<PlayerComponent>(id);

        float notMovingEps = 1e-4;

        b2Vec2 linVel = physicsComponent->body->GetLinearVelocity();

        bool jumps = false;
        if(wantsToJump) {
            if(playerComponent->grounded) {
                jumps = true;
            } else if(playerComponent->canDoubleJump) {
                jumps                          = true;
                playerComponent->canDoubleJump = false;
            }
        }

        b2Vec2 newVel;
        newVel.x = movingRight ? speed : movingLeft ? -speed : 0;

        newVel.y = linVel.y;
        if(jumps) {
            newVel.y                  = 20;
            playerComponent->grounded = false;
        }

        physicsComponent->body->SetLinearVelocity(newVel);
    }
}

void Scene::setInputController(InputController* inputController) {
    m_InputController = inputController;
}
void Scene::doCameraUpdate(RenderContext& renderContext) {
    for(auto id : SceneView<PlayerComponent, Transformation>(*this)) {
        auto* transformation = getComponent<Transformation>(id);

        if(renderContext.imguiData.lockCamera) {
            auto prevPos = m_Camera.getWorldPos();

            m_Camera.setPosition(glm::vec3(transformation->translation.x,
                                           transformation->translation.y + 10,
                                           prevPos.z));
            m_Camera.setLookAt(glm::vec3(transformation->translation.x,
                                         transformation->translation.y + 5, 0));
        }

        if(renderContext.renderSettings.shadowMappingSettings.snapToPlayer) {
            Camera& lightCamera =
                renderContext.renderSettings.shadowMappingSettings.lightCamera;

            lightCamera.normalizeViewDir();

            lightCamera.setPosition(
                transformation->translation
                - lightCamera.getViewDir()
                      * renderContext.renderSettings.shadowMappingSettings.lightCameraDist);
        }
    }
}
