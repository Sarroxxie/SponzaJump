#include <algorithm>
#include "SceneSetup.h"
#include "vulkan/VulkanUtils.h"
#include "physics/PhysicsComponent.h"

MeshComponent createMeshComponent(MeshComponent *component, VulkanBaseContext context, CommandContext commandContext, ObjectDef objectDef) {
    createSampleVertexBuffer(context, commandContext, objectDef, component);
    createSampleIndexBuffer(context, commandContext, objectDef, component);
}

void createSampleVertexBuffer(VulkanBaseContext &context, CommandContext &commandContext, ObjectDef objectDef, MeshComponent *object) {
    object->verticesCount = objectDef.vertices.size();

    VkDeviceSize bufferSize = sizeof(objectDef.vertices[0]) * object->verticesCount;

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
    memcpy(data, objectDef.vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(context.device, stagingBufferMemory);

    createBuffer(context,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 object->vertexBuffer,
                 object->vertexBufferMemory);

    copyBuffer(context, commandContext, stagingBuffer, object->vertexBuffer, bufferSize);

    vkDestroyBuffer(context.device, stagingBuffer, nullptr);
    vkFreeMemory(context.device, stagingBufferMemory, nullptr);
}

void createSampleIndexBuffer(VulkanBaseContext &baseContext, CommandContext &commandContext, ObjectDef objectDef, MeshComponent *object) {

    object->indicesCount = objectDef.indices.size();

    VkDeviceSize bufferSize = sizeof(objectDef.indices[0]) * object->indicesCount;

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
                 object->indexBuffer,
                 object->indexBufferMemory);

    copyBuffer(baseContext, commandContext, stagingBuffer, object->indexBuffer, bufferSize);

    vkDestroyBuffer(baseContext.device, stagingBuffer, nullptr);
    vkFreeMemory(baseContext.device, stagingBufferMemory, nullptr);
}

EntityId addPhysicsEntity(Scene &scene,
                          VulkanBaseContext context,
                          CommandContext commandContext,
                          ObjectDef objectDef,
                          Transformation transformation,
                          glm::vec3 halfSize,
                          bool dynamic,
                          bool fixedRotation) {

    EntityId dynamicEntity = scene.addEntity();

    auto *meshComponent = scene.assign<MeshComponent>(dynamicEntity);

    createMeshComponent(meshComponent, context, commandContext, objectDef);

    auto *pDynamicTransform = scene.assign<Transformation>(dynamicEntity);
    *pDynamicTransform = transformation;

    auto *pDynamicPhysicsComponent = scene.assign<PhysicsComponent>(dynamicEntity);

    b2BodyDef bodyDef;
    if (dynamic)
        bodyDef.type = b2_dynamicBody;
    if (fixedRotation)
        bodyDef.fixedRotation = true;
    bodyDef.position.Set(transformation.translation.x, transformation.translation.y);
    bodyDef.angle = transformation.rotation.z;
    pDynamicPhysicsComponent->body = scene.getWorld().CreateBody(&bodyDef);
    pDynamicPhysicsComponent->dynamic = true;

    b2PolygonShape dynamicBox;
    dynamicBox.SetAsBox(halfSize.x, halfSize.y);

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &dynamicBox;
    fixtureDef.density = 1.0f;
    fixtureDef.friction = 0.3f;

    pDynamicPhysicsComponent->body->CreateFixture(&fixtureDef);
}