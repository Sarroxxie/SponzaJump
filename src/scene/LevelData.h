#ifndef GRAPHICSPRAKTIKUM_LEVELDATA_H
#define GRAPHICSPRAKTIKUM_LEVELDATA_H

#include "glm/vec3.hpp"

#define HAZARD_CATEGORY_BITS 0x02
#define WIN_AREA_CATEGORY_BITS 0x04

typedef struct s_levelData {
    glm::vec3 playerSpawnLocation;

    bool disableDeath = false;
    float deathPlaneHeight = -15;

    bool hasWon = false;
} LevelData;


#endif  // GRAPHICSPRAKTIKUM_LEVELDATA_H
