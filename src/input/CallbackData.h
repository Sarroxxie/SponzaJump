#ifndef GRAPHICSPRAKTIKUM_CALLBACKDATA_H
#define GRAPHICSPRAKTIKUM_CALLBACKDATA_H

#include "vulkan/VulkanRenderer.h"
#include "InputController.h"

struct CallbackData {
    InputController *inputController;
    VulkanRenderer *renderer;
};

#endif //GRAPHICSPRAKTIKUM_CALLBACKDATA_H
