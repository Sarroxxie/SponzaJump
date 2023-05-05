#include "RenderableObject.h"

void cleanRenderableObject(VulkanBaseContext &baseContext, RenderableObject &object) {
    vkDestroyBuffer(baseContext.device, object.indexBuffer, nullptr);
    vkFreeMemory(baseContext.device, object.indexBufferMemory, nullptr);

    vkDestroyBuffer(baseContext.device, object.vertexBuffer, nullptr);
    vkFreeMemory(baseContext.device, object.vertexBufferMemory, nullptr);
}

const ObjectDef COLORED_TRIANGLE_DEF = { std::vector<Vertex>({
                                        {{ 0.0f, -0.5f, 0.0}, {1.0f, 0.0f, 0.0f}},
                                        {{ 0.5f,  0.5f, 0.0}, {0.0f, 1.0f, 0.0f}},
                                        {{-0.5f,  0.5f, 0.0}, {0.0f, 0.0f, 1.0f}}
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

