//
//

#ifndef GRAPHICSPRAKTIKUM_VULKANRENDERER_H
#define GRAPHICSPRAKTIKUM_VULKANRENDERER_H


#include "VulkanContext.h"
#include "window.h"
#include <vulkan/vulkan_core.h>

class VulkanRenderer {

private:
    VkSemaphore m_ImageAvailableSemaphore;
    VkSemaphore m_RenderFinishedSemaphore;
    VkFence m_InFlightFence;

    VulkanContext &m_Context;

    Window &m_Window;

public:
    VulkanRenderer(VulkanContext &context, Window &window);

    void cleanVulkanRessources();

    void render();

    void recompileToSecondaryPipeline();
    void swapToSecondaryPipeline();

private:
    void recordCommandBuffer(VulkanContext &context, uint32_t imageIndex);

    void createSyncObjects(VulkanContext &context);
};


#endif //GRAPHICSPRAKTIKUM_VULKANRENDERER_H
