#include "RenderableObject.h"

void cleanRenderableObject(VulkanBaseContext &baseContext, RenderableObject &object) {
    vkDestroyBuffer(baseContext.device, object.indexBuffer, nullptr);
    vkFreeMemory(baseContext.device, object.indexBufferMemory, nullptr);

    vkDestroyBuffer(baseContext.device, object.vertexBuffer, nullptr);
    vkFreeMemory(baseContext.device, object.vertexBufferMemory, nullptr);
}
