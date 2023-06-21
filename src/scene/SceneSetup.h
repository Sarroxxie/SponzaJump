#ifndef GRAPHICSPRAKTIKUM_SCENESETUP_H
#define GRAPHICSPRAKTIKUM_SCENESETUP_H

#include "Scene.h"
#include "vulkan/ApplicationContext.h"
#include "RenderableObject.h"
#include "Entity.h"
#include "physics/GameContactListener.h"

void createSamplePhysicsScene(const ApplicationVulkanContext& context,
                              Scene&                          scene,
                              GameContactListener&            contactListener);

void addToScene(Scene&                          scene,
                ModelLoader&                    loader,
                GameContactListener&            contactListener);

b2Fixture* addPhysicsComponent(Scene& scene, EntityId entityId, ModelInstance& instance, bool isDynamic);

#endif  // GRAPHICSPRAKTIKUM_SCENESETUP_H
