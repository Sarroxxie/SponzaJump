#ifndef GRAPHICSPRAKTIKUM_RENDERSETUP_H
#define GRAPHICSPRAKTIKUM_RENDERSETUP_H

#include "vulkan/ApplicationContext.h"
#include "RenderContext.h"
#include "Shader.h"


// --- Sample Render Setups

void initializeSimpleSceneRenderContext(ApplicationVulkanContext &appContext, RenderContext &renderContext);
// ---


void initializeRenderContext(ApplicationVulkanContext &appContext, RenderContext &renderContext, const RenderSetupDescription &renderSetupDescription);

void createDescriptorSetLayout(const VulkanBaseContext &context, RenderPassContext &renderContext, const std::vector<VkDescriptorSetLayoutBinding> &bindings);

void initializeRenderPassContext(const ApplicationVulkanContext &appContext, RenderContext &renderContext, const RenderSetupDescription &renderSetupDescription);

void cleanupRenderContext(const VulkanBaseContext &baseContext, RenderPassContext &renderContext);

void createRenderPass(const ApplicationVulkanContext &appContext, RenderContext &renderContext);

void createGraphicsPipeline(const ApplicationVulkanContext &appContext,
                            RenderPassContext &renderContext,
                            VkPipelineLayout& pipelineLayout,
                            VkPipeline&       graphicsPipeline,
                            const RenderSetupDescription &renderSetupDescription);

void createFrameBuffers(ApplicationVulkanContext &appContext, RenderContext &renderContext);

void buildSecondaryGraphicsPipeline(const ApplicationVulkanContext &appContext, RenderContext &renderContext);

bool swapGraphicsPipeline(const ApplicationVulkanContext &appContext, RenderContext &renderContext);

VkDescriptorSetLayoutBinding createUniformBufferLayoutBinding(uint32_t binding, uint32_t descriptorCount, ShaderStage shaderStage);

VkPushConstantRange createPushConstantRange(uint32_t offset, uint32_t size, ShaderStage shaderStage);

#endif //GRAPHICSPRAKTIKUM_RENDERSETUP_H
