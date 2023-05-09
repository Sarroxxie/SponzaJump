#ifndef GRAPHICSPRAKTIKUM_VULKANSETUP_H
#define GRAPHICSPRAKTIKUM_VULKANSETUP_H

#include <vulkan/vulkan_core.h>
#include <vector>
#include "window.h"
#include "VulkanSettings.h"
#include "ApplicationContext.h"
#include "RenderContext.h"

void initializeGraphicsApplication(ApplicationVulkanContext &appContext);

void initializeRenderContext(ApplicationVulkanContext &appContext, RenderContext &renderContext);

void initializeBaseVulkan(ApplicationVulkanContext &appContext);

void initializeSwapChain(ApplicationVulkanContext &appContext);

void initializeVulkanRenderContext(ApplicationVulkanContext &appContext, RenderContext &renderContext);

void initializeCommandContext(ApplicationVulkanContext &appContext);

void cleanupVulkanApplication(ApplicationVulkanContext &appContext);

void cleanupBaseVulkanRessources(VulkanBaseContext &baseContext);

void cleanupSwapChain(VulkanBaseContext &baseContext, SwapchainContext &swapchainContext);

void cleanupRenderContext(VulkanBaseContext &baseContext, VulkanRenderContext &renderContext);

void cleanupCommandContext(VulkanBaseContext &baseContext, CommandContext &commandContext);

void createInstance(VulkanBaseContext &context);

std::vector<const char *> getRequiredExtensions();

bool checkValidationLayerSupport();

void setupDebugMessenger(VulkanBaseContext &context);

void createSurface(VulkanBaseContext &context, Window *window);

void pickPhysicalDevice(VulkanBaseContext &context, GraphicSettings &graphicSettings);

void createLogicalDevice(VulkanBaseContext &context);

void createSwapChain(VulkanBaseContext &context, SwapchainContext &swapchainContext, Window *window);

void recreateSwapChain(ApplicationVulkanContext &appContext, RenderContext &renderContext);

void createImageViews(VulkanBaseContext &context, SwapchainContext &swapchainContext);

VkImageView createImageView(VulkanBaseContext &context, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

void createRenderPass(ApplicationVulkanContext &appContext, RenderContext &renderContext);

VkFormat findDepthFormat(VulkanBaseContext &context);

VkFormat findSupportedFormat(VulkanBaseContext &context, const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                             VkFormatFeatureFlags features);

void createDescriptorSetLayout(VulkanBaseContext &context, VulkanRenderContext &renderContext);

void createGraphicsPipeline(ApplicationVulkanContext &appContext,
                            VulkanRenderContext &renderContext,
                            VkPipelineLayout& pipelineLayout,
                            VkPipeline&       graphicsPipeline,
                            std::string vertexShaderPath = "res/shaders/spv/triangle.vert.spv",
                            std::string fragmentShaderPath = "res/shaders/spv/triangle.frag.spv");

VkShaderModule createShaderModule(VulkanBaseContext &context, const std::vector<char>& code);

void createCommandPool(VulkanBaseContext &baseContext, CommandContext &commandContext);

void createColorResources(VulkanBaseContext &baseContext, SwapchainContext &swapchainContext, GraphicSettings &graphicSettings);

void createDepthResources(VulkanBaseContext &baseContext, SwapchainContext &swapchainContext, GraphicSettings &graphicSettings);

void createFrameBuffers(ApplicationVulkanContext &appContext, RenderContext &renderContext);

void createCommandBuffers(VulkanBaseContext &baseContext, CommandContext &commandContext);

void buildSecondaryGraphicsPipeline(ApplicationVulkanContext &appContext, RenderContext &renderContext);

bool swapGraphicsPipeline(ApplicationVulkanContext &appContext, RenderContext &renderContext);

void compileShader(std::string path, std::string shaderDirectoryPath = "res/shaders/source/");

#endif  // GRAPHICSPRAKTIKUM_VULKANSETUP_H
