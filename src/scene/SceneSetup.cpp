#include <algorithm>
#include "SceneSetup.h"
#include "vulkan/VulkanUtils.h"
#include "physics/PhysicsComponent.h"

void createSamplePhysicsScene(const ApplicationVulkanContext &context, Scene &scene) {
    glm::vec3 groundHalfDims = glm::vec3(30, 1, 1);
    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext,
                     getCuboid(groundHalfDims),
                     Transformation(),
                     groundHalfDims,
                     false);

    glm::vec3 sideHalfDims = glm::vec3(1, 10, 1);
    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext,
                     getCuboid(sideHalfDims),
                     {glm::vec3(-29, 9, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     sideHalfDims,
                     false);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext,
                     getCuboid(sideHalfDims),
                     {glm::vec3(29, 9, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     sideHalfDims,
                     false);

    glm::vec3 floatingBoxHalfDims(1, 1, 1);
    ObjectDef floatingStaticObstacles = getCuboid(floatingBoxHalfDims);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext,
                     floatingStaticObstacles,
                     {glm::vec3(-17, 3, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     floatingBoxHalfDims,
                     false);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext,
                     floatingStaticObstacles,
                     {glm::vec3(-2, 5, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     floatingBoxHalfDims,
                     false);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext,
                     floatingStaticObstacles,
                     {glm::vec3(7, 7, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     floatingBoxHalfDims,
                     false);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext,
                     floatingStaticObstacles,
                     {glm::vec3(18, 4, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     floatingBoxHalfDims,
                     false);

    int numDynamicObjects = 30;
    for (int i = 0; i < numDynamicObjects; i++) {
        glm::vec3 halfSize = glm::vec3(1, 1, 1);
        addPhysicsEntity(scene,
                         context.baseContext,
                         context.commandContext,
                         getCuboid(halfSize, glm::vec3(
                                 static_cast<float>(i + 1) / (static_cast<float>(numDynamicObjects) + 1.0f))),
                         {glm::vec3(-15 + ((static_cast<float>(i) / static_cast<float>(numDynamicObjects)) * 30),
                                    20 + 5 * i, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                         halfSize,
                         true,
                         false);
    }
}


void createMeshComponent(MeshComponent *component, const VulkanBaseContext &context, const CommandContext &commandContext,
                                  const ObjectDef &objectDef) {
    createSampleVertexBuffer(context, commandContext, objectDef, component);
    createSampleIndexBuffer(context, commandContext, objectDef, component);
}

void createSampleVertexBuffer(const VulkanBaseContext &context, const CommandContext &commandContext, const ObjectDef &objectDef,
                              MeshComponent *object) {
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

void createSampleIndexBuffer(const VulkanBaseContext &baseContext, const CommandContext &commandContext, const ObjectDef &objectDef,
                             MeshComponent *object) {

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
                          const VulkanBaseContext &context,
                          const CommandContext &commandContext,
                          const ObjectDef &objectDef,
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

    return dynamicEntity;
}