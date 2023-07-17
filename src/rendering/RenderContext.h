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
    VkFormat       imageFormat;
} ImageResources;

typedef struct
{
    ImageResources depthImages[MAX_CASCADES];
    VkFramebuffer  depthFrameBuffers[MAX_CASCADES];

    BufferResources transformBuffer;

    VkDescriptorSetLayout transformDescriptorSetLayout;
    VkDescriptorSet       transformDescriptorSet;

    VkDescriptorSetLayout materialDescriptorSetLayout;
    VkDescriptorSet       materialDescriptorSet;

    VkPipelineLayout shadowPipelineLayout;
    VkPipeline       shadowPipeline;


    uint32_t shadowMapWidth;
    uint32_t shadowMapHeight;

    RenderPassContext renderPassContext;
    ShadowPushConstant shadowPushConstant;
} ShadowPass;

typedef struct
{
    // descriptor stuff

    BufferResources transformBuffer;
    BufferResources lightingBuffer;
    BufferResources materialBuffer;
    BufferResources cascadeSplitsBuffer;

    VkDescriptorSetLayout transformDescriptorSetLayout;
    VkDescriptorSet       transformDescriptorSet;

    VkDescriptorSetLayout materialDescriptorSetLayout;
    VkDescriptorSet       materialDescriptorSet;

    VkDescriptorSetLayout depthDescriptorSetLayout;
    VkDescriptorSet       depthDescriptorSet;

    VkDescriptorSetLayout gBufferDescriptorSetLayout;
    VkDescriptorSet       gBufferDescriptorSet;

    VkDescriptorSetLayout skyboxDescriptorSetLayout;
    VkDescriptorSet       skyboxDescriptorSet;
    
    // Push Constants
    PushConstant              pushConstant;
    StencilPushConstant       stencilPushConstant;
    PointLightPushConstant    pointLightPushConstant;
    SkyboxPushConstant        skyboxPushConstant;
    ShadowControlPushConstant shadowControlPushConstant;
    CameraUniform             cameraUniform;

    // pipelines
    // visualize shadow map
    VkPipelineLayout visualizePipelineLayout;
    VkPipeline       visualizePipeline;

    // filling gBuffer
    VkPipelineLayout geometryPassPipelineLayout;
    VkPipeline       geometryPassPipeline;

    // rendering full screen quad for directional light (and later IBL)
    VkPipelineLayout primaryLightingPipelineLayout;
    VkPipeline       primaryLightingPipeline;

    // stencil shadow volumes (see https://ogldev.org/www/tutorial37/tutorial37.html)
    VkPipelineLayout stencilPipelineLayout;
    VkPipeline       stencilPipeline;

    // render point lights
    VkPipelineLayout pointLightsPipelineLayout;
    VkPipeline       pointLightsPipeline;
    
    // render skybox
    VkPipelineLayout skyboxPipelineLayout;
    VkPipeline       skyboxPipeline;
    uint32_t         skyboxSubpassID = 0;

    // deferred framebuffer attachments
    VkFramebuffer gBuffer;
    // TODO: position could be calculated from depth but this may be harder to implement
    ImageResources normalAttachment;
    ImageResources albedoAttachment;
    ImageResources aoRoughnessMetallicAttachment;
    ImageResources depthAttachment;
    VkSampler      framebufferAttachmentSampler;
    // TODO: needs to be destroyed
    VkRenderPass geometryPass;

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
    float yOffset    = 10.0f;

    bool visualizeShadowBuffer = false;
    bool playerSpikesShadow    = true;
    bool doPCF                 = true;

    float depthBiasConstant = 0.0f;
    float depthBiasSlope    = 2.0f;

    int meshDrawCalls       = 0;
    int lightDrawCalls      = 0;
    int shadowPassDrawCalls = 0;

    bool pointLights = true;
    // NOTE: this must always be true on startup, can modify at runtime via ImGui
    bool  shadows  = true;
    bool  autoIbl   = true;
    float iblFactor = 0.1f;
    float exposure = 3.0f;
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
    LightingInformation lightingInformation;
    CameraUniform       cameraUniform;
} Uniforms;

typedef struct
{
    // color of sunlight according to https://www.color-name.com/sunlight.color
    glm::vec3 sunColor = glm::vec3(0.95686, 0.91373, 0.60784);
} LightingSettings;

typedef struct
{
    glm::vec3 lightDirection = glm::vec3(-0.452f, -1.0f, -0.095f);
    OrthoSettings projection;

    int numberCascades = MAX_CASCADES;

    int cascadeVisIndex = 0;

    bool visualizeCascades = false;

    // Extra offset for distance of the light camera to account for objects
    // outside of the view frustum along the light direction
    float lightCameraZOffset = 200;

    // controls the blending between logarithmic (1) and linear (0) cascade splits
    float cascadeSplitsBlendFactor = 0.5;
    bool  newCascadeCalculation    = true;
    bool  crossProductUp           = false;
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
    Uniforms       uniforms;

    RenderSetupDescription renderSetupDescription;
} RenderContext;


#endif  // GRAPHICSPRAKTIKUM_RENDERCONTEXT_H
