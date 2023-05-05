#ifndef GRAPHICSPRAKTIKUM_SCENESETUP_H
#define GRAPHICSPRAKTIKUM_SCENESETUP_H

#include "Scene.h"
#include "vulkan/VulkanContext.h"
#include "RenderableObject.h"

RenderableObject createSampleObject(VulkanBaseContext context, CommandContext commandContext, float xOffset = 0);

void createSampleVertexBuffer(VulkanBaseContext &context, CommandContext &commandContext, RenderableObject &object, float xOffset);

void createSampleIndexBuffer(VulkanBaseContext &baseContext, CommandContext &commandContext, RenderableObject &object);

#endif //GRAPHICSPRAKTIKUM_SCENESETUP_H
