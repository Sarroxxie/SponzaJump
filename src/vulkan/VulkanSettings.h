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

#endif //GRAPHICSPRAKTIKUM_VULKANSETTINGS_H
