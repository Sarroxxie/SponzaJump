#ifndef GRAPHICSPRAKTIKUM_SCENE_H
#define GRAPHICSPRAKTIKUM_SCENE_H

#include <vulkan/vulkan_core.h>
#include <vector>
#include "vulkan/ApplicationContext.h"
#include "SceneSetup.h"
#include "RenderableObject.h"
#include "Camera.h"
#include "rendering/RenderContext.h"

typedef uint32_t EntityId;

class Scene {
private:
    std::vector<bool> entities;
    std::vector<EntityId> freeEntities;

    std::vector<RenderableObject> objects;

    Camera m_Camera;

    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    void* uniformBufferMapped;

    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;

public:
    Scene(VulkanBaseContext vulkanBaseContext, RenderContext &renderContext, Camera camera = Camera());

    EntityId addEntity();

    bool removeEntity(EntityId id);

    void addObject(RenderableObject object);

    std::vector<RenderableObject> &getObjects();
    bool hasObject();

    Camera &getCameraRef();

    void cleanup(VulkanBaseContext &baseContext);

    void *getUniformBufferMapping();

    VkDescriptorSet *getDescriptorSet();

    void registerSceneImgui();

    float currentAngleY = 0;
    float currentAngleX = 0;

    float cameraAngleY = 0;
    float cameraDist = 45;

private:
    void createUniformBuffers(VulkanBaseContext vulkanBaseContext);

    void createDescriptorPool(VulkanBaseContext &baseContext);

    void createDescriptorSets(VulkanBaseContext &baseContext, RenderContext &renderContext);

};

#endif //GRAPHICSPRAKTIKUM_SCENE_H
