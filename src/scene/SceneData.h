#ifndef GRAPHICSPRAKTIKUM_SCENEDATA_H
#define GRAPHICSPRAKTIKUM_SCENEDATA_H

#include <vector>
#include "Model.h"

typedef struct {
    std::vector<Mesh>          meshes;
    std::vector<MeshPart>      meshParts;
    std::vector<Texture>       textures;
    std::vector<Material>      materials;

    /*
    std::vector<Model>         models;
    std::vector<ModelInstance> instances;
     */
} SceneData;

#endif  // GRAPHICSPRAKTIKUM_SCENEDATA_H
