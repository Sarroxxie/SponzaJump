#include "RenderableObject.h"

void cleanMeshObject(VulkanBaseContext &baseContext, MeshComponent &object) {
    vkDestroyBuffer(baseContext.device, object.indexBuffer, nullptr);
    vkFreeMemory(baseContext.device, object.indexBufferMemory, nullptr);

    vkDestroyBuffer(baseContext.device, object.vertexBuffer, nullptr);
    vkFreeMemory(baseContext.device, object.vertexBufferMemory, nullptr);
}

ObjectDef getCuboid(glm::vec3 halfSizes, glm::vec3 color) {
    ObjectDef objectDef;

    for (const Vertex &v : COLORED_CUBE_DEF.vertices) {
        objectDef.vertices.push_back({v.pos * halfSizes, color });
    }

    objectDef.indices = COLORED_CUBE_DEF.indices;
    return objectDef;
}

const ObjectDef COLORED_TRIANGLE_DEF = { std::vector<Vertex>({
                                        {{ 0.0f, -0.5f, 0}, {1.0f, 0.0f, 0.0f}},
                                        {{ 0.5f,  0.5f, 0}, {0.0f, 1.0f, 0.0f}},
                                        {{-0.5f,  0.5f, 0}, {0.0f, 0.0f, 1.0f}}
                                    }),
                                    std::vector<uint32_t>(
                                            {0, 1, 2}
                                    )};

const ObjectDef COLORED_SQUARE_DEF = { std::vector<Vertex>({
                                                                     {{-0.5f, -0.5f, 0.0}, {1.0f, 0.0f, 1.0f}},
                                                                     {{ 0.5f, -0.5f, 0.0}, {1.0f, 1.0f, 1.0f}},
                                                                     {{ 0.5f,  0.5f, 0.0}, {0.0f, 1.0f, 1.0f}},
                                                                     {{-0.5f,  0.5f, 0.0}, {0.0f, 0.0f, 1.0f}},
                                                             }),
                                         std::vector<uint32_t>(
                                                 {0, 1, 2, 2, 3, 0}
                                         )};

const ObjectDef COLORED_CUBE_DEF = { std::vector<Vertex>({
                                                                   {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}},
                                                                   {{-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}},
                                                                   {{-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
                                                                   {{-1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 1.0f}},

                                                                   {{ 1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},
                                                                   {{ 1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 1.0f}},
                                                                   {{ 1.0f,  1.0f, -1.0f}, {1.0f, 1.0f, 0.0f}},
                                                                   {{ 1.0f,  1.0f,  1.0f}, {1.0f, 1.0f, 1.0f}},
                                                           }),
                                       std::vector<uint32_t>(
                                               {0, 1, 3, 3, 2, 0, // front
                                                5, 4, 6, 6, 7, 5, // back
                                                4, 5, 1, 1, 0, 4, // bottom
                                                2, 3, 7, 7, 6, 2, // top
                                                4, 0, 2, 2, 6, 4, // left
                                                1, 5, 7, 7, 3, 1 // right
                                                }
                                       )};

const ObjectDef COLORED_PYRAMID = { std::vector<Vertex>({
                                                                   {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}},
                                                                   {{-1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 0.0f}},
                                                                   {{ 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},
                                                                   {{ 1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 1.0f}},

                                                                   {{ 0.0f,  0.0f,  0.0f}, {1.0f, 1.0f, 1.0f}},
                                                           }),
                                       std::vector<uint32_t>(
                                               {0, 1, 3, 3, 2, 0, // bottom
                                                0, 1, 4,
                                                1, 3, 4,
                                                3, 2, 4,
                                                2, 0, 4
                                                }
                                       )};