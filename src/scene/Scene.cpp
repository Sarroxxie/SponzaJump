#include "Scene.h"
#include "rendering/RenderContext.h"
#include "physics/PhysicsComponent.h"
#include "game/PlayerComponent.h"

Scene::Scene(VulkanBaseContext vulkanBaseContext, RenderContext &renderContext, Camera camera)
        : m_Camera(camera), m_World(b2World(b2Vec2(0, -30.0))), m_baseContext(vulkanBaseContext) {
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets(renderContext);
}

void Scene::cleanup() {
    vkDestroyBuffer(m_baseContext.device, uniformBuffer, nullptr);
    vkFreeMemory(m_baseContext.device, uniformBufferMemory, nullptr);

    vkDestroyDescriptorPool(m_baseContext.device, descriptorPool, nullptr);

    ComponentPool &meshComponentPool = componentPools[getComponentTypeId<MeshComponent>()];

    for (EntityId id: SceneView<MeshComponent>(*this)) {
        auto *object = (MeshComponent *) meshComponentPool.getComponent(id);

        cleanMeshObject(m_baseContext, *object);
    }

    for (auto pair: componentPools) {
        pair.second.clean();
    }
}

Camera &Scene::getCameraRef() {
    return m_Camera;
}

b2World &Scene::getWorld() {
    return m_World;
}

void Scene::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(SceneTransform);

    createBuffer(m_baseContext, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer,
                 uniformBufferMemory);

    vkMapMemory(m_baseContext.device, uniformBufferMemory, 0, bufferSize, 0, &uniformBufferMapped);
}

void Scene::registerSceneImgui() {
    ImGui::Begin("Scene");
    ImGui::SliderFloat("Object Angle X", &currentAngleX, 0, glm::two_pi<float>());
    ImGui::SliderFloat("Object Angle Y", &currentAngleY, 0, glm::two_pi<float>());
    ImGui::SliderFloat("Camera Angle Y", &cameraAngleY, 0, glm::two_pi<float>());
    ImGui::SliderFloat("Camera Distance", &cameraDist, 0, 900);
    ImGui::End();
}

void Scene::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 1> poolSizes{};

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(m_baseContext.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void Scene::createDescriptorSets(RenderContext &renderContext) {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &renderContext.renderPassContext.descriptorSetLayout;

    if (vkAllocateDescriptorSets(m_baseContext.device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(SceneTransform);

    /*
    std::array<VkDescriptorImageInfo, 2> imageInfos{};
    imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfos[0].imageView = texture->getImageView();
    imageInfos[0].sampler = textureSampler;

    imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfos[1].imageView = texture2->getImageView();
    imageInfos[1].sampler = textureSampler;

    std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
        */



    VkWriteDescriptorSet descriptorWrite;
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.pNext = nullptr;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    /*
    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptorSets[i];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//            descriptorWrites[1].descriptorCount = 1;
//            descriptorWrites[1].pImageInfo = &imageInfo;

    descriptorWrites[1].descriptorCount = 2;
    descriptorWrites[1].pImageInfo = imageInfos.data();
    */

    vkUpdateDescriptorSets(m_baseContext.device, 1, &descriptorWrite, 0, nullptr);


}

void *Scene::getUniformBufferMapping() {
    return uniformBufferMapped;
}

VkDescriptorSet *Scene::getDescriptorSet() {
    return &descriptorSet;
}

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

    ComponentTypeId renderableCompId = getComponentTypeId<MeshComponent>();
    if (entities[id].componentMask.test(getComponentTypeId<MeshComponent>())) {
        auto *object = (MeshComponent *) componentPools[renderableCompId].getComponent(id);
        cleanMeshObject(m_baseContext, *object);
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
