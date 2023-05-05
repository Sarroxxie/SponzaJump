#ifndef GRAPHICSPRAKTIKUM_SCENESETUP_H
#define GRAPHICSPRAKTIKUM_SCENESETUP_H

#include "Scene.h"
#include "vulkan/VulkanContext.h"
#include "RenderableObject.h"

RenderableObject createSampleObject(VulkanBaseContext context, CommandContext commandContext);

void createSampleVertexBuffer(VulkanBaseContext &context, CommandContext &commandContext, RenderableObject &object);

void createSampleIndexBuffer(VulkanBaseContext &baseContext, CommandContext &commandContext, RenderableObject &object);

#endif //GRAPHICSPRAKTIKUM_SCENESETUP_H
