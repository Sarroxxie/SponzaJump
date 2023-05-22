#include <algorithm>
#include "SceneSetup.h"
#include "vulkan/VulkanUtils.h"
#include "physics/PhysicsComponent.h"
#include "game/PlayerComponent.h"

void
createSamplePhysicsScene(const ApplicationVulkanContext &context, Scene &scene, GameContactListener &contactListener) {
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

    glm::vec3 slopeHalfDims = glm::vec3(1, 20, 1);
    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext,
                     getCuboid(slopeHalfDims),
                     {glm::vec3(45, 10, 0), glm::vec3(0, 0, glm::half_pi<float>() / 2), glm::vec3(1)},
                     slopeHalfDims,
                     false);

    glm::vec3 floatingBoxHalfDims(1, 1, 1);
    ObjectDef floatingStaticObstacles = getCuboid(floatingBoxHalfDims);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext,
                     floatingStaticObstacles,
                     {glm::vec3(-24, 4, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     floatingBoxHalfDims,
                     false);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext,
                     floatingStaticObstacles,
                     {glm::vec3(-15, 8, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     floatingBoxHalfDims,
                     false);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext,
                     floatingStaticObstacles,
                     {glm::vec3(0, 7, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     floatingBoxHalfDims,
                     false);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext,
                     floatingStaticObstacles,
                     {glm::vec3(13, 9, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     floatingBoxHalfDims,
                     false);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext,
                     floatingStaticObstacles,
                     {glm::vec3(15, 15, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     floatingBoxHalfDims,
                     false);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext,
                     floatingStaticObstacles,
                     {glm::vec3(5, 19, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     floatingBoxHalfDims,
                     false);

    glm::vec3 floatingBoxHalfDims2(1, 0.5, 1);
    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext,
                     getCuboid(floatingBoxHalfDims2),
                     {glm::vec3(-11, 18.5, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     floatingBoxHalfDims2,
                     false);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext,
                     getCuboid(floatingBoxHalfDims),
                     {glm::vec3(-27, 18, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     floatingBoxHalfDims,
                     false);

    EntityId playerEntity = addPlayerEntity(scene,
                                            context.baseContext,
                                            context.commandContext,
                                            getCuboid(floatingBoxHalfDims, glm::vec3(0.2, 0.2, 0.8)),
                                            {glm::vec3(30, 20, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                                            floatingBoxHalfDims,
                                            contactListener,
                                            true,
                                            true);

    auto *playerComponent = scene.assign<PlayerComponent>(playerEntity);

    contactListener.setPlayerComponent(playerComponent);

    /*
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
     */
}


void
createMeshComponent(MeshComponent *component, const VulkanBaseContext &context, const CommandContext &commandContext,
                    const ObjectDef &objectDef) {
    createSampleVertexBuffer(context, commandContext, objectDef, component);
    createSampleIndexBuffer(context, commandContext, objectDef, component);
}

void createSampleVertexBuffer(const VulkanBaseContext &context, const CommandContext &commandContext,
                              const ObjectDef &objectDef,
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

void createSampleIndexBuffer(const VulkanBaseContext &baseContext, const CommandContext &commandContext,
                             const ObjectDef &objectDef,
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
    fixtureDef.friction = 0.0f;

    pDynamicPhysicsComponent->body->CreateFixture(&fixtureDef);

    return dynamicEntity;
}

EntityId addPlayerEntity(Scene &scene, const VulkanBaseContext &context, const CommandContext &commandContext,
                         const ObjectDef &objectDef, Transformation transformation, glm::vec3 halfSize,
                         GameContactListener &contactListener,
                         bool dynamic, bool fixedRotation) {
    EntityId playerEntity = scene.addEntity();

    auto *meshComponent = scene.assign<MeshComponent>(playerEntity);

    createMeshComponent(meshComponent, context, commandContext, objectDef);

    auto *pDynamicTransform = scene.assign<Transformation>(playerEntity);
    *pDynamicTransform = transformation;

    auto *pDynamicPhysicsComponent = scene.assign<PhysicsComponent>(playerEntity);

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
    fixtureDef.friction = 0.0f;

    b2Fixture *fixture = pDynamicPhysicsComponent->body->CreateFixture(&fixtureDef);
    contactListener.setPlayerFixture(fixture);

    return playerEntity;
}
