//
//

#include <iostream>
#include "GameContactListener.h"
#include "box2d/b2_contact.h"

void GameContactListener::BeginContact(b2Contact *contact) {
    // test if any fixture is player fixture -> early exit
    int fixture_id = 0;
    if (contact->GetFixtureA() == playerFixture) fixture_id = 1;
    if (contact->GetFixtureB() == playerFixture) fixture_id = 2;

    if (!fixture_id) return;

    if (contactIsBelow(contact, fixture_id)) {
        playerComponent->grounded = true;
    }
}

void GameContactListener::EndContact(b2Contact *contact) {
    int fixture_id = 0;
    if (contact->GetFixtureA() == playerFixture) fixture_id = 1;
    if (contact->GetFixtureB() == playerFixture) fixture_id = 2;

    if (!fixture_id) return;

    if (contactIsBelow(contact, fixture_id)) {
        playerComponent->grounded = false;
    }
}

void GameContactListener::PreSolve(b2Contact *contact, const b2Manifold *oldManifold) {

}

void GameContactListener::PostSolve(b2Contact *contact, const b2ContactImpulse *impulse) {

}

void GameContactListener::setPlayerFixture(b2Fixture *fixture) {
    playerFixture = fixture;
}

bool GameContactListener::contactIsBelow(b2Contact *contact, int fixture_id) {
    b2Manifold *manifold = contact->GetManifold();

    // test if playerfixture is manifold fixture
    bool manifoldFixIsPlayerFix = (manifold->type == b2Manifold::e_faceA && fixture_id == 1)
                                  || (manifold->type == b2Manifold::e_faceB && fixture_id == 2);


    // if yes -> test_direction = (0, -1) else (0, 1)
    b2Vec2 test_direction = b2Vec2(0, manifoldFixIsPlayerFix ? -1 : 1);

    // dot product of manifold normal and test_direction -> if larger than 1: yes else no
    float dotVal = test_direction.x * manifold->localNormal.x + test_direction.y * manifold->localNormal.y;

    return dotVal > 0;
}

void GameContactListener::setPlayerComponent(PlayerComponent *component) {
    playerComponent = component;
}
