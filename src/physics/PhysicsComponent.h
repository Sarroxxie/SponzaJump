#ifndef GRAPHICSPRAKTIKUM_PHYSICSCOMPONENT_H
#define GRAPHICSPRAKTIKUM_PHYSICSCOMPONENT_H

#include "box2d/box2d.h"

struct PhysicsComponent {
    b2Body *body = nullptr;
    bool dynamic = false;
};

#endif //GRAPHICSPRAKTIKUM_PHYSICSCOMPONENT_H
