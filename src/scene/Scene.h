#ifndef GRAPHICSPRAKTIKUM_SCENE_H
#define GRAPHICSPRAKTIKUM_SCENE_H

#include <vulkan/vulkan_core.h>
#include <vector>
#include <map>
#include <set>
#include "vulkan/ApplicationContext.h"
#include "RenderableObject.h"
#include "Camera.h"
#include "rendering/RenderContext.h"
#include "Entity.h"
#include "Component.h"
#include "box2d/box2d.h"
#include "input/InputController.h"
#include "Model.h"
#include "ModelLoader.h"
#include "SceneData.h"
#include "LevelData.h"

template <typename... ComponentTypes>
struct SceneView;

class Scene
{
  private:
    std::vector<Entity>   entities;
    std::vector<EntityId> freeEntities;

    std::map<ComponentTypeId, ComponentPool> componentPools;

    SceneData sceneData;
    LevelData levelData;

    Camera m_Camera;
    float  cameraOffsetY = 3.0f;

    b2World m_World;

    InputController* m_InputController = nullptr;

    ApplicationVulkanContext& m_Context;

  public:
    Scene(ApplicationVulkanContext& vulkanContext,
          Camera                    camera = Camera(glm::vec3(0, 0, 32)));

    // ECS ---
    EntityId addEntity();

    bool removeEntity(EntityId id);

    std::vector<Entity>& getEntities();

    template <typename T>
    T* assign(EntityId entityId) {
        ComponentTypeId componentTypeId = getComponentTypeId<T>();

        if(componentPools.find(componentTypeId) == componentPools.end()) {
            componentPools[componentTypeId] = std::move(ComponentPool(sizeof(T)));
        }

        ComponentPool& pool        = componentPools[componentTypeId];
        ComponentId    componentId = pool.getNewComponentId();

        pool.mapComponent(entityId, componentId);
        entities[entityId].componentMask.set(componentTypeId);

        T* component = (T*)pool.getComponent(entityId);
        *component   = T();

        return component;
    }

    template <typename T>
    T* getComponent(EntityId entityId) {
        ComponentTypeId componentTypeId = getComponentTypeId<T>();

        if(componentPools.find(componentTypeId) == componentPools.end()) {
            return nullptr;
        }

        return (T*)componentPools[componentTypeId].getComponent(entityId);
    }
    // -------

    ModelLoadingOffsets getModelLoadingOffsets();

    SceneData& getSceneData();
    LevelData& getLevelData();

    Camera& getCameraRef();
    void    doCameraUpdate(RenderContext& renderContext);

    b2World& getWorld();
    void     doPhysicsUpdate(uint64_t deltaMillis);

    void doGameplayUpdate();
    bool gameplayActive();

    void setInputController(InputController* inputController);
    void handleUserInput();

    void cleanup();
    void registerSceneImgui(RenderContext& renderContext);
    void registerWinDialog();

  private:

    void resetLevel();
    void resetPlayer();
};

template <typename... ComponentTypes>
struct SceneView
{
    Scene*        scene{nullptr};
    ComponentMask componentMask;
    bool          all{false};

    SceneView(Scene& scene)
        : scene(&scene) {
        auto numComponents = sizeof...(ComponentTypes);
        if(numComponents == 0) {
            all = true;
        } else {
            ComponentTypeId typeIds[] = {getComponentTypeId<ComponentTypes>()..., 0};

            for(size_t i = 0; i < numComponents; i++) {
                componentMask.set(typeIds[i]);
            }
        }
    }

    struct Iterator
    {
        Scene*        scene;
        EntityId      index;
        ComponentMask componentMask;
        bool          all{false};

        Iterator(Scene* scene, EntityId entityIndex, ComponentMask mask, bool all)
            : scene(scene)
            , index(entityIndex)
            , componentMask(mask)
            , all(all) {}

        EntityId operator*() const { return index; }

        bool operator==(const Iterator& other) const {
            return index == other.index;
        }

        bool operator!=(const Iterator& other) const {
            return index != other.index;
        }

        Iterator& operator++() {
            auto entities = scene->getEntities();
            do {
                index++;
            } while(index < entities.size()
                    && (!entityValid(entities[index], all, componentMask)));
            return *this;
        }
    };


    Iterator begin() const {
        int  firstIndex = 0;
        auto entities   = scene->getEntities();
        while(firstIndex < entities.size()
              && !entityValid(entities[firstIndex], all, componentMask)) {
            firstIndex++;
        }

        return Iterator(scene, firstIndex, componentMask, all);
    }

    Iterator end() const {
        return Iterator(scene, scene->getEntities().size(), componentMask, all);
    }
};

#endif  // GRAPHICSPRAKTIKUM_SCENE_H
