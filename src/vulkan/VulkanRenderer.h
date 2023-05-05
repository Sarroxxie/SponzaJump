//
//

#ifndef GRAPHICSPRAKTIKUM_VULKANRENDERER_H
#define GRAPHICSPRAKTIKUM_VULKANRENDERER_H


#include "VulkanContext.h"
#include "window.h"
#include "scene/Scene.h"
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

    void render(Scene &scene);

    void recompileToSecondaryPipeline();
    void swapToSecondaryPipeline();

    ApplicationContext getContext();

private:
    void recordCommandBuffer(ApplicationContext &appContext, RenderContext &renderContext, Scene &scene, uint32_t imageIndex);

    void createSyncObjects(VulkanBaseContext &baseContext);
};


#endif //GRAPHICSPRAKTIKUM_VULKANRENDERER_H
