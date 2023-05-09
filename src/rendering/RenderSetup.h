#ifndef GRAPHICSPRAKTIKUM_RENDERSETUP_H
#define GRAPHICSPRAKTIKUM_RENDERSETUP_H

#include "vulkan/ApplicationContext.h"
#include "RenderContext.h"
#include "Shader.h"

void initializeRenderContext(ApplicationVulkanContext &appContext, RenderContext &renderContext);

void createDescriptorSetLayout(const VulkanBaseContext &context, VulkanRenderContext &renderContext);

void initializeVulkanRenderContext(const ApplicationVulkanContext &appContext, RenderContext &renderContext);

void cleanupRenderContext(const VulkanBaseContext &baseContext, VulkanRenderContext &renderContext);

void createRenderPass(const ApplicationVulkanContext &appContext, RenderContext &renderContext);

void createGraphicsPipeline(const ApplicationVulkanContext &appContext,
                            VulkanRenderContext &renderContext,
                            VkPipelineLayout& pipelineLayout,
                            VkPipeline&       graphicsPipeline,
                            const Shader &vertexShader,
                            const Shader &fragmentShader);

void createFrameBuffers(ApplicationVulkanContext &appContext, RenderContext &renderContext);

void buildSecondaryGraphicsPipeline(const ApplicationVulkanContext &appContext, RenderContext &renderContext);

bool swapGraphicsPipeline(const ApplicationVulkanContext &appContext, RenderContext &renderContext);


#endif //GRAPHICSPRAKTIKUM_RENDERSETUP_H
