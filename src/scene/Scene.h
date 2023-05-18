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

template <typename... ComponentTypes>
struct SceneView;

class Scene {
private:
    std::vector<Entity> entities;
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

    std::vector<Entity> &getEntities();

    template<typename T>
    T *assign(EntityId entityId) {
        ComponentTypeId componentTypeId = getComponentTypeId<T>();

        if (componentPools.find(componentTypeId) == componentPools.end()) {
            componentPools[componentTypeId] = std::move(ComponentPool(sizeof(T)));
        }

        ComponentPool &pool = componentPools[componentTypeId];
        ComponentId componentId = pool.getNewComponentId();

        pool.mapComponent(entityId, componentId);
        entities[entityId].componentMask.set(componentTypeId);

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

template <typename... ComponentTypes>
struct SceneView {
    Scene *scene {nullptr};
    ComponentMask componentMask;
    bool all {false};

    SceneView(Scene &scene): scene(&scene)
    {
        auto numComponents = sizeof...(ComponentTypes);
        if (numComponents == 0) {
            all = true;
        } else {
            ComponentTypeId typeIds[] = { getComponentTypeId<ComponentTypes>() ..., 0 };

            for (size_t i = 0; i < numComponents; i++) {
                componentMask.set(typeIds[i]);
            }
        }
    }

    struct Iterator
    {
        Scene* scene;
        EntityId index;
        ComponentMask componentMask;
        bool all{ false };

        Iterator(Scene *scene, EntityId entityIndex, ComponentMask mask, bool all)
                : scene(scene), index(entityIndex), componentMask(mask), all(all) { }

        EntityId operator*() const {
            return index;
        }

        bool operator==(const Iterator& other) const
        {
            return index == other.index;
        }

        bool operator!=(const Iterator& other) const
        {
            return index != other.index;
        }

        Iterator& operator++()
        {
            auto entities = scene->getEntities();
            do {
                index++;
            } while(index < entities.size() && (!entityValid(entities[index], all, componentMask)));
            return *this;
        }
    };



    Iterator begin() const
    {
        int firstIndex = 0;
        auto entities = scene->getEntities();
        while (firstIndex < entities.size() && !entityValid(entities[firstIndex], all, componentMask)) {
            firstIndex++;
        }

        return Iterator(scene, firstIndex, componentMask, all);
    }

    Iterator end() const
    {
        return Iterator(scene, scene->getEntities().size(), componentMask, all);
    }

};

#endif //GRAPHICSPRAKTIKUM_SCENE_H
