#ifndef GRAPHICSPRAKTIKUM_PLAYERCOMPONENT_H
#define GRAPHICSPRAKTIKUM_PLAYERCOMPONENT_H

struct PlayerComponent
{
    bool grounded;
    bool canDoubleJump;

    bool touchesHazard;
    bool touchesWin;

    PlayerComponent()
        : grounded(false)
        , canDoubleJump(false)
        , touchesHazard(false)
        , touchesWin(false) {}
};

#endif  // GRAPHICSPRAKTIKUM_PLAYERCOMPONENT_H
