#ifndef GRAPHICSPRAKTIKUM_RENDERABLEOBJECT_H
#define GRAPHICSPRAKTIKUM_RENDERABLEOBJECT_H

#include <vulkan/vulkan_core.h>

#include "vulkan/VulkanContext.h"
#include "vulkan/VulkanUtils.h"

typedef struct
{
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    uint32_t verticesCount;
    uint32_t indicesCount;

    glm::vec3 offset;
} RenderableObject;

void cleanRenderableObject(VulkanBaseContext &baseContext, RenderableObject &object);

typedef struct objectDef_s {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
} ObjectDef;

extern const ObjectDef COLORED_TRIANGLE_DEF;
extern const ObjectDef COLORED_SQUARE_DEF;

#endif //GRAPHICSPRAKTIKUM_RENDERABLEOBJECT_H
