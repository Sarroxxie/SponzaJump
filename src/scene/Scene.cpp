#include "Scene.h"
#include "rendering/RenderContext.h"
#include "physics/PhysicsComponent.h"
#include "game/PlayerComponent.h"
#include "rendering/host_device.h"
#include <glm/gtc/type_ptr.hpp>

Scene::Scene(VulkanBaseContext vulkanBaseContext, RenderContext& renderContext, Camera camera)
    : m_Camera(camera)
    , m_World(b2World(b2Vec2(0, -30.0)))
    , m_baseContext(vulkanBaseContext) {}

void Scene::cleanup() {
    for(auto pair : componentPools) {
        pair.second.clean();
    }

    for(auto& mesh : sceneData.meshes) {
        mesh.cleanup(m_baseContext);
    }

    for(auto& texture : sceneData.textures) {
        texture.cleanup(m_baseContext);
    }
}

/*
 * Adds a loaded object to the scene.
 */
void Scene::addObject(ModelLoader loader) {
    sceneData.meshes.insert(sceneData.meshes.end(), loader.meshes.begin(), loader.meshes.end());
    sceneData.meshParts.insert(sceneData.meshParts.end(), loader.meshParts.begin(), loader.meshParts.end());
    sceneData.textures.insert(sceneData.textures.end(), loader.textures.begin(), loader.textures.end());
    sceneData.materials.insert(sceneData.materials.end(), loader.materials.begin(), loader.materials.end());
    // sceneData.models.insert(sceneData.models.end(), loader.models.begin(), loader.models.end());
    // sceneData.instances.insert(sceneData.instances.end(), loader.instances.begin(), loader.instances.end());
}

ModelLoadingOffsets Scene::getModelLoadingOffsets() {
    ModelLoadingOffsets offsets;
    offsets.meshesOffset    = sceneData.meshes.size();
    offsets.meshPartsOffset = sceneData.meshParts.size();
    offsets.texturesOffset  = sceneData.textures.size();
    offsets.materialsOffset = sceneData.materials.size();
    // offsets.modelsOffset    = sceneData.models.size();
    // offsets.instancesOffset = sceneData.instances.size();
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
    ImGui::Begin("Scene");

    ImGui::SliderFloat3("Camera Pos",
                        glm::value_ptr(m_Camera.getWorldPosRef()), 0, 100);
    ImGui::SliderFloat3("Camera Dir",
                        glm::value_ptr(m_Camera.getViewDirRef()), -glm::pi<float>(), glm::pi<float>());

    ImGui::Checkbox("Lock Camera to Player", &renderContext.imguiData.lockCamera);


    ImGui::Spacing();

    ImGui::SliderFloat3(
        "Light Camera Pos",
        glm::value_ptr(renderContext.renderPasses.shadowPass.lightCamera.getWorldPosRef()),
        -5, 5);
    ImGui::SliderFloat3(
        "Light Camera Dir",
        glm::value_ptr(renderContext.renderPasses.shadowPass.lightCamera.getViewDirRef()),
        -1, 1);

    ImGui::Checkbox("Visualize Shadow Buffer", &renderContext.imguiData.visualizeShadowBuffer);

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
        // TODO this check is not only true if the player is actually grounded

        b2Vec2 newVel;
        newVel.x = movingRight ? speed : movingLeft ? -speed : 0;
        newVel.y = jumps ? 20 : linVel.y;

        physicsComponent->body->SetLinearVelocity(newVel);
    }
}

void Scene::setInputController(InputController* inputController) {
    m_InputController = inputController;
}
void Scene::doCameraUpdate(const RenderContext& renderContext) {
    for(auto id : SceneView<PlayerComponent, Transformation>(*this)) {
        auto* transformation = getComponent<Transformation>(id);

        if(renderContext.imguiData.lockCamera) {
            auto prevPos = m_Camera.getWorldPos();

            m_Camera.setPosition(glm::vec3(transformation->translation.x,
                                           transformation->translation.y, prevPos.z));
            m_Camera.setLookAt(glm::vec3(transformation->translation.x,
                                         transformation->translation.y, 0));
        }
    }
}
