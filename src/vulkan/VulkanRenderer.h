//
//

#ifndef GRAPHICSPRAKTIKUM_VULKANRENDERER_H
#define GRAPHICSPRAKTIKUM_VULKANRENDERER_H


#include "ApplicationContext.h"
#include "window.h"
#include "scene/Scene.h"
#include "rendering/RenderContext.h"
#include <vulkan/vulkan_core.h>

class VulkanRenderer {

private:
    VkSemaphore m_ImageAvailableSemaphore;
    VkSemaphore m_RenderFinishedSemaphore;
    VkFence m_InFlightFence;

    ApplicationVulkanContext &m_Context;
    RenderContext &m_RenderContext;

    int frameNumber = 0;

public:
    VulkanRenderer(ApplicationVulkanContext &context, RenderContext &renderContext);

    void cleanVulkanRessources();

    void render(Scene &scene);

    void setRenderContext(RenderContext &renderContext);

    void recompileToSecondaryPipeline();
    void swapToSecondaryPipeline();

    ApplicationVulkanContext getContext();

private:
    void recordCommandBuffer(Scene &scene, uint32_t imageIndex);

    void recordShadowPass(Scene &scene, uint32_t imageIndex);

    void recordMainRenderPass(Scene &scene, uint32_t imageIndex);

    void createSyncObjects(VulkanBaseContext &baseContext);

    void updateUniformBuffer(Scene &scene);
};


#endif //GRAPHICSPRAKTIKUM_VULKANRENDERER_H
