//
//

#include <iostream>
#include "GameContactListener.h"
#include "box2d/b2_contact.h"
#include "glm/trigonometric.hpp"
#include "scene/LevelData.h"

void GameContactListener::BeginContact(b2Contact* contact) {
    // test if any fixture is player fixture -> early exit
    int fixture_id = 0;
    if(contact->GetFixtureA() == playerFixture)
        fixture_id = FIXTURE_ID_A;
    if(contact->GetFixtureB() == playerFixture)
        fixture_id = FIXTURE_ID_B;

    if(!fixture_id)
        return;

    if(playerHitCategory(contact, fixture_id, HAZARD_CATEGORY_BITS)) {
        playerComponent->touchesHazard = true;
    }

    if(playerHitCategory(contact, fixture_id, WIN_AREA_CATEGORY_BITS)) {
        playerComponent->touchesWin = true;
    }

    if(contactMakesGrounded(contact, fixture_id)) {
        playerComponent->grounded      = true;
        playerComponent->canDoubleJump = true;
    }
}

void GameContactListener::EndContact(b2Contact* contact) {
    int fixture_id = 0;
    if(contact->GetFixtureA() == playerFixture)
        fixture_id = FIXTURE_ID_A;
    if(contact->GetFixtureB() == playerFixture)
        fixture_id = FIXTURE_ID_B;

    if(!fixture_id)
        return;

    if(contactIsBelow(contact, fixture_id)) {
        playerComponent->grounded = false;
    }
}

void GameContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold) {}

void GameContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) {}

void GameContactListener::setPlayerFixture(b2Fixture* fixture) {
    playerFixture = fixture;
}

void GameContactListener::setPlayerComponent(PlayerComponent* component) {
    playerComponent = component;
}

bool GameContactListener::contactIsBelow(b2Contact* contact, int fixture_id) {
    b2Vec2 worldManifoldNormal = getWorldManifoldNormal(contact, fixture_id);

    return worldManifoldNormal.y > 0;
}

bool GameContactListener::contactMakesGrounded(b2Contact* contact, int fixture_id) {
    b2Vec2 worldManifoldNormal = getWorldManifoldNormal(contact, fixture_id);

    b2Vec2 up(0, 1);

    float dotVal = up.x * worldManifoldNormal.x + up.y * worldManifoldNormal.y;

    const float maxGroundedAngle = 45;

    return dotVal >= glm::cos(glm::radians<float>(maxGroundedAngle));
}

b2Vec2 GameContactListener::getWorldManifoldNormal(b2Contact* contact, int fixture_id) {
    b2WorldManifold worldManifold;

    contact->GetWorldManifold(&worldManifold);

    bool playerFixtureIsFixA = fixture_id == FIXTURE_ID_A;

    b2Vec2 worldManifoldNormal = worldManifold.normal;

    if(playerFixtureIsFixA) {
        worldManifoldNormal *= -1;
    }

    return worldManifoldNormal;
}
bool GameContactListener::playerHitCategory(b2Contact* contact, int fixture_id, uint32 categoryBits) {
    return (fixture_id == FIXTURE_ID_A && hasHazardBits(contact->GetFixtureB(), categoryBits))
           || (fixture_id == FIXTURE_ID_B && hasHazardBits(contact->GetFixtureA(), categoryBits));
}
bool GameContactListener::hasHazardBits(b2Fixture* fixture, uint32 categoryBits) {
    return (fixture->GetFilterData().categoryBits & categoryBits) == categoryBits;
}
