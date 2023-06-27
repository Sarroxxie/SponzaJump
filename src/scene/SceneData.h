#ifndef GRAPHICSPRAKTIKUM_SCENEDATA_H
#define GRAPHICSPRAKTIKUM_SCENEDATA_H

#include <vector>
#include "Model.h"
#include "rendering/host_device.h"

typedef struct {
    std::vector<Mesh>          meshes;
    std::vector<MeshPart>      meshParts;
    std::vector<Texture>       textures;
    std::vector<Material>      materials;
    std::vector<Model>         models;
    std::vector<PointLight>    lights;

    CubeMap cubemap;
} SceneData;

#endif  // GRAPHICSPRAKTIKUM_SCENEDATA_H
