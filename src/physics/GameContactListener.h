#include <box2d/b2_world_callbacks.h>
#include "game/PlayerComponent.h"

#ifndef GRAPHICSPRAKTIKUM_GAMECONTACTLISTENER_H
#define GRAPHICSPRAKTIKUM_GAMECONTACTLISTENER_H

#define FIXTURE_ID_A 1
#define FIXTURE_ID_B 2

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
    b2Vec2 getWorldManifoldNormal(b2Contact *contact, int fixture_id);

    bool contactMakesGrounded(b2Contact *contact, int fixture_id);

    bool contactIsBelow(b2Contact *contact, int fixture_id);

    bool playerHitCategory(b2Contact *contact, int fixture_id, uint32 categoryBits);

    bool hasHazardBits(b2Fixture *fixture, uint32 categoryBits);
};


#endif //GRAPHICSPRAKTIKUM_GAMECONTACTLISTENER_H
