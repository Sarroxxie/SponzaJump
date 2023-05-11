#ifndef GRAPHICSPRAKTIKUM_VULKANSETUP_H
#define GRAPHICSPRAKTIKUM_VULKANSETUP_H

#include <vulkan/vulkan_core.h>
#include <vector>
#include "window.h"
#include "VulkanSettings.h"
#include "VulkanContext.h"

void initializeGraphicsApplication(ApplicationContext &appContext);

void initializeBaseVulkan(ApplicationContext &appContext);

void initializeSwapChain(ApplicationContext &appContext);

void initializeRenderContext(ApplicationContext &appContext);

void cleanupVulkan(ApplicationContext &appContext);

void cleanupBaseVulkanRessources(VulkanBaseContext &baseContext);

void cleanupSwapChain(VulkanBaseContext &baseContext, SwapchainContext &swapchainContext);

void cleanupRenderContext(VulkanBaseContext &baseContext, RenderContext &renderContext);

void cleanupImGuiContext(VulkanBaseContext& baseContext, RenderContext& renderContext);

void createInstance(VulkanBaseContext &context);

std::vector<const char *> getRequiredExtensions();

bool checkValidationLayerSupport();

void setupDebugMessenger(VulkanBaseContext &context);

void createSurface(VulkanBaseContext &context, Window *window);

void pickPhysicalDevice(VulkanBaseContext &context, RenderSettings &renderSettings);

void createLogicalDevice(VulkanBaseContext &context);

void createSwapChain(VulkanBaseContext &context, SwapchainContext &swapchainContext, Window *window);

void recreateSwapChain(ApplicationContext &appContext);

void createImageViews(VulkanBaseContext &context, SwapchainContext &swapchainContext);

VkImageView createImageView(VulkanBaseContext &context, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

void createRenderPass(ApplicationContext &appContext);

VkFormat findDepthFormat(VulkanBaseContext &context);

VkFormat findSupportedFormat(VulkanBaseContext &context, const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                             VkFormatFeatureFlags features);

void createDescriptorSetLayout(ApplicationContext &appContext);

void createGraphicsPipeline(ApplicationContext &appContext,
                            RenderContext &renderContext,
                            VkPipelineLayout& pipelineLayout,
                            VkPipeline&       graphicsPipeline,
                            std::string vertexShaderPath = "res/shaders/spv/triangle.vert.spv",
                            std::string fragmentShaderPath = "res/shaders/spv/triangle.frag.spv");

VkShaderModule createShaderModule(VulkanBaseContext &context, const std::vector<char>& code);

void createCommandPool(VulkanBaseContext &baseContext, RenderContext &renderContext);

void createColorResources(VulkanBaseContext &baseContext, SwapchainContext &swapchainContext, RenderSettings &renderSettings);

void createDepthResources(VulkanBaseContext &baseContext, SwapchainContext &swapchainContext, RenderSettings &renderSettings);

void createFrameBuffers(ApplicationContext &appContext);

void createVertexBuffer(ApplicationContext &appContext);

void createIndexBuffer(ApplicationContext &appContext);

void createCommandBuffers(VulkanBaseContext &baseContext, RenderContext &renderContext);

void buildSecondaryGraphicsPipeline(ApplicationContext &appContext);

bool swapGraphicsPipeline(ApplicationContext &appContext);

void compileShader(std::string path, std::string shaderDirectoryPath = "res/shaders/source/");

void initializeImGui(ApplicationContext& appContext);
#endif  // GRAPHICSPRAKTIKUM_VULKANSETUP_H
