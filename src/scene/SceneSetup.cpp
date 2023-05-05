#include <algorithm>
#include "SceneSetup.h"
#include "vulkan/VulkanUtils.h"

RenderableObject createObject(VulkanBaseContext context, CommandContext commandContext, ObjectDef objectDef, glm::vec3 offset) {
    RenderableObject object;

    createSampleVertexBuffer(context, commandContext, objectDef, object, offset);
    createSampleIndexBuffer(context, commandContext, objectDef, object);

    return object;
}

void createSampleVertexBuffer(VulkanBaseContext &context, CommandContext &commandContext, ObjectDef objectDef, RenderableObject &object, glm::vec3 offset) {
    std::vector<Vertex> shiftedVertices(objectDef.vertices.size());

    object.offset = offset;

    std::transform(objectDef.vertices.begin(),
                   objectDef.vertices.end(),
                   shiftedVertices.begin(),
                   [offset](Vertex v) -> Vertex { return {v.pos + offset, v.color}; });

    object.verticesCount = shiftedVertices.size();

    VkDeviceSize bufferSize = sizeof(shiftedVertices[0]) * object.verticesCount;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(context,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer,
                 stagingBufferMemory);

    void *data;
    vkMapMemory(context.device, stagingBufferMemory, 0, bufferSize, 0, &data);

    // We use Host Coherent Memory to make sure data is synchronized, could also manually flush Memory Ranges
    memcpy(data, shiftedVertices.data(), (size_t) bufferSize);
    vkUnmapMemory(context.device, stagingBufferMemory);

    createBuffer(context,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 object.vertexBuffer,
                 object.vertexBufferMemory);

    copyBuffer(context, commandContext, stagingBuffer, object.vertexBuffer, bufferSize);

    vkDestroyBuffer(context.device, stagingBuffer, nullptr);
    vkFreeMemory(context.device, stagingBufferMemory, nullptr);
}

void createSampleIndexBuffer(VulkanBaseContext &baseContext, CommandContext &commandContext, ObjectDef objectDef, RenderableObject &object) {

    object.indicesCount = objectDef.indices.size();

    VkDeviceSize bufferSize = sizeof(objectDef.indices[0]) * object.indicesCount;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(baseContext,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer,
                 stagingBufferMemory);

    void *data;
    vkMapMemory(baseContext.device, stagingBufferMemory, 0, bufferSize, 0, &data);

    // We use Host Coherent Memory to make sure data is synchronized, could also manually flush Memory Ranges
    memcpy(data, objectDef.indices.data(), (size_t) bufferSize);
    vkUnmapMemory(baseContext.device, stagingBufferMemory);

    createBuffer(baseContext,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 object.indexBuffer,
                 object.indexBufferMemory);

    copyBuffer(baseContext, commandContext, stagingBuffer, object.indexBuffer, bufferSize);

    vkDestroyBuffer(baseContext.device, stagingBuffer, nullptr);
    vkFreeMemory(baseContext.device, stagingBufferMemory, nullptr);
}