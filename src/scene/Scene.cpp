//
//

#include "Scene.h"
#include "rendering/RenderContext.h"

Scene::Scene(VulkanBaseContext vulkanBaseContext, RenderContext &renderContext, Camera camera)
        : m_Camera(camera) {
    createUniformBuffers(vulkanBaseContext);
    createDescriptorPool(vulkanBaseContext);
    createDescriptorSets(vulkanBaseContext, renderContext);
}

void Scene::addObject(RenderableObject object) {
    objects.push_back(object);
}

void Scene::cleanup(VulkanBaseContext &baseContext) {
    vkDestroyBuffer(baseContext.device, uniformBuffer, nullptr);
    vkFreeMemory(baseContext.device, uniformBufferMemory, nullptr);

    vkDestroyDescriptorPool(baseContext.device, descriptorPool, nullptr);

    for (auto &object: objects) {
        cleanRenderableObject(baseContext, object);
    }
}

std::vector<RenderableObject> &Scene::getObjects() {
    return objects;
}

bool Scene::hasObject() {
    return objects.size() > 0;
}

Camera &Scene::getCameraRef() {
    return m_Camera;
}

void Scene::createUniformBuffers(VulkanBaseContext vulkanBaseContext) {
    VkDeviceSize bufferSize = sizeof(SceneTransform);

    createBuffer(vulkanBaseContext, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer,
                 uniformBufferMemory);

    vkMapMemory(vulkanBaseContext.device, uniformBufferMemory, 0, bufferSize, 0, &uniformBufferMapped);
}

void Scene::createDescriptorPool(VulkanBaseContext &baseContext) {
    std::array<VkDescriptorPoolSize, 1> poolSizes{};

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(baseContext.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void Scene::createDescriptorSets(VulkanBaseContext &baseContext, RenderContext &renderContext) {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &renderContext.renderPassContext.descriptorSetLayout;

    if (vkAllocateDescriptorSets(baseContext.device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
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

    vkUpdateDescriptorSets(baseContext.device, 1, &descriptorWrite, 0, nullptr);


}

void *Scene::getUniformBufferMapping() {
    return uniformBufferMapped;
}

VkDescriptorSet *Scene::getDescriptorSet() {
    return &descriptorSet;
}
