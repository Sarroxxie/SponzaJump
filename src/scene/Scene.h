#ifndef GRAPHICSPRAKTIKUM_SCENE_H
#define GRAPHICSPRAKTIKUM_SCENE_H

#include <vulkan/vulkan_core.h>
#include <vector>
#include "vulkan/VulkanContext.h"
#include "SceneSetup.h"
#include "RenderableObject.h"


class Scene {
private:
    std::vector<RenderableObject> objects;

public:
    Scene();

    void addObject(RenderableObject object);

    std::vector<RenderableObject> getObjects();
    bool hasObject();


    void cleanup(VulkanBaseContext &baseContext);
};

#endif //GRAPHICSPRAKTIKUM_SCENE_H
