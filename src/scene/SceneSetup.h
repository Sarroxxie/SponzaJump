#ifndef GRAPHICSPRAKTIKUM_SCENESETUP_H
#define GRAPHICSPRAKTIKUM_SCENESETUP_H

#include "Scene.h"
#include "vulkan/ApplicationContext.h"
#include "RenderableObject.h"
#include "Entity.h"

EntityId addPhysicsEntity(Scene &scene,
                          VulkanBaseContext context,
                          CommandContext commandContext,
                          ObjectDef objectDef,
                          Transformation transformation,
                          glm::vec3 halfSize,
                          bool dynamic = false,
                          bool fixedRotation = true);

MeshComponent createMeshComponent(MeshComponent *component, VulkanBaseContext context, CommandContext commandContext, ObjectDef objectDef);

void createSampleVertexBuffer(VulkanBaseContext &context, CommandContext &commandContext, ObjectDef objectDef, MeshComponent *object);

void createSampleIndexBuffer(VulkanBaseContext &baseContext, CommandContext &commandContext, ObjectDef objectDef, MeshComponent *object);

#endif //GRAPHICSPRAKTIKUM_SCENESETUP_H
