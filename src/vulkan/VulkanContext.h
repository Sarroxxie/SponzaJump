#ifndef GRAPHICSPRAKTIKUM_VULKANCONTEXT_H
#define GRAPHICSPRAKTIKUM_VULKANCONTEXT_H

#include <vulkan/vulkan_core.h>
#include "VulkanSettings.h"

typedef struct
{
    VkInstance               instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR             surface;
    VkPhysicalDevice         physicalDevice;
    VkDevice                 device;

    uint8_t        maxSupportedMsaaSamples;
    VulkanSettings vulkanSettings;

    uint32_t graphicsQueueFamily;
    VkQueue  graphicsQueue;
    VkQueue  presentQueue;

    VkSwapchainKHR swapChain;
    VkFormat       swapChainImageFormat;
    VkExtent2D     swapChainExtent;

    std::vector<VkImage>     swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    VkRenderPass renderPass;

    // allows swapping between graphics pipeline at runtime
    VkPipelineLayout pipelineLayouts[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkPipeline       graphicsPipelines[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};
    bool             activePipelineIndex = 0;

    VkCommandPool commandPool;

    // TODO these should be extracted to "BufferImage" class or similar
    VkImage        colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView    colorImageView;

    VkImage        depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView    depthImageView;

    std::vector<VkFramebuffer> swapChainFramebuffers;

    // TODO these should be extracted into classes, used in single objects
    VkBuffer       vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkBuffer       indexBuffer;
    VkDeviceMemory indexBufferMemory;

    // TODO might want a vector of commandBuffers here later, so we can record
    // another one while the previous frame is still rendering, that way we can
    // have multiple frames in flight
    VkCommandBuffer commandBuffer;
} VulkanContext;

#endif  // GRAPHICSPRAKTIKUM_VULKANCONTEXT_H
