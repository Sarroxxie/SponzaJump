#include "VulkanSetup.h"
#include "VulkanSettings.h"
#include "GLFW/glfw3.h"
#include "utils/FileUtils.h"
#include "VulkanUtils.h"

#include <vulkan/vulkan_core.h>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <set>
#include <array>
#include <filesystem>

// setup in large parts taken from https://vulkan-tutorial.com/Introduction
void initializeGraphicsApplication(ApplicationContext &appContext) {
    initializeBaseVulkan(appContext);
    initializeSwapChain(appContext);
    initializeRenderContext(appContext);

    initializeImGui(appContext);

    createFrameBuffers(appContext);

    createVertexBuffer(appContext);
    createIndexBuffer(appContext);

    createDescriptorSetLayout(appContext);
    /*
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
     */
}

void initializeBaseVulkan(ApplicationContext &appContext) {
    createInstance(appContext.baseContext);
    setupDebugMessenger(appContext.baseContext);
    createSurface(appContext.baseContext, appContext.window);
    pickPhysicalDevice(appContext.baseContext, appContext.renderSettings);
    createLogicalDevice(appContext.baseContext);
}

void initializeSwapChain(ApplicationContext &appContext) {
    createSwapChain(appContext.baseContext, appContext.swapchainContext, appContext.window);
    createImageViews(appContext.baseContext, appContext.swapchainContext);

    // TODO don't need to create ColorRessources if multisampling is turned off
    createColorResources(appContext.baseContext, appContext.swapchainContext, appContext.renderSettings);
    createDepthResources(appContext.baseContext, appContext.swapchainContext, appContext.renderSettings);
}

void initializeRenderContext(ApplicationContext &appContext) {
    createRenderPass(appContext);

    createGraphicsPipeline(appContext,
                           appContext.renderContext,
                           appContext.renderContext.pipelineLayouts[0],
                           appContext.renderContext.graphicsPipelines[0]);

    createCommandPool(appContext.baseContext, appContext.renderContext);
    createCommandBuffers(appContext.baseContext, appContext.renderContext);
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
    appInfo.apiVersion = VK_API_VERSION_1_0;

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

void cleanupVulkan(ApplicationContext &appContext) {
    cleanupSwapChain(appContext.baseContext, appContext.swapchainContext);

    cleanupRenderContext(appContext.baseContext, appContext.renderContext);
    // TODO: create own context for imgui maybe
    cleanupImGuiContext(appContext.baseContext, appContext.renderContext);

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

void cleanupRenderContext(VulkanBaseContext &baseContext, RenderContext &renderContext) {
    vkDestroyCommandPool(baseContext.device, renderContext.commandPool, nullptr);

    // delete graphics pipeline(s) (if the 2nd one was created, delete that too)
    vkDestroyPipeline(baseContext.device, renderContext.graphicsPipelines[0], nullptr);
    vkDestroyPipelineLayout(baseContext.device, renderContext.pipelineLayouts[0], nullptr);
    if(renderContext.graphicsPipelines[1] != VK_NULL_HANDLE) {
        vkDestroyPipeline(baseContext.device, renderContext.graphicsPipelines[1], nullptr);
        vkDestroyPipelineLayout(baseContext.device, renderContext.pipelineLayouts[1], nullptr);
    }

    vkDestroyRenderPass(baseContext.device, renderContext.renderPass, nullptr);

    vkDestroyBuffer(baseContext.device, renderContext.object.indexBuffer, nullptr);
    vkFreeMemory(baseContext.device, renderContext.object.indexBufferMemory, nullptr);

    vkDestroyBuffer(baseContext.device, renderContext.object.vertexBuffer, nullptr);
    vkFreeMemory(baseContext.device, renderContext.object.vertexBufferMemory, nullptr);
}

// @IMGUI
void cleanupImGuiContext(VulkanBaseContext& baseContext, RenderContext& renderContext) {
    vkDestroyDescriptorPool(baseContext.device, renderContext.imGuiDescriptorPool, nullptr);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
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

void pickPhysicalDevice(VulkanBaseContext &context, RenderSettings &renderSettings) {
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
            renderSettings.maxMsaaSamples = getMaxUsableSampleCount(device);
            break;
        }
    }

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

void recreateSwapChain(ApplicationContext &appContext) {
    int width = 0, height = 0;
    glfwGetFramebufferSize(appContext.window->getWindowHandle(), &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(appContext.window->getWindowHandle(), &width, &height);
        glfwWaitEvents();
    }

    // @IMGUI
    if(ImGui::GetCurrentContext() != nullptr) {
        ImGui::EndFrame();
        auto& imguiIO = ImGui::GetIO();
        imguiIO.DisplaySize =
            ImVec2(static_cast<float>(width), static_cast<float>(height));
    }

    vkDeviceWaitIdle(appContext.baseContext.device);

    cleanupSwapChain(appContext.baseContext, appContext.swapchainContext);

    createSwapChain(appContext.baseContext, appContext.swapchainContext, appContext.window);
    createImageViews(appContext.baseContext, appContext.swapchainContext);

    // TODO don't need to create ColorRessources if multisampling is turned off
    createColorResources(appContext.baseContext, appContext.swapchainContext, appContext.renderSettings);
    createDepthResources(appContext.baseContext, appContext.swapchainContext, appContext.renderSettings);

    createFrameBuffers(appContext);
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

void createRenderPass(ApplicationContext &appContext) {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = appContext.swapchainContext.swapChainImageFormat;
    colorAttachment.samples = appContext.renderSettings.msaaSamples;

    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; //This is overwritten when using msaa to better layout


    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat(appContext.baseContext);
    depthAttachment.samples = appContext.renderSettings.msaaSamples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::vector<VkAttachmentDescription> attachments({colorAttachment, depthAttachment});

    // TODO: if samples is VK_SAMPLE_COUNT_1_BIT Vulkan expects no resolveAttachment
    // This has not been tested yet -> might be error source
    // Might also be important in createFrameBuffers ? (not sure, might also only save some memory)

    if (appContext.renderSettings.useMsaa) {
        // when using Msaa, the color attachment is not the attachment presenting
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = appContext.swapchainContext.swapChainImageFormat;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        subpass.pResolveAttachments = &colorAttachmentResolveRef;

        attachments.push_back(colorAttachmentResolve);
    }

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;

    // @IMGUI
    dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;

    // @IMGUI
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(appContext.baseContext.device, &renderPassInfo, nullptr, &appContext.renderContext.renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

VkFormat findDepthFormat(VulkanBaseContext &context) {
    return findSupportedFormat(
            context,
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkFormat findSupportedFormat(VulkanBaseContext &context, const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                             VkFormatFeatureFlags features) {
    for (VkFormat format: candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(context.physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

void createDescriptorSetLayout(ApplicationContext &appContext) {
    // TODO: currently empty, this is used to create descriptors for shaders, so we can pass data -> Maybe use some sort of struct to define what we want, so we don't have to hardcode it.
    // -> currently setting DescriptorSetLayouts for pipelineLayoutInfo in createGraphicsPipeline is commented out
    /*
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 2;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
     */
}

void createGraphicsPipeline(ApplicationContext &appContext,
                            RenderContext &renderContext,
                            VkPipelineLayout& pipelineLayout,
                            VkPipeline&       graphicsPipeline,
                            std::string       vertexShaderPath,
                            std::string       fragmentShaderPath) {
    // TODO hardcoded shaders to display triangle
    std::vector<char> vertexShaderCode;
    std::vector<char> fragmentShaderCode;

    if (!std::filesystem::exists(vertexShaderPath)) {
        compileShader("triangle.vert");
    }
    if (!readFile(vertexShaderPath, vertexShaderCode)) {
        throw std::runtime_error("failed to open vertex shader code!");
    }
    if (!std::filesystem::exists(fragmentShaderPath)) {
        compileShader("triangle.frag");
    }
    if(!readFile(fragmentShaderPath, fragmentShaderCode)) {
        throw std::runtime_error("failed to open fragment shader code!");
    }

    VkShaderModule vertShaderModule = createShaderModule(appContext.baseContext, vertexShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(appContext.baseContext, fragmentShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    // Can set compile time constants here
    // -> Let compiler optimize
    // vertShaderStageInfo.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;

    rasterizer.rasterizerDiscardEnable = VK_FALSE;

    // Enable Wireframe rendering here, requires GPU feature to be enabled
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

    rasterizer.lineWidth = 1.0f;
    // rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_TRUE;
    multisampling.minSampleShading = .2f;
    multisampling.rasterizationSamples = appContext.renderSettings.msaaSamples;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
    // colorBlendAttachment.blendEnable = VK_FALSE;

    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;


    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // Optional

    //pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    if(vkCreatePipelineLayout(appContext.baseContext.device, &pipelineLayoutInfo, nullptr,
                              &pipelineLayout)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.pDepthStencilState = &depthStencil;

    pipelineInfo.layout = pipelineLayout;

    pipelineInfo.renderPass = renderContext.renderPass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if(vkCreateGraphicsPipelines(appContext.baseContext.device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                 nullptr, &graphicsPipeline)
       !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(appContext.baseContext.device, fragShaderModule, nullptr);
    vkDestroyShaderModule(appContext.baseContext.device, vertShaderModule, nullptr);
}

VkShaderModule createShaderModule(VulkanBaseContext &context, const std::vector<char> &code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(context.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

void createCommandPool(VulkanBaseContext &baseContext, RenderContext &renderContext) {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(baseContext.physicalDevice, baseContext.surface);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(baseContext.device, &poolInfo, nullptr, &renderContext.commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void createColorResources(VulkanBaseContext &baseContext, SwapchainContext &swapchainContext, RenderSettings &renderSettings) {
    VkFormat colorFormat = swapchainContext.swapChainImageFormat;

    createImage(baseContext,
                swapchainContext.swapChainExtent.width,
                swapchainContext.swapChainExtent.height,
                1,
                renderSettings.msaaSamples,
                colorFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                swapchainContext.colorImage.image,
                swapchainContext.colorImage.imageMemory);

    swapchainContext.colorImage.imageView = createImageView(baseContext, swapchainContext.colorImage.image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void createDepthResources(VulkanBaseContext &baseContext, SwapchainContext &swapchainContext, RenderSettings &renderSettings) {
    VkFormat depthFormat = findDepthFormat(baseContext);

    createImage(baseContext,
                swapchainContext.swapChainExtent.width,
                swapchainContext.swapChainExtent.height,
                1,
                renderSettings.msaaSamples,
                depthFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                swapchainContext.depthImage.image,
                swapchainContext.depthImage.imageMemory);

    swapchainContext.depthImage.imageView = createImageView(baseContext, swapchainContext.depthImage.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

void createFrameBuffers(ApplicationContext &appContext) {
    appContext.swapchainContext.swapChainFramebuffers.resize(appContext.swapchainContext.swapChainImageViews.size());

    for (size_t i = 0; i < appContext.swapchainContext.swapChainImageViews.size(); i++) {

        std::vector<VkImageView> attachments;

        if (appContext.renderSettings.useMsaa) {
            attachments.push_back(appContext.swapchainContext.colorImage.imageView);
            attachments.push_back(appContext.swapchainContext.depthImage.imageView);
            attachments.push_back(appContext.swapchainContext.swapChainImageViews[i]);
        } else {
            attachments.push_back(appContext.swapchainContext.swapChainImageViews[i]);
            attachments.push_back(appContext.swapchainContext.depthImage.imageView);
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = appContext.renderContext.renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = appContext.swapchainContext.swapChainExtent.width;
        framebufferInfo.height = appContext.swapchainContext.swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(appContext.baseContext.device, &framebufferInfo, nullptr, &appContext.swapchainContext.swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }

}

void createVertexBuffer(ApplicationContext &appContext) {
    const std::vector<Vertex> vertices({
            {{0.0f, -0.5f, 0.5}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, 0.5f, 0}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, 0.5f, 0.1}, {0.0f, 0.0f, 1.0f}}
    });

    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(appContext.baseContext,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer,
                 stagingBufferMemory);

    void *data;
    vkMapMemory(appContext.baseContext.device, stagingBufferMemory, 0, bufferSize, 0, &data);

    // We use Host Coherent Memory to make sure data is synchronized, could also manually flush Memory Ranges
    std::memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(appContext.baseContext.device, stagingBufferMemory);

    createBuffer(appContext.baseContext,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 appContext.renderContext.object.vertexBuffer,
                 appContext.renderContext.object.vertexBufferMemory);

    copyBuffer(appContext.baseContext, appContext.renderContext, stagingBuffer, appContext.renderContext.object.vertexBuffer, bufferSize);

    vkDestroyBuffer(appContext.baseContext.device, stagingBuffer, nullptr);
    vkFreeMemory(appContext.baseContext.device, stagingBufferMemory, nullptr);
}

void createIndexBuffer(ApplicationContext &appContext) {
    const std::vector<uint32_t> indices({0, 1, 2});

    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(appContext.baseContext,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer,
                 stagingBufferMemory);

    void *data;
    vkMapMemory(appContext.baseContext.device, stagingBufferMemory, 0, bufferSize, 0, &data);

    // We use Host Coherent Memory to make sure data is synchronized, could also manually flush Memory Ranges
    memcpy(data, indices.data(), (size_t) bufferSize);
    vkUnmapMemory(appContext.baseContext.device, stagingBufferMemory);

    createBuffer(appContext.baseContext,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 appContext.renderContext.object.indexBuffer,
                 appContext.renderContext.object.indexBufferMemory);

    copyBuffer(appContext.baseContext, appContext.renderContext, stagingBuffer, appContext.renderContext.object.indexBuffer, bufferSize);

    vkDestroyBuffer(appContext.baseContext.device, stagingBuffer, nullptr);
    vkFreeMemory(appContext.baseContext.device, stagingBufferMemory, nullptr);
}

void createCommandBuffers(VulkanBaseContext &baseContext, RenderContext &renderContext) {
    // context.commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = renderContext.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    // allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    // if (vkAllocateCommandBuffers(context.device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
    if (vkAllocateCommandBuffers(baseContext.device, &allocInfo, &renderContext.commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

// Builds a graphics pipeline and stores it in the secondary slot
void buildSecondaryGraphicsPipeline(ApplicationContext &appContext) {
    compileShader("triangle.frag");
    compileShader("triangle.vert");
    if(appContext.renderContext.graphicsPipelines[!appContext.renderContext.activePipelineIndex] != VK_NULL_HANDLE) {
        vkDestroyPipeline(appContext.baseContext.device,
                          appContext.renderContext.graphicsPipelines[!appContext.renderContext.activePipelineIndex], nullptr);
        vkDestroyPipelineLayout(appContext.baseContext.device,
                                appContext.renderContext.pipelineLayouts[!appContext.renderContext.activePipelineIndex],
                                nullptr);
    }

    createGraphicsPipeline(appContext,
                           appContext.renderContext,
                           appContext.renderContext.pipelineLayouts[!appContext.renderContext.activePipelineIndex],
                           appContext.renderContext.graphicsPipelines[!appContext.renderContext.activePipelineIndex]);
}

// Swaps to the secondary graphics pipeline if it is valid
bool swapGraphicsPipeline(ApplicationContext &appContext) {
    if(appContext.renderContext.graphicsPipelines[!appContext.renderContext.activePipelineIndex] != VK_NULL_HANDLE) {
        appContext.renderContext.activePipelineIndex = !appContext.renderContext.activePipelineIndex;
        return true;
    } else {
        return false;
    }
}

// Compiles a shader to SPIR-V format using a systemcall for "glslangValidator.exe".
// The shader path has to be relative to the shaderDirectoryPath.
void compileShader(std::string relativeShaderPath, std::string shaderDirectoryPath) {
    std::string command = std::string(VULKAN_GLSLANG_VALIDATOR_PATH)
                          + " -g --target-env vulkan1.0 -o \"" + shaderDirectoryPath
                          + "../spv/" + relativeShaderPath + ".spv\" \""
                          + shaderDirectoryPath + relativeShaderPath + "\"";
    // this suppresses the console output from the command (command differs on windows and unix)
    /*#if defined(_WIN32) || defined(_WIN64)
        command += " > nul";
    #else
        command += " > /dev/null";
    #endif*/
    system(command.c_str());
}

// @IMGUI
// Heavily inspired by "https://github.com/ocornut/imgui/blob/master/examples/example_glfw_vulkan/main.cpp"
void initializeImGui(ApplicationContext& appContext) {
    // Create ImGui Context and set style
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    // TODO: is this line necessary?
    //io.IniFilename = nullptr;
    //io.LogFilename = nullptr;
    ImGui::StyleColorsDark();

    // Create Descriptor Pool for ImGui
    //ImGui_ImplGlfw_InitForVulkan(appContext.window->getWindowHandle(), true);
    // TODO: need to call 
    // "vkDestroyDescriptorPool(g_Device, g_DescriptorPool, nullptr);" somewhere!!
    //       -> save handle to descriptor pool somewhere
    VkDescriptorPoolSize poolSizes[]    = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets       = 1000 * IM_ARRAYSIZE(poolSizes);
    poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
    poolInfo.pPoolSizes    = poolSizes;
    vkCreateDescriptorPool(appContext.baseContext.device, &poolInfo, nullptr,
                           &appContext.renderContext.imGuiDescriptorPool);

    // initializes ImGui
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(appContext.baseContext.device,
                            appContext.swapchainContext.swapChain, &imageCount, nullptr);
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance                  = appContext.baseContext.instance;
    initInfo.PhysicalDevice            = appContext.baseContext.physicalDevice;
    initInfo.Device                    = appContext.baseContext.device;
    initInfo.QueueFamily    = appContext.baseContext.graphicsQueueFamily;
    initInfo.Queue          = appContext.baseContext.graphicsQueue;
    initInfo.PipelineCache  = VK_NULL_HANDLE;
    initInfo.DescriptorPool = appContext.renderContext.imGuiDescriptorPool;
    // TODO: check for interference here
    initInfo.Subpass        = 0;
    initInfo.Allocator      = VK_NULL_HANDLE;
    // TODO: find out if these are the correct values
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = imageCount;
    // TODO: could implement to check for errors
    initInfo.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&initInfo, appContext.renderContext.renderPass);

        // Upload fonts to GPU
    VkCommandBuffer commandBuffer =
        beginSingleTimeCommands(appContext.baseContext, appContext.renderContext);
    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
    endSingleTimeCommands(appContext.baseContext, appContext.renderContext, commandBuffer);
}