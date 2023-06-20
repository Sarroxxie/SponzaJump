#include "Scene.h"
#include "rendering/RenderContext.h"
#include "physics/PhysicsComponent.h"
#include "game/PlayerComponent.h"
#include "rendering/host_device.h"

Scene::Scene(VulkanBaseContext vulkanBaseContext, RenderContext &renderContext, Camera camera)
        : m_Camera(camera), m_World(b2World(b2Vec2(0, -30.0))), m_baseContext(vulkanBaseContext) {
}

void Scene::cleanup() {
        for (auto pair: componentPools) {
        pair.second.clean();
    }

    for (auto &mesh: meshes) {
        mesh.cleanup(m_baseContext);
    }

    for (auto &texture: textures) {
        texture.cleanup(m_baseContext);
    }
    cubemap.cleanup(m_baseContext);
}

/*
* Adds a loaded object to the scene.
*/
void Scene::addObject(ModelLoader loader) {
    meshes.insert(meshes.end(), loader.meshes.begin(), loader.meshes.end());
    meshParts.insert(meshParts.end(), loader.meshParts.begin(), loader.meshParts.end());
    textures.insert(textures.end(), loader.textures.begin(), loader.textures.end());
    materials.insert(materials.end(), loader.materials.begin(), loader.materials.end());
    models.insert(models.end(), loader.models.begin(), loader.models.end());
    instances.insert(instances.end(), loader.instances.begin(), loader.instances.end());
}

ModelLoadingOffsets Scene::getModelLoadingOffsets() {
    ModelLoadingOffsets offsets;
    offsets.meshesOffset    = meshes.size();
    offsets.meshPartsOffset = meshParts.size();
    offsets.texturesOffset  = textures.size();
    offsets.materialsOffset = materials.size();
    offsets.modelsOffset    = models.size();
    offsets.instancesOffset = instances.size();
    return offsets;
}


std::vector<Mesh> &Scene::getMeshes() {
    return meshes;
}

std::vector<MeshPart> &Scene::getMeshParts() {
    return meshParts;
}

std::vector<Texture> &Scene::getTextures() {
    return textures;
}

std::vector<Material> &Scene::getMaterials() {
    return materials;
}

std::vector<Model> &Scene::getModels() {
    return models;
}

std::vector<ModelInstance> &Scene::getInstances() {
    return instances;
}

CubeMap& Scene::getCubeMap() {
    return cubemap;
}

Camera &Scene::getCameraRef() {
    return m_Camera;
}

b2World &Scene::getWorld() {
    return m_World;
}

void Scene::registerSceneImgui() {
    ImGui::Begin("Scene");
    ImGui::SliderFloat("Object Angle X", &currentAngleX, 0, glm::two_pi<float>());
    ImGui::SliderFloat("Object Angle Y", &currentAngleY, 0, glm::two_pi<float>());
    ImGui::SliderFloat("Camera Angle Y", &cameraAngleY, 0, glm::two_pi<float>());
    ImGui::SliderFloat("Camera Distance", &cameraDist, 0, 100);
    ImGui::End();
}

// TODO take relevant descriptorsSetLayout info into RenderSetup

/*
void Scene::createMaterialsDescriptorSetLayout(RenderPassContext& renderPassContext) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    // materials buffer
    VkDescriptorSetLayoutBinding materialsBinding;
    materialsBinding.binding         = MaterialsBindings::eMaterials;
    materialsBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    materialsBinding.descriptorCount = 1;
    materialsBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings.push_back(materialsBinding);

    // texture samplers
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = MaterialsBindings::eTextures;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = static_cast<uint32_t>(this->textures.size());
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings.push_back(samplerLayoutBinding);

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings    = bindings.data();

    if(vkCreateDescriptorSetLayout(m_baseContext.device, &layoutInfo, nullptr,
                                   &renderPassContext.materialsDescriptorSetLayout)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create materials descriptor set layout!");
    }
}

void Scene::createMaterialsBufferDescriptorSet(RenderContext& renderContext) {
    createMaterialsDescriptorSetLayout(renderContext.renderPassContext);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &renderContext.renderPassContext.materialsDescriptorSetLayout;

    if(vkAllocateDescriptorSets(m_baseContext.device, &allocInfo, &materialsDescriptorSet)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    std::vector<VkWriteDescriptorSet> descriptorWrites;

    VkDescriptorBufferInfo materialsBufferInfo{};
    materialsBufferInfo.buffer = materialsBuffer;
    materialsBufferInfo.offset = 0;
    materialsBufferInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet materialsDescriptorWrite;
    materialsDescriptorWrite.sType  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    materialsDescriptorWrite.pNext  = nullptr;
    materialsDescriptorWrite.dstSet = materialsDescriptorSet;
    materialsDescriptorWrite.dstBinding      = MaterialsBindings::eMaterials;
    materialsDescriptorWrite.dstArrayElement = 0;
    materialsDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    materialsDescriptorWrite.descriptorCount = 1;
    materialsDescriptorWrite.pBufferInfo     = &materialsBufferInfo;

    descriptorWrites.push_back(materialsDescriptorWrite);

    // All texture samplers
    std::vector<VkDescriptorImageInfo> textureSamplers;
    for(auto& texture : textures) {
        textureSamplers.emplace_back(texture.descriptorInfo);
    }

    VkWriteDescriptorSet texturesWrite{};
    texturesWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    texturesWrite.dstSet          = materialsDescriptorSet;
    texturesWrite.dstBinding      = MaterialsBindings::eTextures;
    texturesWrite.dstArrayElement = 0;
    texturesWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    // TODO where to textureSamplers come from ?
    texturesWrite.descriptorCount = textureSamplers.size();
    texturesWrite.pImageInfo      = textureSamplers.data();

    descriptorWrites.push_back(texturesWrite);

    vkUpdateDescriptorSets(m_baseContext.device, static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
}
*/

EntityId Scene::addEntity() {
    if (freeEntities.empty()) {
        entities.emplace_back();
        return static_cast<EntityId>(entities.size() - 1);
    }
    EntityId id = freeEntities.back();
    freeEntities.pop_back();
    entities[id] = {true, ComponentMask()};
    return id;
}

bool Scene::removeEntity(EntityId id) {
    if (entities.size() <= id || !entities[id].active) {
        return false;
    }

    entities[id] = {false, ComponentMask()};
    freeEntities.push_back(id);

    return true;
}

std::vector<Entity> &Scene::getEntities() {
    return entities;
}

void Scene::doPhysicsUpdate(uint64_t deltaMillis) {
    float timeStep = static_cast<float>(deltaMillis) / 1000.0f;
    int32 velocityIterations = 6;
    int32 positionIterations = 2;
    m_World.Step(timeStep, velocityIterations, positionIterations);
    for (EntityId id: SceneView<Transformation, PhysicsComponent>(*this)) {
        auto *physicsComponent = getComponent<PhysicsComponent>(id);
        if (physicsComponent->dynamic) {
            auto *transform = getComponent<Transformation>(id);

            b2Vec2 newPos = physicsComponent->body->GetPosition();

            transform->translation = glm::vec3(newPos.x, newPos.y, transform->translation.z);
            transform->rotation.z = physicsComponent->body->GetAngle();

        }
    }
}

void Scene::handleUserInput() {
    if (m_InputController == nullptr) return;
    bool movingLeft = m_InputController->isPressed(GLFW_KEY_A);
    bool movingRight = m_InputController->isPressed(GLFW_KEY_D);

    bool wantsToJump = m_InputController->getSinglePress(GLFW_KEY_SPACE);

    float speed = 10;

    for (auto id: SceneView<PlayerComponent, PhysicsComponent>(*this)) {
        auto *physicsComponent = getComponent<PhysicsComponent>(id);
        auto *playerComponent = getComponent<PlayerComponent>(id);

        float notMovingEps = 1e-4;

        b2Vec2 linVel = physicsComponent->body->GetLinearVelocity();

        bool jumps = false;
        if (wantsToJump) {
            if (playerComponent->grounded) {
                jumps = true;
            } else if (playerComponent->canDoubleJump) {
                jumps = true;
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

void Scene::setInputController(InputController *inputController) {
    m_InputController = inputController;
}

void Scene::doCameraUpdate() {
    for (auto id: SceneView<PlayerComponent, Transformation>(*this)) {
        auto *transformation = getComponent<Transformation>(id);

        m_Camera.setPosition(glm::vec3(transformation->translation.x, transformation->translation.y, cameraDist));
        m_Camera.setLookAt(glm::vec3(transformation->translation.x, transformation->translation.y, 0));
    }
}
