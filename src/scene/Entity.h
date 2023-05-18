#ifndef GRAPHICSPRAKTIKUM_ENTITY_H
#define GRAPHICSPRAKTIKUM_ENTITY_H

#include <cstdint>
#include <bitset>

inline const int MAX_COMPONENT_TYPES = 32;
typedef int EntityId;

typedef std::bitset<MAX_COMPONENT_TYPES> ComponentMask;

struct Entity {
    bool active {true};
    ComponentMask componentMask;
};

#endif //GRAPHICSPRAKTIKUM_ENTITY_H
