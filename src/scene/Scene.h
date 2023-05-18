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


/*
class Scene;

template <typename... ComponentTypes>
struct SceneView {
    Scene *scene {nullptr};
    std::vector<ComponentTypeId> componentTypeIds;
    bool all {false};

    SceneView(Scene &scene): scene(&scene)
    {
        auto numComponents = sizeof...(ComponentTypes);
        if (numComponents == 0) {
            all = true;
        } else {
            ComponentTypeId typeIds[] = { getComponentTypeId<ComponentTypes>() ..., 0 };

            componentTypeIds.resize(numComponents);
            for (size_t i = 0; i < numComponents; i++) {
                componentTypeIds[i] = typeIds[i];
            }
        }
    }

    struct Iterator
    {
        EntityId index;
        Scene* pScene;
        std::vector<ComponentTypeId> componentTypeIds;
        bool all{ false };

        Iterator(Scene *scene, EntityId entityId, ) {

        }

        EntityId operator*() const {

        }

        bool operator==(const Iterator& other) const
        {
            // Compare two iterators
        }

        bool operator!=(const Iterator& other) const
        {
            // Similar to above
        }

        Iterator& operator++()
        {
            // Move the iterator forward
        }
    };

    const Iterator begin() const
    {
        // Give an iterator to the beginning of this view
    }

    const Iterator end() const
    {
        // Give an iterator to the end of this view
    }

};

 */

class Scene {
private:
    std::vector<bool> entities;
    std::vector<EntityId> freeEntities;

    std::map<ComponentId, ComponentPool> componentPools;

    Camera m_Camera;

    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    void* uniformBufferMapped;

    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;

    VulkanBaseContext m_baseContext;

public:
    Scene(VulkanBaseContext vulkanBaseContext, RenderContext &renderContext, Camera camera = Camera());

    // ECS ---
    EntityId addEntity();

    bool removeEntity(EntityId id);

    std::vector<bool> &getEntities();

    std::map<ComponentId, ComponentPool> *getComponentPools();

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
    template<typename... Types>
    SceneView<Types> getSceneView() {

    }
     */

    // -------


    Camera &getCameraRef();

    void cleanup();

    void *getUniformBufferMapping();

    VkDescriptorSet *getDescriptorSet();

    void registerSceneImgui();

    float currentAngleY = 0;
    float currentAngleX = 0;

    float cameraAngleY = 0;
    float cameraDist = 45;

private:
    void createUniformBuffers();

    void createDescriptorPool();

    void createDescriptorSets(RenderContext &renderContext);

};

#endif //GRAPHICSPRAKTIKUM_SCENE_H
