//
//

#include "Scene.h"
#include "rendering/RenderContext.h"

Scene::Scene(VulkanBaseContext vulkanBaseContext, RenderContext &renderContext, Camera camera)
        : m_Camera(camera), m_baseContext(vulkanBaseContext) {
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets(renderContext);
}

void Scene::cleanup() {
    vkDestroyBuffer(m_baseContext.device, uniformBuffer, nullptr);
    vkFreeMemory(m_baseContext.device, uniformBufferMemory, nullptr);

    vkDestroyDescriptorPool(m_baseContext.device, descriptorPool, nullptr);

    // TODO use SceneView
    for (size_t i = 0; i < entities.size(); i++) {
        if (!entities[i]) continue;

        ComponentPool &renderablePool = componentPools[getComponentTypeId<RenderableObject>()];

        if (renderablePool.hasComponent(i)) {
            auto *object = (RenderableObject *) renderablePool.getComponent(i);

            cleanRenderableObject(m_baseContext, *object);
        }
    }
}

Camera &Scene::getCameraRef() {
    return m_Camera;
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
    ImGui::SliderFloat("Camera Distance", &cameraDist, 0, 150);
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
        entities.push_back(true);
        return static_cast<EntityId>(entities.size() - 1);
    }
    EntityId id = freeEntities.back();
    freeEntities.pop_back();
    entities[id] = true;
    return id;
}

bool Scene::removeEntity(EntityId id) {
    // TODO add Version so that a reused entity does not have access to Components anymore ?

    if (entities.size() <= id || !entities[id]) {
        return false;
    }

    ComponentTypeId renderableCompId = getComponentTypeId<RenderableObject>();
    if (componentPools[renderableCompId].hasComponent(id)) {
        auto *object = (RenderableObject *) componentPools[renderableCompId].getComponent(id);
        cleanRenderableObject(m_baseContext, *object);
    }

    entities[id] = false;
    freeEntities.push_back(id);

    return true;
}

std::vector<bool> &Scene::getEntities() {
    return entities;
}

std::map<ComponentId, ComponentPool> *Scene::getComponentPools() {
    return &componentPools;
}