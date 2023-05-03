#ifndef GRAPHICSPRAKTIKUM_VULKANSETTINGS_H
#define GRAPHICSPRAKTIKUM_VULKANSETTINGS_H

#include <vector>

static const bool enableValidationLayers = true;

const std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

typedef struct VulkanSettings_s {
    bool useMsaa = false;
    uint8_t maxMsaaSamples = 1;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
} VulkanSettings;

#endif //GRAPHICSPRAKTIKUM_VULKANSETTINGS_H
