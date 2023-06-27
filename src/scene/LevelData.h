#ifndef GRAPHICSPRAKTIKUM_LEVELDATA_H
#define GRAPHICSPRAKTIKUM_LEVELDATA_H

#include "glm/vec3.hpp"

typedef struct s_levelData {
    glm::vec3 playerSpawnLocation;

    bool disableDeath = false;
    float deathPlaneHeight = 0;
} LevelData;


#endif  // GRAPHICSPRAKTIKUM_LEVELDATA_H
