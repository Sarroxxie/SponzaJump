#ifndef GRAPHICSPRAKTIKUM_VULKANUTILS_H
#define GRAPHICSPRAKTIKUM_VULKANUTILS_H

#include <vulkan/vulkan_core.h>
#include <iostream>
#include <optional>
#include "window.h"
#include "ApplicationContext.h"
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <array>
#include <scene/Model.h>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    [[nodiscard]] bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};


// TODO This struct is taken from tutorial, we will probably create better way to hold vertices at some point, then this can be deleted
struct Vertex {
    glm::vec3 pos;
    glm::vec3 nrm;
    glm::vec4 tangents;
    glm::vec2 texCoord;
    // uint32_t texIndex;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};

        // TODO don't hardcode binding index here, can pass it in or similar
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = static_cast<uint32_t>(offsetof(Vertex, pos));

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = static_cast<uint32_t>(offsetof(Vertex, nrm));
        
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[2].offset = static_cast<uint32_t>(offsetof(Vertex, tangents));

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[3].offset = static_cast<uint32_t>(offsetof(Vertex, texCoord));

        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const {
        return pos == other.pos && nrm == other.nrm
               && tangents == other.tangents && texCoord == other.texCoord;
    }
};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {


    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        // Message is important enough to show
        std::cerr << "validation layer error: " << pCallbackData->pMessage << std::endl;
    }

    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                      const VkAllocationCallbacks *pAllocator,
                                      VkDebugUtilsMessengerEXT *pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator);

bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

bool checkDeviceExtensionSupport(VkPhysicalDevice device);

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice physicalDevice);

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, Window *window);

void createImage(const VulkanBaseContext& context,
                 uint32_t                 width,
                 uint32_t                 height,
                 uint32_t                 mipLevels,
                 uint32_t                 arrayLayers,
                 VkSampleCountFlagBits    numSamples,
                 VkFormat                 format,
                 VkImageTiling            tiling,
                 VkImageUsageFlags        usage,
                 VkMemoryPropertyFlags    properties,
                 VkImage&                 image,
                 VkDeviceMemory&          imageMemory);

VkImageView createImageView(const VulkanBaseContext &context, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

void createBuffer(const VulkanBaseContext &context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                  VkBuffer &buffer, VkDeviceMemory &bufferMemory);

void createCubeMap(VulkanBaseContext context,
                   CommandContext    commandContext,
                   CubeMap&          cubemap,
                   bool              mipmaps);

void createTextureImage(VulkanBaseContext& context,
                        CommandContext&    commandContext,
                        std::string        path,
                        VkFormat           format,
                        Texture&           texture,
                        bool               mipmaps);

void createTextureSampler(VulkanBaseContext& context, VkSampler& textureSampler, uint32_t mipLevels);

void copyBuffer(const VulkanBaseContext &context, const CommandContext &commandContext, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

void copyBufferToImage(const VulkanBaseContext& context,
                       const CommandContext&    commandContext,
                       VkBuffer                 buffer,
                       VkImage                  image,
                       uint32_t                 width,
                       uint32_t                 height,
                       uint32_t                 offset,
                       uint32_t                 baseArrayLayer);

VkCommandBuffer beginSingleTimeCommands(const VulkanBaseContext  &context, const CommandContext &commandContext);

void endSingleTimeCommands(const VulkanBaseContext &context, const CommandContext &commandContext, VkCommandBuffer commandBuffer);

uint32_t findMemoryType(const VulkanBaseContext &context, uint32_t typeFilter, VkMemoryPropertyFlags properties);

VkFormat findDepthFormat(const VulkanBaseContext &context);

VkFormat findSupportedFormat(const VulkanBaseContext &context, const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                             VkFormatFeatureFlags features);

void generateMipmaps(const VulkanBaseContext& context,
                     const CommandContext&    commandContext,
                     VkImage                  image,
                     VkFormat                 imageFormat,
                     int32_t                  texWidth,
                     int32_t                  texHeight,
                     uint32_t                 mipLevels);

void transitionImageLayout(const VulkanBaseContext& context,
                           const CommandContext&    commandContext,
                           VkImage                  image,
                           VkImageLayout            oldLayout,
                           VkImageLayout            newLayout,
                           VkImageAspectFlags       aspectFlags,
                           uint32_t                 mipLevels);

VkDeviceAddress getBufferDeviceAddress(VkDevice device, VkBuffer buffer);

void transitionImageLayout(const VulkanBaseContext& context,
                           const CommandContext&    commandContext,
                           VkImage                  image,
                           VkFormat                 format,
                           uint32_t                 baseArrayLayer,
                           VkImageLayout            oldLayout,
                           VkImageLayout            newLayout);
#endif //GRAPHICSPRAKTIKUM_VULKANUTILS_H
