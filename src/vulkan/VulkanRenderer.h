//
//

#ifndef GRAPHICSPRAKTIKUM_VULKANRENDERER_H
#define GRAPHICSPRAKTIKUM_VULKANRENDERER_H


#include "VulkanContext.h"
#include <vulkan/vulkan_core.h>

class VulkanRenderer {

private:
    VkSemaphore m_ImageAvailableSemaphore;
    VkSemaphore m_RenderFinishedSemaphore;
    VkFence m_InFlightFence;

    VulkanContext &m_Context;

public:
    VulkanRenderer(VulkanContext &context);

    void cleanVulkanRessources();

    void render();

private:
    void recordCommandBuffer(VulkanContext &context, uint32_t imageIndex);

    void createSyncObjects(VulkanContext &context);
};


#endif //GRAPHICSPRAKTIKUM_VULKANRENDERER_H
