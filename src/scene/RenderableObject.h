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
    glm::vec3 translation {glm::vec3(0)};
    glm::vec3 rotation {glm::vec3(0)};
    glm::vec3 scaling {glm::vec3(1)};

    [[nodiscard]] glm::mat4 getMatrix() const {
        glm::mat4 scaleMat = glm::scale(glm::mat4(1), scaling);
        glm::mat4 rotateMat = glm::eulerAngleXYZ(rotation.x, rotation.y, rotation.z);
        glm::mat4 translateMat = glm::translate(glm::mat4(1), translation);

        return translateMat * rotateMat * scaleMat;
    }
} Transformation;

typedef struct meshComponent_s {
    VkBuffer vertexBuffer {VK_NULL_HANDLE};
    VkDeviceMemory vertexBufferMemory {VK_NULL_HANDLE};

    VkBuffer indexBuffer {VK_NULL_HANDLE};
    VkDeviceMemory indexBufferMemory {VK_NULL_HANDLE};

    uint32_t verticesCount = 0;
    uint32_t indicesCount = 0;
} MeshComponent;

void cleanMeshObject(VulkanBaseContext &baseContext, MeshComponent &object);

typedef struct objectDef_s {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
} ObjectDef;

ObjectDef getCuboid(glm::vec3 halfSizes, glm::vec3 color = glm::vec3(0.7));

extern const ObjectDef COLORED_TRIANGLE_DEF;
extern const ObjectDef COLORED_SQUARE_DEF;
extern const ObjectDef COLORED_CUBE_DEF;
extern const ObjectDef COLORED_PYRAMID;

#endif //GRAPHICSPRAKTIKUM_RENDERABLEOBJECT_H
