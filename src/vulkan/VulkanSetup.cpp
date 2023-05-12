#include "VulkanSetup.h"
#include "VulkanSettings.h"
#include "GLFW/glfw3.h"
#include "utils/FileUtils.h"
#include "VulkanUtils.h"
#include "rendering/RenderSetup.h"

#include <vulkan/vulkan_core.h>
#include <stdexcept>
#include <cstring>
#include <set>
#include <filesystem>

// setup in large parts taken from https://vulkan-tutorial.com/Introduction
void initializeGraphicsApplication(ApplicationVulkanContext &appContext) {
    initializeBaseVulkan(appContext);
    initializeSwapChain(appContext);
    initializeCommandContext(appContext);

}

void initializeBaseVulkan(ApplicationVulkanContext &appContext) {
    createInstance(appContext.baseContext);
    setupDebugMessenger(appContext.baseContext);
    createSurface(appContext.baseContext, appContext.window);
    pickPhysicalDevice(appContext.baseContext, appContext.graphicSettings);
    createLogicalDevice(appContext.baseContext);
}

void initializeSwapChain(ApplicationVulkanContext &appContext) {
    createSwapChain(appContext.baseContext, appContext.swapchainContext, appContext.window);
    createImageViews(appContext.baseContext, appContext.swapchainContext);

    appContext.swapchainContext.swapChainFramebuffers.resize(appContext.swapchainContext.swapChainImageViews.size());

    // TODO don't need to create ColorRessources if multisampling is turned off
    createColorResources(appContext.baseContext, appContext.swapchainContext, appContext.graphicSettings);
    createDepthResources(appContext.baseContext, appContext.swapchainContext, appContext.graphicSettings);
}

void initializeCommandContext(ApplicationVulkanContext &appContext) {
    createCommandPool(appContext.baseContext, appContext.commandContext);
    createCommandBuffers(appContext.baseContext, appContext.commandContext);
}

void createInstance(VulkanBaseContext &context) {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};

    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Candles";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;


    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();


    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateInstance(&createInfo, nullptr, &context.instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

void cleanupVulkanApplication(ApplicationVulkanContext &appContext) {
    cleanupSwapChain(appContext.baseContext, appContext.swapchainContext);

    cleanupCommandContext(appContext.baseContext, appContext.commandContext);

    cleanupBaseVulkanRessources(appContext.baseContext);
}

void cleanupBaseVulkanRessources(VulkanBaseContext &baseContext) {
    vkDestroyDevice(baseContext.device, nullptr);

    vkDestroySurfaceKHR(baseContext.instance, baseContext.surface, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(baseContext.instance, baseContext.debugMessenger, nullptr);
    }

    vkDestroyInstance(baseContext.instance, nullptr);
}

void cleanupCommandContext(VulkanBaseContext &baseContext, CommandContext &commandContext) {
    vkDestroyCommandPool(baseContext.device, commandContext.commandPool, nullptr);

}

std::vector<const char *> getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool checkValidationLayerSupport() {
    //std::cout << "Checking Validation Layer Support" << std::endl;

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName: validationLayers) {
        bool layerFound = false;

        for (const auto &layerProperties: availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

void setupDebugMessenger(VulkanBaseContext &context) {
    if (!enableValidationLayers) return;
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;

    if (CreateDebugUtilsMessengerEXT(context.instance, &createInfo, nullptr, &context.debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void createSurface(VulkanBaseContext &context, Window *window) {
    if (glfwCreateWindowSurface(context.instance, window->getWindowHandle(), nullptr, &context.surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void pickPhysicalDevice(VulkanBaseContext &context, GraphicSettings &graphicSettings) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(context.instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(context.instance, &deviceCount, devices.data());

    context.physicalDevice = VK_NULL_HANDLE;

    for (const auto &device: devices) {
        if (isDeviceSuitable(device, context.surface)) {
            context.physicalDevice = device;
            graphicSettings.maxMsaaSamples = getMaxUsableSampleCount(device);
            break;
        }
    }
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(context.physicalDevice, &deviceProperties);


    context.maxSupportedMinorVersion = VK_API_VERSION_MINOR(deviceProperties.apiVersion);

    if (context.physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void createLogicalDevice(VulkanBaseContext &context) {
    QueueFamilyIndices indices = findQueueFamilies(context.physicalDevice, context.surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily: uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.sampleRateShading = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(context.physicalDevice, &createInfo, nullptr, &context.device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    context.graphicsQueueFamily = indices.graphicsFamily.value();

    vkGetDeviceQueue(context.device, indices.graphicsFamily.value(), 0, &context.graphicsQueue);
    vkGetDeviceQueue(context.device, indices.presentFamily.value(), 0, &context.presentQueue);
}

void createSwapChain(VulkanBaseContext &context, SwapchainContext &swapchainContext, Window *window) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(context.physicalDevice, context.surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = context.surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(context.physicalDevice, context.surface);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE; // Clips pixel if they are for example obscured by another window

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(context.device, &createInfo, nullptr, &swapchainContext.swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(context.device, swapchainContext.swapChain, &imageCount, nullptr);
    swapchainContext.swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(context.device, swapchainContext.swapChain, &imageCount, swapchainContext.swapChainImages.data());

    swapchainContext.swapChainImageFormat = surfaceFormat.format;
    swapchainContext.swapChainExtent = extent;
}

void recreateSwapChain(ApplicationVulkanContext &appContext, RenderContext &renderContext) {
    int width = 0, height = 0;
    glfwGetFramebufferSize(appContext.window->getWindowHandle(), &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(appContext.window->getWindowHandle(), &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(appContext.baseContext.device);

    cleanupSwapChain(appContext.baseContext, appContext.swapchainContext);

    createSwapChain(appContext.baseContext, appContext.swapchainContext, appContext.window);
    createImageViews(appContext.baseContext, appContext.swapchainContext);

    // TODO don't need to create ColorRessources if multisampling is turned off
    createColorResources(appContext.baseContext, appContext.swapchainContext, appContext.graphicSettings);
    createDepthResources(appContext.baseContext, appContext.swapchainContext, appContext.graphicSettings);

    createFrameBuffers(appContext, renderContext);
}

void cleanupSwapChain(VulkanBaseContext &baseContext, SwapchainContext &swapchainContext) {
    vkDestroyImageView(baseContext.device, swapchainContext.colorImage.imageView, nullptr);
    vkDestroyImage(baseContext.device, swapchainContext.colorImage.image, nullptr);
    vkFreeMemory(baseContext.device, swapchainContext.colorImage.imageMemory, nullptr);

    vkDestroyImageView(baseContext.device, swapchainContext.depthImage.imageView, nullptr);
    vkDestroyImage(baseContext.device, swapchainContext.depthImage.image, nullptr);
    vkFreeMemory(baseContext.device, swapchainContext.depthImage.imageMemory, nullptr);

    for (auto framebuffer: swapchainContext.swapChainFramebuffers) {
        vkDestroyFramebuffer(baseContext.device, framebuffer, nullptr);
    }

    for (auto imageView: swapchainContext.swapChainImageViews) {
        vkDestroyImageView(baseContext.device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(baseContext.device, swapchainContext.swapChain, nullptr);
}

void createImageViews(VulkanBaseContext &context, SwapchainContext &swapchainContext) {
    swapchainContext.swapChainImageViews.resize(swapchainContext.swapChainImages.size());

    for (size_t i = 0; i < swapchainContext.swapChainImages.size(); i++) {
        swapchainContext.swapChainImageViews[i] = createImageView(context, swapchainContext.swapChainImages[i],
                                                                  swapchainContext.swapChainImageFormat,
                                                         VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

VkImageView createImageView(VulkanBaseContext &context, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags,
                            uint32_t mipLevels) {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;

    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;

    createInfo.subresourceRange.aspectMask = aspectFlags;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = mipLevels;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(context.device, &createInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }

    return imageView;
}

void createCommandPool(VulkanBaseContext &baseContext, CommandContext &commandContext) {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(baseContext.physicalDevice, baseContext.surface);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(baseContext.device, &poolInfo, nullptr, &commandContext.commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void createColorResources(VulkanBaseContext &baseContext, SwapchainContext &swapchainContext, GraphicSettings &graphicSettings) {
    VkFormat colorFormat = swapchainContext.swapChainImageFormat;

    createImage(baseContext,
                swapchainContext.swapChainExtent.width,
                swapchainContext.swapChainExtent.height,
                1,
                graphicSettings.msaaSamples,
                colorFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                swapchainContext.colorImage.image,
                swapchainContext.colorImage.imageMemory);

    swapchainContext.colorImage.imageView = createImageView(baseContext, swapchainContext.colorImage.image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void createDepthResources(VulkanBaseContext &baseContext, SwapchainContext &swapchainContext, GraphicSettings &graphicSettings) {
    VkFormat depthFormat = findDepthFormat(baseContext);

    createImage(baseContext,
                swapchainContext.swapChainExtent.width,
                swapchainContext.swapChainExtent.height,
                1,
                graphicSettings.msaaSamples,
                depthFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                swapchainContext.depthImage.image,
                swapchainContext.depthImage.imageMemory);

    swapchainContext.depthImage.imageView = createImageView(baseContext, swapchainContext.depthImage.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

void createCommandBuffers(VulkanBaseContext &baseContext, CommandContext &commandContext) {
    // context.commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandContext.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    // allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    // if (vkAllocateCommandBuffers(context.device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
    if (vkAllocateCommandBuffers(baseContext.device, &allocInfo, &commandContext.commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}