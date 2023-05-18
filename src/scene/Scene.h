#ifndef GRAPHICSPRAKTIKUM_SCENE_H
#define GRAPHICSPRAKTIKUM_SCENE_H

#include <vulkan/vulkan_core.h>
#include <vector>
#include <map>
#include <set>
#include "vulkan/ApplicationContext.h"
#include "SceneSetup.h"
#include "RenderableObject.h"
#include "Camera.h"
#include "rendering/RenderContext.h"
#include "Entity.h"
#include "Component.h"


class Scene {
private:
    std::vector<bool> entities;
    std::vector<EntityId> freeEntities;

    std::map<ComponentId, ComponentPool> componentPools;

    std::vector<RenderableObject> objects;

    Camera m_Camera;

    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    void* uniformBufferMapped;

    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;

public:
    Scene(VulkanBaseContext vulkanBaseContext, RenderContext &renderContext, Camera camera = Camera());

    // ECS ---
    EntityId addEntity();

    bool removeEntity(EntityId id);

    template<typename T>
    T *assign(EntityId entityId) {
        ComponentTypeId componentTypeId = getComponentTypeId<T>();

        if (componentPools.find(componentTypeId) == componentPools.end()) {
            componentPools[componentTypeId] = std::move(ComponentPool(sizeof(T)));
        }

        ComponentPool &pool = componentPools[componentTypeId];
        ComponentId componentId = pool.getNewComponentId();

        pool.mapComponent(entityId, componentId);

        return (T*) pool.getComponent(entityId);
    }

    template<typename T>
    T *getComponent(EntityId entityId) {
        ComponentTypeId componentTypeId = getComponentTypeId<T>();

        if (componentPools.find(componentTypeId) == componentPools.end()) {
            return nullptr;
        }

        return (T*) componentPools[componentTypeId].getComponent(entityId);
    }

    /*
    // TODO proper SceneView

    template<typename... Types>
    SceneView getSceneView();
    // -------
    */

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
