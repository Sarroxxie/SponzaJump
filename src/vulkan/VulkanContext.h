#ifndef GRAPHICSPRAKTIKUM_VULKANCONTEXT_H
#define GRAPHICSPRAKTIKUM_VULKANCONTEXT_H

#include <vulkan/vulkan_core.h>
#include "VulkanSettings.h"

typedef struct {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    uint8_t maxSupportedMsaaSamples;
    VulkanSettings vulkanSettings;

    uint32_t graphicsQueueFamily;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSwapchainKHR swapChain;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    VkRenderPass renderPass;

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkCommandPool commandPool;

    // TODO these should be extracted to "BufferImage" class or similar
    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    std::vector<VkFramebuffer> swapChainFramebuffers;
} VulkanContext;

#endif //GRAPHICSPRAKTIKUM_VULKANCONTEXT_H
