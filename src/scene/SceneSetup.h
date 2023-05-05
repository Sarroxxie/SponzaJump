#ifndef GRAPHICSPRAKTIKUM_SCENESETUP_H
#define GRAPHICSPRAKTIKUM_SCENESETUP_H

#include "Scene.h"
#include "vulkan/VulkanContext.h"
#include "RenderableObject.h"

RenderableObject createObject(VulkanBaseContext context, CommandContext commandContext, ObjectDef objectDef, glm::vec3 offset = glm::vec3(0));

void createSampleVertexBuffer(VulkanBaseContext &context, CommandContext &commandContext, ObjectDef objectDef, RenderableObject &object, glm::vec3 offset);

void createSampleIndexBuffer(VulkanBaseContext &baseContext, CommandContext &commandContext, ObjectDef objectDef, RenderableObject &object);

#endif //GRAPHICSPRAKTIKUM_SCENESETUP_H
