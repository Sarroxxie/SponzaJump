#ifndef GRAPHICSPRAKTIKUM_RENDERABLEOBJECT_H
#define GRAPHICSPRAKTIKUM_RENDERABLEOBJECT_H

#include <vulkan/vulkan_core.h>

#include "vulkan/VulkanContext.h"

typedef struct
{
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    uint32_t verticesCount;
    uint32_t indicesCount;
} RenderableObject;

void cleanRenderableObject(VulkanBaseContext &baseContext, RenderableObject &object);

#endif //GRAPHICSPRAKTIKUM_RENDERABLEOBJECT_H
