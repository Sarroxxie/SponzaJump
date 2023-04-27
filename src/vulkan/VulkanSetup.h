#ifndef GRAPHICSPRAKTIKUM_VULKANSETUP_H
#define GRAPHICSPRAKTIKUM_VULKANSETUP_H

#include <vulkan/vulkan_core.h>
#include <vector>
#include "window.h"
#include "VulkanSettings.h"
#include "VulkanContext.h"

void initializeVulkan(VulkanContext &context, Window &window);

void cleanupVulkan(VulkanContext &context);

void createInstance(VulkanContext &context);

std::vector<const char *> getRequiredExtensions();

bool checkValidationLayerSupport();

void setupDebugMessenger(VulkanContext &context);

void createSurface(VulkanContext &context, Window &window);

void pickPhysicalDevice(VulkanContext &context);

void createLogicalDevice(VulkanContext &context);

void createSwapChain(VulkanContext &context, Window &window);

void recreateSwapChain(VulkanContext &context, Window &window);

void cleanupSwapChain(VulkanContext &context);

void createImageViews(VulkanContext &context);

VkImageView createImageView(VulkanContext &context, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

void createRenderPass(VulkanContext &context);

VkFormat findDepthFormat(VulkanContext &context);

VkFormat findSupportedFormat(VulkanContext &context, const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                             VkFormatFeatureFlags features);

void createDescriptorSetLayout(VulkanContext &context);

void createGraphicsPipeline(VulkanContext &context);

VkShaderModule createShaderModule(VulkanContext &context, const std::vector<char> &code);

void createCommandPool(VulkanContext &context);

void createColorResources(VulkanContext &context);

void createDepthResources(VulkanContext &context);

void createFrameBuffers(VulkanContext &context);
#endif //GRAPHICSPRAKTIKUM_VULKANSETUP_H
