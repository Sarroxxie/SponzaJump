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

void addToScene(Scene &scene, ModelLoader &loader, GameContactListener &contactListener);

EntityId addPhysicsEntity(Scene &scene,
                          const VulkanBaseContext &context,
                          const CommandContext &commandContext,
                          int modelID,
                          Transformation transformation,
                          glm::vec3 halfSize,
                          bool dynamic = false,
                          bool fixedRotation = true);

EntityId addPlayerEntity(Scene &scene,
                          const VulkanBaseContext &context,
                          const CommandContext &commandContext,
                          int modelID,
                          Transformation transformation,
                          glm::vec3 halfSize,
                          GameContactListener &contactListener,
                          bool dynamic = false,
                          bool fixedRotation = true);
#endif //GRAPHICSPRAKTIKUM_SCENESETUP_H
