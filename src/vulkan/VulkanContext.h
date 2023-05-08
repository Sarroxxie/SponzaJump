#ifndef GRAPHICSPRAKTIKUM_VULKANCONTEXT_H
#define GRAPHICSPRAKTIKUM_VULKANCONTEXT_H

#include <vulkan/vulkan_core.h>
#include "VulkanSettings.h"
#include "BufferImage.h"
#include "window.h"

typedef struct {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    uint32_t graphicsQueueFamily;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
} VulkanBaseContext;

typedef struct {
    VkSwapchainKHR swapChain;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    // TODO how to combine Framebuffers with swapChainImageViews ?
    std::vector<VkFramebuffer> swapChainFramebuffers;

    BufferImage colorImage;
    BufferImage depthImage;
} SwapchainContext;

typedef struct {
    VkCommandPool commandPool;

    // TODO might want a vector of commandBuffers here later, so we can record
    // another one while the previous frame is still rendering, that way we can
    // have multiple frames in flight
    VkCommandBuffer commandBuffer;
} CommandContext;

typedef struct {
    bool useMsaa = false;
    uint8_t maxMsaaSamples = 1;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
} GraphicSettings;

typedef struct {
    VulkanBaseContext baseContext;
    SwapchainContext swapchainContext;
    CommandContext commandContext;

    GraphicSettings graphicSettings;

    Window *window;
} ApplicationVulkanContext;



typedef struct
{
    // TODO how to use multiple Renderpasses, multiple pipelines ?
    VkRenderPass renderPass;

    // allows swapping between graphics pipeline at runtime
    VkPipelineLayout pipelineLayouts[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkPipeline graphicsPipelines[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};

    bool activePipelineIndex = 0;
} VulkanRenderContext;

typedef struct {
    // TODO stuff like projection ...
} RenderSettings;

typedef struct {
    VulkanRenderContext vulkanRenderContext;
    RenderSettings renderSettings;
} RenderContext;

#endif  // GRAPHICSPRAKTIKUM_VULKANCONTEXT_H
