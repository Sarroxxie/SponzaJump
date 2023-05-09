#ifndef GRAPHICSPRAKTIKUM_RENDERABLEOBJECT_H
#define GRAPHICSPRAKTIKUM_RENDERABLEOBJECT_H

#include <vulkan/vulkan_core.h>

#include "vulkan/ApplicationContext.h"
#include "vulkan/VulkanUtils.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include <glm/gtx/euler_angles.hpp>

typedef struct transformation_s
{
    glm::vec3 translation;
    glm::vec3 rotation;
    glm::vec3 scaling;

    [[nodiscard]] glm::mat4 getMatrix() const {
        glm::mat4 scaleMat = glm::scale(glm::mat4(1), scaling);
        glm::mat4 rotateMat = glm::eulerAngleXYZ(rotation.x, rotation.y, rotation.z);
        glm::mat4 translateMat = glm::translate(glm::mat4(1), translation);

        return translateMat * rotateMat * scaleMat;
    }
} Transformation;

typedef struct
{
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    uint32_t verticesCount;
    uint32_t indicesCount;

    Transformation transformation;
} RenderableObject;

void cleanRenderableObject(VulkanBaseContext &baseContext, RenderableObject &object);

typedef struct objectDef_s {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
} ObjectDef;

extern const ObjectDef COLORED_TRIANGLE_DEF;
extern const ObjectDef COLORED_SQUARE_DEF;
extern const ObjectDef COLORED_CUBE_DEF;
extern const ObjectDef COLORED_PYRAMID;

#endif //GRAPHICSPRAKTIKUM_RENDERABLEOBJECT_H
