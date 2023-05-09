#ifndef GRAPHICSPRAKTIKUM_RENDERCONTEXT_H
#define GRAPHICSPRAKTIKUM_RENDERCONTEXT_H

#include <vulkan/vulkan_core.h>
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "Shader.h"
#include "RenderSetupDescription.h"

typedef struct
{
    alignas(16) glm::mat4 perspectiveTransform;
    alignas(16) glm::mat4 cameraTransform;
} SceneTransform;

typedef struct
{
    // TODO how to use multiple Renderpasses, multiple pipelines ?
    VkRenderPass renderPass;

    // allows swapping between graphics pipeline at runtime
    VkPipelineLayout pipelineLayouts[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkPipeline graphicsPipelines[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};

    bool activePipelineIndex = 0;

    VkDescriptorSetLayout descriptorSetLayout;
} RenderPassContext;

typedef struct {
    float fov;
    float nearPlane;
    float farPlane;
} RenderSettings;

static inline glm::mat4 getPerspectiveMatrix(RenderSettings renderSettings, size_t width, size_t height) {
    return glm::perspective<float>(renderSettings.fov, static_cast<float>(width) / static_cast<float>(height), renderSettings.nearPlane, renderSettings.farPlane);
}

typedef struct {
    RenderPassContext renderPassContext;
    RenderSettings renderSettings;

    RenderSetupDescription renderSetupDescription;
} RenderContext;


#endif //GRAPHICSPRAKTIKUM_RENDERCONTEXT_H
