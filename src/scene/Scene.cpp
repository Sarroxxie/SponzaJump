//
//

#include "Scene.h"

Scene::Scene() { }

void Scene::addObject(RenderableObject object) {
    objects.push_back(object);
}

void Scene::cleanup(VulkanBaseContext &baseContext) {
    for (auto &object: objects) {
        cleanRenderableObject(baseContext, object);
    }
}

std::vector<RenderableObject> Scene::getObjects() {
    return objects;
}

bool Scene::hasObject() {
    return objects.size() > 0;
}
