#include "Scene.h"
#include "rendering/RenderContext.h"
#include "physics/PhysicsComponent.h"
#include "game/PlayerComponent.h"
#include "rendering/host_device.h"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

Scene::Scene(ApplicationVulkanContext& vulkanContext, Camera camera)
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

    if(sceneData.pointLightMesh.vertexBuffer != VK_NULL_HANDLE) {
        sceneData.pointLightMesh.cleanup(m_Context.baseContext);
    }

    sceneData.skybox.cleanup(m_Context.baseContext);
    sceneData.irradianceMap.cleanup(m_Context.baseContext);
    sceneData.radianceMap.cleanup(m_Context.baseContext);
    sceneData.brdfIntegrationLUT.cleanup(m_Context.baseContext);
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
                            glm::value_ptr(m_Camera.getWorldPosRef()), 0, 500);

        ImGui::SliderFloat3("Camera Dir", glm::value_ptr(m_Camera.getViewDirRef()), -1, 1);

        ImGui::SliderFloat("Y Lookat Offset", &cameraOffsetY, 0.0f, 10.0f);

        ImGui::Checkbox("Lock Camera to Player", &renderContext.imguiData.lockCamera);
    }
    if(ImGui::CollapsingHeader("Perspective Controls")) {
        PerspectiveSettings& perspectiveSettings =
            renderContext.renderSettings.perspectiveSettings;

        ImGui::SliderFloat("Near plane", &perspectiveSettings.nearPlane, 0.1, 500);
        ImGui::SliderFloat("Far plane", &perspectiveSettings.farPlane, 0.1, 500);
        ImGui::SliderFloat("FOV", &perspectiveSettings.fov, 0, glm::pi<float>());
    }
    if(ImGui::CollapsingHeader("Shadow Controls")) {

        ShadowMappingSettings& shadowSettings = renderContext.renderSettings.shadowMappingSettings;

        ImGui::SliderFloat3("Light Camera Dir",
                            glm::value_ptr(shadowSettings.lightDirection), -1, 1);

        ImGui::SliderFloat("light camera Z offset",
                           &shadowSettings.lightCameraZOffset, 0, 500);

        ImGui::Checkbox("Visualize Shadow Buffer", &renderContext.imguiData.visualizeShadowBuffer);

        ImGui::Checkbox("Player Spikes Shadow", &renderContext.imguiData.playerSpikesShadow);

        ImGui::Checkbox("Percentage Closer Filtering", &renderContext.imguiData.doPCF);

        ImGui::Indent();
        if(ImGui::CollapsingHeader("Cascaded Shadow Map Controls")) {
            ImGui::Checkbox("Visualize Cascades", &shadowSettings.visualizeCascades);

            ImGui::Checkbox("New Cascade Calculations", &shadowSettings.newCascadeCalculation);

            ImGui::Checkbox("Cross Up Vec", &shadowSettings.crossProductUp);

            ImGui::SliderFloat("Cascade Split Depth blend",
                               &shadowSettings.cascadeSplitsBlendFactor, 0.0f, 1.0f);

            ImGui::SliderInt("Number Cascades", &shadowSettings.numberCascades, 1, MAX_CASCADES);

            ImGui::SliderInt("Vis Cascade Index", &shadowSettings.cascadeVisIndex,
                             0, shadowSettings.numberCascades - 1);
        }
        ImGui::Unindent();
    }

    if(ImGui::CollapsingHeader("Depth Controls")) {
        ImGui::SliderFloat("Depth Bias Constant",
                           &renderContext.imguiData.depthBiasConstant, -5, 5);

        ImGui::SliderFloat("Depth Bias Slope",
                           &renderContext.imguiData.depthBiasSlope, -5, 5);
    }

    if(ImGui::CollapsingHeader("Lighting Controls")) {
        ImGui::Checkbox("Point Lights", &renderContext.imguiData.pointLights);
        ImGui::Checkbox("Shadows", &renderContext.imguiData.shadows);
        ImGui::Checkbox("Automatic IBL Factor", &renderContext.imguiData.autoIbl);
        ImGui::SliderFloat("IBL factor", &renderContext.imguiData.iblFactor,
                           0.0, 1, "%3f", ImGuiSliderFlags_Logarithmic);
        ImGui::SliderFloat("Skybox Exposure", &renderContext.imguiData.exposure,
                           0.1,
                           5, "%3f", ImGuiSliderFlags_Logarithmic);
    }

    if(ImGui::CollapsingHeader("Gameplay Controls")) {
        ImGui::Checkbox("Disable Death", &levelData.disableDeath);

        ImGui::SliderFloat("Death Plane height", &levelData.deathPlaneHeight, -30, 30);
    }

    ImGui::End();

    if(levelData.hasWon) {
        registerWinDialog();
    }
}

void Scene::registerWinDialog() {
    auto width = static_cast<float>(m_Context.swapchainContext.swapChainExtent.width);
    auto height = static_cast<float>(m_Context.swapchainContext.swapChainExtent.height);

    auto imguiWindowWidth  = width / 5;
    auto imguiWindowHeight = height / 5;

    ImGui::SetNextWindowSize(ImVec2(imguiWindowWidth, imguiWindowHeight));

    ImGui::SetNextWindowPos(ImVec2(width / 2, height / 2), ImGuiCond_Always,
                            ImVec2(0.5, 0.5));

    ImGui::Begin("Win Dialog");

    ImGui::Text("You Won");
    if(ImGui::Button("Restart")) {
        resetLevel();
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

            if(newPos.x != transform->translation.x
               || newPos.y != transform->translation.y
               || physicsComponent->body->GetAngle() != transform->rotation.z) {
                transform->translation =
                    glm::vec3(newPos.x, newPos.y, transform->translation.z);
                transform->rotation.z = physicsComponent->body->GetAngle();
                transform->hasChanged = true;
            }
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

        if(renderContext.imguiData.autoIbl) {
            // TODO: should remove, this is only for presentation purposes
            float       posX            = transformation->translation.x;
            const float LOWER_THRESHOLD = 90.0f;
            const float UPPER_THRESHOLD = 195.0f;

            if(posX < LOWER_THRESHOLD) {
                renderContext.imguiData.iblFactor = 0.05f;
            } else if(posX > UPPER_THRESHOLD) {
                renderContext.imguiData.iblFactor = 1.0f;
            } else {
                float iblFactor = std::abs(posX - LOWER_THRESHOLD)
                                  / (UPPER_THRESHOLD - LOWER_THRESHOLD);
                // quadratic interpolation looks better here
                renderContext.imguiData.iblFactor =
                    std::clamp(std::powf(iblFactor, 2), 0.05f, 1.0f);
            }
        }


        if(renderContext.imguiData.lockCamera) {
            auto prevPos = m_Camera.getWorldPos();

            m_Camera.setPosition(glm::vec3(transformation->translation.x,
                                           transformation->translation.y + 10,
                                           prevPos.z));
            m_Camera.setLookAt(glm::vec3(transformation->translation.x,
                                         transformation->translation.y + cameraOffsetY, 0));
        }
    }
}

LevelData& Scene::getLevelData() {
    return levelData;
}
void Scene::doGameplayUpdate() {
    for(auto id : SceneView<PlayerComponent, Transformation, PhysicsComponent>(*this)) {
        auto* playerComponent  = getComponent<PlayerComponent>(id);
        auto* transformation   = getComponent<Transformation>(id);
        auto* physicsComponent = getComponent<PhysicsComponent>(id);

        bool died = false;

        if(!levelData.disableDeath) {
            if(transformation->translation.y <= levelData.deathPlaneHeight) {
                std::cout << "Died to death plane" << std::endl;

                died = true;
            }

            if(playerComponent->touchesHazard) {
                std::cout << "Died to Hazard" << std::endl;

                playerComponent->touchesHazard = false;
                died                           = true;
            }
        } else {
            playerComponent->touchesHazard = false;
        }

        if(died) {
            transformation->translation = levelData.playerSpawnLocation;
            transformation->hasChanged  = true;
            physicsComponent->body->SetTransform(
                b2Vec2(levelData.playerSpawnLocation.x,
                       levelData.playerSpawnLocation.y),
                physicsComponent->body->GetAngle());

            playerComponent->touchesWin = false;
        }

        if(playerComponent->touchesWin) {
            levelData.hasWon = true;
        }
    }
}
bool Scene::gameplayActive() {
    return !levelData.hasWon;
}

void Scene::resetLevel() {
    levelData.hasWon = false;
    resetPlayer();
}
void Scene::resetPlayer() {
    for(auto id : SceneView<PlayerComponent, Transformation, PhysicsComponent>(*this)) {
        auto* playerComponent  = getComponent<PlayerComponent>(id);
        auto* transformation   = getComponent<Transformation>(id);
        auto* physicsComponent = getComponent<PhysicsComponent>(id);

        *playerComponent = PlayerComponent();

        transformation->translation = levelData.playerSpawnLocation;
        transformation->hasChanged  = true;
        physicsComponent->body->SetTransform(
            b2Vec2(levelData.playerSpawnLocation.x,
                   levelData.playerSpawnLocation.y),
            physicsComponent->body->GetAngle());

        playerComponent->touchesWin = false;
    }
}
