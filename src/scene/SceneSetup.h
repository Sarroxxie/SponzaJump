#ifndef GRAPHICSPRAKTIKUM_SCENESETUP_H
#define GRAPHICSPRAKTIKUM_SCENESETUP_H

#include "Scene.h"
#include "vulkan/ApplicationContext.h"
#include "RenderableObject.h"

RenderableObject createObject(VulkanBaseContext context,
                              CommandContext commandContext,
                              ObjectDef objectDef,
                              Transformation transformation = { glm::vec3(0), glm::vec3(0), glm::vec3(1)});

void createObject(RenderableObject *object, VulkanBaseContext context, CommandContext commandContext, ObjectDef objectDef, Transformation transformation);

void createSampleVertexBuffer(VulkanBaseContext &context, CommandContext &commandContext, ObjectDef objectDef, RenderableObject &object);

void createSampleIndexBuffer(VulkanBaseContext &baseContext, CommandContext &commandContext, ObjectDef objectDef, RenderableObject &object);

#endif //GRAPHICSPRAKTIKUM_SCENESETUP_H
