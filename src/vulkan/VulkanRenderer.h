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

    ApplicationContext &m_Context;

public:
    VulkanRenderer(ApplicationContext &context);

    void cleanVulkanRessources();

    void render();

    void recompileToSecondaryPipeline();
    void swapToSecondaryPipeline();

    ApplicationContext getContext();

private:
    void recordCommandBuffer(SwapchainContext &swapchainContext, RenderContext &renderContext, uint32_t imageIndex);

    void createSyncObjects(VulkanBaseContext &baseContext);
};


#endif //GRAPHICSPRAKTIKUM_VULKANRENDERER_H
