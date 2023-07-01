#ifndef GRAPHICSPRAKTIKUM_RENDERCONTEXT_H
#define GRAPHICSPRAKTIKUM_RENDERCONTEXT_H

#include <vulkan/vulkan_core.h>
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "Shader.h"
#include "RenderSetupDescription.h"
#include "scene/Camera.h"
#include "host_device.h"

typedef struct
{
    alignas(16) glm::mat4 perspectiveTransform;
    alignas(16) glm::mat4 cameraTransform;
} SceneTransform;


typedef struct
{
    VkRenderPass renderPass;

    VkPipelineLayout pipelineLayouts[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkPipeline       graphicsPipelines[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};

    bool activePipelineIndex = 0;

    RenderPassDescription renderPassDescription;

    // redundant information used to recreate graphic pipelines
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    // TODO
    /*
     * 1. fix all compile errors
     * 2. create main pass descriptorSetLayouts using stored layouts after textures are created
     * 3. Create main pass descriptorSets
     * 4. Find better way to create DescriptorSetLayouts (and maybe descriptorSets ?)
     *      -> maybe recreate DescriptorSetLayouts once all is known
     *      -> maybe use constant oversize of buffers etc.
     */
} RenderPassContext;

typedef struct
{
    VkBuffer       buffer              = VK_NULL_HANDLE;
    VkDeviceMemory bufferMemory        = VK_NULL_HANDLE;
    void*          bufferMemoryMapping = VK_NULL_HANDLE;
} BufferResources;

typedef struct
{
    VkImage        image     = VK_NULL_HANDLE;
    VkDeviceMemory memory    = VK_NULL_HANDLE;
    VkImageView    imageView = VK_NULL_HANDLE;
} ImageResources;

typedef struct
{
    ImageResources depthImages[MAX_CASCADES];
    VkFramebuffer  depthFrameBuffers[MAX_CASCADES];

    BufferResources transformBuffer;

    VkDescriptorSetLayout transformDescriptorSetLayout;
    VkDescriptorSet       transformDescriptorSet;

    uint32_t shadowMapWidth;
    uint32_t shadowMapHeight;

    RenderPassContext renderPassContext;
} ShadowPass;

typedef struct
{
    VkDescriptorSetLayout transformDescriptorSetLayout;
    VkDescriptorSet       transformDescriptorSet;

    BufferResources transformBuffer;
    BufferResources lightingBuffer;
    BufferResources cascadeSplitsBuffer;

    VkDescriptorSetLayout materialDescriptorSetLayout;
    VkDescriptorSet       materialDescriptorSet;

    VkDescriptorSetLayout depthDescriptorSetLayout;
    VkDescriptorSet       depthDescriptorSet;

    VkPipelineLayout visualizePipelineLayout;
    VkPipeline       visualizePipeline;

    VkPipelineLayout skyboxPipelineLayout;
    VkPipeline       skyboxPipeline;
    uint32_t         skyboxSubpassID = 0;

    BufferResources materialBuffer;

    VkSampler depthSampler;

    RenderPassContext renderPassContext;
} MainPass;

typedef struct
{
    MainPass mainPass;

    ShadowPass shadowPass;
} RenderPasses;

typedef struct
{
    bool lockCamera = true;

    bool visualizeShadowBuffer = false;
    bool playerSpikesShadow    = false;
    bool doPCF                 = false;

    float depthBiasConstant = 0.0f;
    float depthBiasSlope = 2.0f;
} ImguiData;

typedef struct
{
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
} ImguiContext;

typedef struct
{
    float widthHeightDim;

    float zNear;
    float zFar;
} OrthoSettings;

typedef struct
{
    float fov;
    float nearPlane;
    float farPlane;
} PerspectiveSettings;

typedef struct
{
    // color of sunlight according to https://www.color-name.com/sunlight.color
    glm::vec3 sunColor = glm::vec3(0.95686, 0.91373, 0.60784);
} LightingSettings;

typedef struct
{
    Camera        lightCamera;
    float lightCameraDist = 60;
    bool snapToPlayer = true;

    OrthoSettings projection;

    int numberCascades = MAX_CASCADES;
    int cascadeVisIndex = 0;
    // controls the blending between logarithmic (1) and linear (0) cascade splits
    float cascadeSplitsBlendFactor = 0.5;
} ShadowMappingSettings;

typedef struct
{
    ShadowMappingSettings shadowMappingSettings;

    PerspectiveSettings perspectiveSettings;

    LightingSettings lightingSetting;
} RenderSettings;

static inline glm::mat4 getPerspectiveMatrix(PerspectiveSettings perspectiveSettings,
                                             size_t width,
                                             size_t height) {
    return glm::perspective<float>(perspectiveSettings.fov,
                                   static_cast<float>(width) / static_cast<float>(height),
                                   perspectiveSettings.nearPlane,
                                   perspectiveSettings.farPlane);
}

static inline glm::mat4 getOrthogonalProjectionMatrix(OrthoSettings orthoSettings) {
    return glm::ortho(-orthoSettings.widthHeightDim / 2, orthoSettings.widthHeightDim / 2,
                      -orthoSettings.widthHeightDim / 2, orthoSettings.widthHeightDim / 2,
                      orthoSettings.zNear, orthoSettings.zFar);
}

typedef struct s_renderContext
{
    RenderPasses renderPasses;

    VkDescriptorPool descriptorPool;

    bool         usesImgui;
    ImguiContext imguiContext;
    ImguiData    imguiData;

    RenderSettings renderSettings;

    RenderSetupDescription renderSetupDescription;
} RenderContext;


#endif  // GRAPHICSPRAKTIKUM_RENDERCONTEXT_H
