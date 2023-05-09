#ifndef GRAPHICSPRAKTIKUM_RENDERSETUP_H
#define GRAPHICSPRAKTIKUM_RENDERSETUP_H

#include "vulkan/ApplicationContext.h"
#include "RenderContext.h"

void initializeRenderContext(ApplicationVulkanContext &appContext, RenderContext &renderContext);

void createDescriptorSetLayout(const VulkanBaseContext &context, VulkanRenderContext &renderContext);

void initializeVulkanRenderContext(const ApplicationVulkanContext &appContext, RenderContext &renderContext);

void cleanupRenderContext(const VulkanBaseContext &baseContext, VulkanRenderContext &renderContext);

void createRenderPass(const ApplicationVulkanContext &appContext, RenderContext &renderContext);

void createGraphicsPipeline(const ApplicationVulkanContext &appContext,
                            VulkanRenderContext &renderContext,
                            VkPipelineLayout& pipelineLayout,
                            VkPipeline&       graphicsPipeline,
                            std::string vertexShaderPath = "res/shaders/spv/triangle.vert.spv", // TODO make sure there are no default arguments here once proper shaders are introduced
                            std::string fragmentShaderPath = "res/shaders/spv/triangle.frag.spv");

void createFrameBuffers(ApplicationVulkanContext &appContext, RenderContext &renderContext);


// TODO move these following shader methods to better location
VkShaderModule createShaderModule(const VulkanBaseContext &context, const std::vector<char>& code);

void compileShader(const std::string &path, const std::string &shaderDirectoryPath = "res/shaders/source/");

void buildSecondaryGraphicsPipeline(const ApplicationVulkanContext &appContext, RenderContext &renderContext);

bool swapGraphicsPipeline(const ApplicationVulkanContext &appContext, RenderContext &renderContext);


#endif //GRAPHICSPRAKTIKUM_RENDERSETUP_H
