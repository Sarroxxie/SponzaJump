#include "SceneSetup.h"
#include "vulkan/VulkanUtils.h"

RenderableObject createSampleObject(VulkanBaseContext context, CommandContext commandContext) {
    RenderableObject object;

    createSampleVertexBuffer(context, commandContext, object);
    createSampleIndexBuffer(context, commandContext, object);

    return object;
}

void createSampleVertexBuffer(VulkanBaseContext &context, CommandContext &commandContext, RenderableObject &object) {
    const std::vector<Vertex> vertices({
                                               {{0.0f, -0.5f, 0.5}, {1.0f, 0.0f, 0.0f}},
                                               {{0.5f, 0.5f, 0}, {0.0f, 1.0f, 0.0f}},
                                               {{-0.5f, 0.5f, 0.1}, {0.0f, 0.0f, 1.0f}}
                                       });

    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

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
    memcpy(data, vertices.data(), (size_t) bufferSize);
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

void createSampleIndexBuffer(VulkanBaseContext &baseContext, CommandContext &commandContext, RenderableObject &object) {
    const std::vector<uint32_t> indices({0, 1, 2});

    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

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
    memcpy(data, indices.data(), (size_t) bufferSize);
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