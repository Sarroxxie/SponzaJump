#ifndef GRAPHICSPRAKTIKUM_RENDERSETUPDESCRIPTION_H
#define GRAPHICSPRAKTIKUM_RENDERSETUPDESCRIPTION_H

#include <vulkan/vulkan_core.h>
#include <vector>
#include "Shader.h"

typedef struct
{
    Shader vertexShader;
    Shader fragmentShader;

    std::vector<VkDescriptorSetLayoutBinding> bindings;

    std::vector<VkPushConstantRange> pushConstantRanges;
} RenderSetupDescription;

#endif //GRAPHICSPRAKTIKUM_RENDERSETUPDESCRIPTION_H
