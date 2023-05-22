#include <box2d/b2_world_callbacks.h>
#include "game/PlayerComponent.h"

#ifndef GRAPHICSPRAKTIKUM_GAMECONTACTLISTENER_H
#define GRAPHICSPRAKTIKUM_GAMECONTACTLISTENER_H


class GameContactListener : b2ContactListener {

private:
    b2Fixture *playerFixture;
    PlayerComponent *playerComponent;


public:
    void BeginContact(b2Contact* contact) override;

    void EndContact(b2Contact* contact) override;

    void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override;

    void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override;

    void setPlayerFixture(b2Fixture *fixture);
    void setPlayerComponent(PlayerComponent *component);

private:
    bool contactIsBelow(b2Contact *contact, int fixture_id);

};


#endif //GRAPHICSPRAKTIKUM_GAMECONTACTLISTENER_H
