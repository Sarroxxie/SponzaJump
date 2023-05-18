#ifndef GRAPHICSPRAKTIKUM_ENTITY_H
#define GRAPHICSPRAKTIKUM_ENTITY_H

#include <cstdint>
#include <bitset>


inline const int MAX_COMPONENT_TYPES = 32;
typedef int EntityId;

typedef std::bitset<MAX_COMPONENT_TYPES> ComponentMask;

// Entity Component System built based on tutorial
// https://www.david-colson.com/2020/02/09/making-a-simple-ecs.html
struct Entity {
    bool active {true};
    ComponentMask componentMask {ComponentMask ()};
};

inline bool entityValid(Entity &entity, bool allComponents, ComponentMask mask) {
    return entity.active && (allComponents | mask == (mask & entity.componentMask));
}

#endif //GRAPHICSPRAKTIKUM_ENTITY_H
