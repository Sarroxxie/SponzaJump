#ifndef GRAPHICSPRAKTIKUM_RENDERSETUPDESCRIPTION_H
#define GRAPHICSPRAKTIKUM_RENDERSETUPDESCRIPTION_H

#include <vulkan/vulkan_core.h>
#include <vector>
#include "Shader.h"

typedef struct s_renderPassDescription
{
    Shader vertexShader;
    Shader fragmentShader;

    std::vector<VkPushConstantRange> pushConstantRanges;

    bool enableDepthBias = false;
}RenderPassDescription;

typedef struct
{
    RenderPassDescription mainRenderPassDescription;

    RenderPassDescription shadowPassDescription;

    bool enableImgui = false;
} RenderSetupDescription;

#endif //GRAPHICSPRAKTIKUM_RENDERSETUPDESCRIPTION_H
