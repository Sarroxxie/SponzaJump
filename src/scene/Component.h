#ifndef GRAPHICSPRAKTIKUM_COMPONENT_H
#define GRAPHICSPRAKTIKUM_COMPONENT_H

#include <vector>
#include <cstdint>
#include <set>
#include <cstring>
#include <stdexcept>
#include "Entity.h"
#include "vulkan/ApplicationContext.h"


typedef int ComponentTypeId;
typedef int ComponentId;

extern int s_componentCounter;

template <class T>
int getComponentTypeId()
{
    // can only have MAX_COMPONENT_TYPES different types of components
    // can change that constant in Entity.h if needed
    assert(s_componentCounter < MAX_COMPONENT_TYPES - 1);
    static int s_componentId = s_componentCounter++;
    return s_componentId;
}

struct ComponentPool {
    std::vector<ComponentId> entityMapping;

    char *data;
    std::set<ComponentId> freeComponents;

    uint32_t componentSize;
    size_t currentSize;

    ComponentPool() {
        componentSize = 1;
        currentSize = 0;
        data = nullptr;
    };

    explicit ComponentPool(uint32_t componentSize, size_t initialSize = 16)
        : componentSize(componentSize)
    {
        data = new char[initialSize * componentSize];
        currentSize = initialSize;
        for (int i = 0; i < initialSize; i++) {
            freeComponents.insert(i);
        }
    }

    /*
    ComponentPool(ComponentPool &&other) noexcept {
        componentSize = other.componentSize;
        currentSize = other.currentSize;

        data = other.data;
        other.data = nullptr;

        freeComponents = std::move(other.freeComponents);
        entityMapping = std::move(other.entityMapping);
    }

    ComponentPool &operator= (ComponentPool &&other) noexcept {
        if (this == &other) return *this;

        delete []data;

        componentSize = other.componentSize;
        currentSize = other.currentSize;

        data = other.data;
        other.data = nullptr;

        freeComponents = std::move(other.freeComponents);
        entityMapping = std::move(other.entityMapping);

        return *this;
    }
    */

    void clean() {
        delete []data;
    }

    ComponentId getNewComponentId() {
        if (freeComponents.empty()) {
            char *newData = new char[componentSize * currentSize * 2];
            memcpy(newData, data, currentSize * componentSize);

            delete []data;
            data = newData;
            for (int i = static_cast<ComponentId>(currentSize); i < currentSize * 2; i++) {
                freeComponents.insert(static_cast<ComponentId>(i));
            }
            currentSize *= 2;
        }

        auto freeComponent = freeComponents.begin();
        ComponentId id = *freeComponent;

        freeComponents.erase(freeComponent);
        return id;
    }

    bool freeComponent(ComponentId id) {
        if (currentSize <= id) return false;

        memset(data + id * componentSize, 0, componentSize);
        // clean component here ?

        freeComponents.insert(id);
    }

    bool mapComponent(EntityId entityId, ComponentId componentId) {
        if (currentSize <= componentId) return false;

        if (entityMapping.size() <= entityId) entityMapping.resize(entityId + 1, -1);
        entityMapping[entityId] = componentId;

        return true;
    }

    bool removeMapping(EntityId entityId) {
        if (entityMapping.size() <= entityId) return false;

        entityMapping[entityId] = -1;
    }

    bool hasComponent(EntityId entityId) {
        return entityMapping.size() > entityId && entityMapping[entityId] != -1;
    }

    char *getComponent(EntityId entityId) {
        if (entityMapping.size() <= entityId) throw std::runtime_error("Entity Id not found");

        int index = entityMapping[entityId];
        if (index == -1) throw std::runtime_error("Entity does not have a component of this type");

        if (currentSize <= index) throw std::runtime_error("invalid mapping encountered");

        return data + index * componentSize;
    }
};

#endif //GRAPHICSPRAKTIKUM_COMPONENT_H