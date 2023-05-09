#ifndef GRAPHICSPRAKTIKUM_VULKANSETUP_H
#define GRAPHICSPRAKTIKUM_VULKANSETUP_H

#include <vulkan/vulkan_core.h>
#include <vector>
#include "window.h"
#include "VulkanSettings.h"
#include "ApplicationContext.h"
#include "rendering/RenderContext.h"

void initializeGraphicsApplication(ApplicationVulkanContext &appContext);

void initializeBaseVulkan(ApplicationVulkanContext &appContext);

void initializeSwapChain(ApplicationVulkanContext &appContext);

void initializeCommandContext(ApplicationVulkanContext &appContext);

void cleanupVulkanApplication(ApplicationVulkanContext &appContext);

void cleanupBaseVulkanRessources(VulkanBaseContext &baseContext);

void cleanupSwapChain(VulkanBaseContext &baseContext, SwapchainContext &swapchainContext);

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

void createCommandPool(VulkanBaseContext &baseContext, CommandContext &commandContext);

void createColorResources(VulkanBaseContext &baseContext, SwapchainContext &swapchainContext, GraphicSettings &graphicSettings);

void createDepthResources(VulkanBaseContext &baseContext, SwapchainContext &swapchainContext, GraphicSettings &graphicSettings);

void createCommandBuffers(VulkanBaseContext &baseContext, CommandContext &commandContext);

#endif  // GRAPHICSPRAKTIKUM_VULKANSETUP_H
