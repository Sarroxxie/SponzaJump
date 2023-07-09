#include <stdexcept>
#include <filesystem>
#include "RenderSetup.h"
#include "vulkan/VulkanUtils.h"
#include "rendering/host_device.h"
#include "vulkan/VulkanSetup.h"

RenderSetupDescription initializeSimpleSceneRenderContext(ApplicationVulkanContext& appContext,
                                                          RenderContext& renderContext,
                                                          Scene& scene) {
    auto& settings                         = renderContext.renderSettings;
    settings.perspectiveSettings.fov       = glm::radians(45.0f);
    settings.perspectiveSettings.nearPlane = 1;
    settings.perspectiveSettings.farPlane  = 300;

    ShadowMappingSettings shadowMappingSettings;
    shadowMappingSettings.lightCamera =
        Camera(glm::vec3(50, 50, 20), glm::normalize(glm::vec3(-50, -50, -20)));
    shadowMappingSettings.projection.widthHeightDim = 50;
    shadowMappingSettings.projection.zNear          = 1;
    shadowMappingSettings.projection.zFar           = 100;

    settings.shadowMappingSettings = shadowMappingSettings;

    RenderSetupDescription renderSetupDescription;
    renderSetupDescription.enableImgui = true;

    // -- Shadow Pass
    RenderPassDescription shadowPassDescription;

    shadowPassDescription.vertexShader.shaderStage = ShaderStage::VERTEX_SHADER;
    shadowPassDescription.vertexShader.shaderSourceName = "shadow.vert";
    shadowPassDescription.vertexShader.sourceDirectory  = "res/shaders/source/";
    shadowPassDescription.vertexShader.spvDirectory     = "res/shaders/spv/";

    shadowPassDescription.fragmentShader.shaderStage = ShaderStage::FRAGMENT_SHADER;
    shadowPassDescription.fragmentShader.shaderSourceName = "shadow.frag";
    shadowPassDescription.fragmentShader.sourceDirectory = "res/shaders/source/";
    shadowPassDescription.fragmentShader.spvDirectory = "res/shaders/spv/";

    shadowPassDescription.pushConstantRanges.push_back(createPushConstantRange(
        0, sizeof(ShadowPushConstant), getStageFlag(ShaderStage::VERTEX_SHADER)));

    shadowPassDescription.enableDepthBias = true;

    renderSetupDescription.shadowPassDescription = shadowPassDescription;

    // --- Main Render Pass
    RenderPassDescription mainRenderPassDescription;

    mainRenderPassDescription.vertexShader.shaderStage = ShaderStage::VERTEX_SHADER;
    mainRenderPassDescription.vertexShader.shaderSourceName = "mainPass.vert";
    mainRenderPassDescription.vertexShader.sourceDirectory = "res/shaders/source/";
    mainRenderPassDescription.vertexShader.spvDirectory = "res/shaders/spv/";

    renderSetupDescription.mainRenderPassDescription.pushConstantRanges.push_back(
        createPushConstantRange(0, sizeof(PushConstant),
                                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT));

    mainRenderPassDescription.fragmentShader.shaderStage = ShaderStage::FRAGMENT_SHADER;
    mainRenderPassDescription.fragmentShader.shaderSourceName = "mainPass.frag";
    mainRenderPassDescription.fragmentShader.sourceDirectory = "res/shaders/source/";
    mainRenderPassDescription.fragmentShader.spvDirectory = "res/shaders/spv/";

    mainRenderPassDescription.pushConstantRanges.push_back(
        createPushConstantRange(0, sizeof(PushConstant),
                                getStageFlag(ShaderStage::VERTEX_SHADER)
                                    | getStageFlag(ShaderStage::FRAGMENT_SHADER)));

    /*
    VkPushConstantRange pushConstantRange;
    // this push constant range starts at the beginning
    pushConstantRange.offset = 0;
    // this push constant range takes up the size of a PushConstant struct
    pushConstantRange.size = sizeof(PushConstant);
    // this push constant range is accessible in the vertex and fragment shader
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    */

    renderSetupDescription.mainRenderPassDescription = mainRenderPassDescription;

    initializeRenderContext(appContext, renderContext, renderSetupDescription, scene);
    return renderSetupDescription;
}

void initializeRenderContext(ApplicationVulkanContext& appContext,
                             RenderContext&            renderContext,
                             const RenderSetupDescription& renderSetupDescription,
                             Scene& scene) {

    createDescriptorPool(appContext.baseContext, renderContext);

    // --- Shadow Pass

    initializeShadowPass(appContext, renderContext, renderSetupDescription.shadowPassDescription);

    createShadowPassResources(appContext, renderContext);
    createShadowPassDescriptorSets(appContext, renderContext);

    // --- Main Render Pass
    initializeMainRenderPass(appContext, renderContext,
                             renderSetupDescription.mainRenderPassDescription, scene);


    renderContext.renderSetupDescription = renderSetupDescription;
    createFrameBuffers(appContext, renderContext);

    renderContext.usesImgui = renderSetupDescription.enableImgui;
    if(renderContext.usesImgui) {
        initializeImGui(appContext, renderContext);
    }
}

void initializeMainRenderPass(const ApplicationVulkanContext& appContext,
                              RenderContext&                  renderContext,
                              const RenderPassDescription& renderPassDescription,
                              Scene& scene) {

    createMainPassDescriptorSetLayouts(appContext, renderContext.renderPasses.mainPass, scene);

    createMainRenderPass(appContext, renderContext);

    createGeometryRenderPass(appContext, renderContext);

    createDepthSampler(appContext, renderContext.renderPasses.mainPass);
    createMainPassResources(appContext, renderContext, scene);

    createGraphicsPipeline(
        appContext, renderContext.renderPasses.mainPass.renderPassContext,
        renderContext.renderPasses.mainPass.renderPassContext.pipelineLayouts[0],
        renderContext.renderPasses.mainPass.renderPassContext.graphicsPipelines[0],
        renderPassDescription,
        renderContext.renderPasses.mainPass.renderPassContext.descriptorSetLayouts);

    // TODO: skybox should have own descriptor set
    // same goes for the skybox pipeline (as it uses the descriptor set that
    // contains the textures for now)
    createGeometryPassPipeline(appContext, renderContext, renderPassDescription,
                               renderContext.renderPasses.mainPass);
    createPrimaryLightingPipeline(appContext, renderContext,
                                  renderContext.renderPasses.mainPass);
    createSkyboxPipeline(appContext, renderContext, renderContext.renderPasses.mainPass);

    createMainPassDescriptorSets(appContext, renderContext, scene);

    renderContext.renderPasses.mainPass.renderPassContext.renderPassDescription =
        renderPassDescription;

    createVisualizationPipeline(appContext, renderContext,
                                renderContext.renderPasses.mainPass);
}

void createDescriptorSetLayout(const VulkanBaseContext& context,
                               VkDescriptorSetLayout&   layout,
                               const std::vector<VkDescriptorSetLayoutBinding>& bindings) {
    if(bindings.empty())
        return;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings    = bindings.data();

    if(vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}


void cleanupRenderContext(const VulkanBaseContext& baseContext, RenderContext& renderContext) {
    if(renderContext.usesImgui) {
        cleanupImGuiContext(baseContext, renderContext);
    }

    cleanDeferredPass(baseContext, renderContext.renderPasses.mainPass);

    cleanMainPass(baseContext, renderContext.renderPasses.mainPass);

    cleanShadowPass(baseContext, renderContext.renderPasses.shadowPass);

    vkDestroyDescriptorPool(baseContext.device, renderContext.descriptorPool, nullptr);
}

void cleanupRenderPassContext(const VulkanBaseContext& baseContext,
                              const RenderPassContext& renderPassContext) {
    // delete graphics pipeline(s) (if the 2nd one was created, delete that too)
    vkDestroyPipeline(baseContext.device, renderPassContext.graphicsPipelines[0], nullptr);
    vkDestroyPipelineLayout(baseContext.device,
                            renderPassContext.pipelineLayouts[0], nullptr);

    if(renderPassContext.graphicsPipelines[1] != VK_NULL_HANDLE) {
        vkDestroyPipeline(baseContext.device,
                          renderPassContext.graphicsPipelines[1], nullptr);
        vkDestroyPipelineLayout(baseContext.device,
                                renderPassContext.pipelineLayouts[1], nullptr);
    }

    vkDestroyRenderPass(baseContext.device, renderPassContext.renderPass, nullptr);
}

void cleanupImGuiContext(const VulkanBaseContext& baseContext, RenderContext& renderContext) {
    vkDestroyDescriptorPool(baseContext.device,
                            renderContext.imguiContext.descriptorPool, nullptr);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void createMainRenderPass(const ApplicationVulkanContext& appContext,
                          RenderContext&                  renderContext) {
    VkAttachmentDescription colorAttachment{};

    auto sampleCount = appContext.graphicSettings.useMsaa ?
                           appContext.graphicSettings.msaaSamples :
                           VK_SAMPLE_COUNT_1_BIT;

    // Color Attachment
    createBlankAttachment(appContext, colorAttachment, sampleCount,
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    colorAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Depth Attachment
    VkAttachmentDescription depthAttachment{};
    createBlankAttachment(appContext, depthAttachment, sampleCount,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
    depthAttachment.format = findDepthFormat(appContext.baseContext);
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depthAttachment.storeOp =
        VK_ATTACHMENT_STORE_OP_STORE;  // TODO this will probably need to change for shadow mapping ?

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;


    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::vector<VkAttachmentDescription> attachments({colorAttachment, depthAttachment});

    // TODO: if samples is VK_SAMPLE_COUNT_1_BIT Vulkan
    // expects no resolveAttachment This has not been tested
    // yet -> might be error source Might also be important
    // in createFrameBuffers ? (not sure, might also only
    // save some memory)

    if(appContext.graphicSettings.useMsaa) {
        // when using Msaa, the color attachment is not the attachment presenting
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachmentResolve{};
        createBlankAttachment(appContext, colorAttachmentResolve,
                              VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        subpass.pResolveAttachments = &colorAttachmentResolveRef;

        attachments.push_back(colorAttachmentResolve);
    }

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments    = attachments.data();
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;

    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass      = 0;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    if(renderContext.usesImgui) {
        // @IMGUI
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;  // |
                                                                              // VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;

        // @IMGUI
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;  // | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
                                        | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;  // | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    } else {
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;  // | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask = 0;

        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;  // | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;  // |
                                                                               // VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    dependencies[1].srcSubpass    = 0;
    dependencies[1].dstSubpass    = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].dstStageMask  = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies   = dependencies.data();

    if(vkCreateRenderPass(appContext.baseContext.device, &renderPassInfo, nullptr,
                          &renderContext.renderPasses.mainPass.renderPassContext.renderPass)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

/*
 * Inspired by SashaWillems sample on deferred rendering
 * (https://github.com/SaschaWillems/Vulkan/blob/master/examples/deferred/deferred.cpp)
 * and adjusted to fit our needs.
 */
void createGeometryRenderPass(const ApplicationVulkanContext& appContext,
                              RenderContext&                  renderContext) {
    // create attachments
    // World Space Position
    createDeferredAttachment(appContext, VK_FORMAT_R16G16B16A16_SFLOAT,
                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                             renderContext.renderPasses.mainPass.positionAttachment);
    // World Space Normals
    createDeferredAttachment(appContext, VK_FORMAT_R16G16B16A16_SFLOAT,
                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                             renderContext.renderPasses.mainPass.normalAttachment);
    // Albedo
    createDeferredAttachment(appContext, VK_FORMAT_R8G8B8A8_UNORM,
                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                             renderContext.renderPasses.mainPass.albedoAttachment);
    // AO - Roughness - Metallic
    createDeferredAttachment(appContext, VK_FORMAT_R8G8B8A8_UNORM,
                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                             renderContext.renderPasses.mainPass.aoRoughnessMetallicAttachment);
    // Depth
    VkFormat depthFormat = findDepthFormat(appContext.baseContext);
    createDeferredAttachment(appContext, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                             renderContext.renderPasses.mainPass.depthAttachment);

    // set up separate renderpass with references to the color and depth attachments
    std::array<VkAttachmentDescription, 5> attachmentDescs = {};

    // init attachment properties
    for(uint32_t i = 0; i < 5; ++i) {
        attachmentDescs[i].samples        = VK_SAMPLE_COUNT_1_BIT;
        attachmentDescs[i].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDescs[i].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescs[i].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        // need special case for depth attachment
        if(i == 4) {
            attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            //attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        } else {
            attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    }

    // formats
    attachmentDescs[0].format =
        renderContext.renderPasses.mainPass.positionAttachment.imageFormat;
    attachmentDescs[1].format =
        renderContext.renderPasses.mainPass.normalAttachment.imageFormat;
    attachmentDescs[2].format =
        renderContext.renderPasses.mainPass.albedoAttachment.imageFormat;
    attachmentDescs[3].format =
        renderContext.renderPasses.mainPass.aoRoughnessMetallicAttachment.imageFormat;
    attachmentDescs[4].format =
        renderContext.renderPasses.mainPass.depthAttachment.imageFormat;

    // attachment references
    std::vector<VkAttachmentReference> colorReferences;
    colorReferences.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    colorReferences.push_back({1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    colorReferences.push_back({2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    colorReferences.push_back({3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

    VkAttachmentReference depthReference = {};
    depthReference.attachment            = 4;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pColorAttachments    = colorReferences.data();
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
    subpass.pDepthStencilAttachment = &depthReference;

    // TODO: need to integrate ImGUI somewhere here
    // use subpass dependencies for attachment layout transitions
    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass   = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass   = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
                                    | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
                                    | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // create render pass
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.pAttachments = attachmentDescs.data();
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
    renderPassCreateInfo.subpassCount    = 1;
    renderPassCreateInfo.pSubpasses      = &subpass;
    renderPassCreateInfo.dependencyCount = 2;
    renderPassCreateInfo.pDependencies   = dependencies.data();
    if(vkCreateRenderPass(appContext.baseContext.device, &renderPassCreateInfo,
                          nullptr, &renderContext.renderPasses.mainPass.geometryPass)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }

    // create framebuffer (gBuffer)
    std::array<VkImageView, 5> attachments;
    attachments[0] = renderContext.renderPasses.mainPass.positionAttachment.imageView;
    attachments[1] = renderContext.renderPasses.mainPass.normalAttachment.imageView;
    attachments[2] = renderContext.renderPasses.mainPass.albedoAttachment.imageView;
    attachments[3] =
        renderContext.renderPasses.mainPass.aoRoughnessMetallicAttachment.imageView;
    attachments[4] = renderContext.renderPasses.mainPass.depthAttachment.imageView;

    VkFramebufferCreateInfo framebufferCreateInfo = {};
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.pNext = NULL;
    framebufferCreateInfo.renderPass = renderContext.renderPasses.mainPass.geometryPass;
    framebufferCreateInfo.pAttachments = attachments.data();
    framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferCreateInfo.width = appContext.swapchainContext.swapChainExtent.width;
    framebufferCreateInfo.height = appContext.swapchainContext.swapChainExtent.height;
    framebufferCreateInfo.layers = 1;
    vkCreateFramebuffer(appContext.baseContext.device, &framebufferCreateInfo,
                        nullptr, &renderContext.renderPasses.mainPass.gBuffer);

    // create sampler to sample from the color attachments
    VkSamplerCreateInfo samplerCreateInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerCreateInfo.magFilter     = VK_FILTER_NEAREST;
    samplerCreateInfo.minFilter     = VK_FILTER_NEAREST;
    samplerCreateInfo.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeV  = samplerCreateInfo.addressModeU;
    samplerCreateInfo.addressModeW  = samplerCreateInfo.addressModeU;
    samplerCreateInfo.mipLodBias    = 0.0f;
    samplerCreateInfo.maxAnisotropy = 1.0f;
    samplerCreateInfo.minLod        = 0.0f;
    samplerCreateInfo.maxLod        = 1.0f;
    samplerCreateInfo.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    vkCreateSampler(appContext.baseContext.device, &samplerCreateInfo, nullptr,
                    &renderContext.renderPasses.mainPass.framebufferAttachmentSampler);
}

void initializeShadowPass(const ApplicationVulkanContext& appContext,
                          RenderContext&                  renderContext,
                          const RenderPassDescription& renderPassDescription) {

    ShadowPass& shadowPass = renderContext.renderPasses.shadowPass;

    std::vector<VkDescriptorSetLayoutBinding> bindings;

    bindings.push_back(createLayoutBinding(SceneBindings::eCamera, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                           getStageFlag(ShaderStage::VERTEX_SHADER)));

    createDescriptorSetLayout(appContext.baseContext,
                              shadowPass.transformDescriptorSetLayout, bindings);

    auto& shadowPassLayouts = shadowPass.renderPassContext.descriptorSetLayouts;
    shadowPassLayouts.push_back(shadowPass.transformDescriptorSetLayout);

    // Shadow Depth Buffer Attachment
    VkAttachmentDescription depthAttachment{};
    createBlankAttachment(appContext, depthAttachment, VK_SAMPLE_COUNT_1_BIT,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

    depthAttachment.format  = findDepthFormat(appContext.baseContext);
    depthAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 0;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 0;
    subpass.pColorAttachments       = VK_NULL_HANDLE;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments    = &depthAttachment;
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;

    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass    = 0;
    dependencies[0].srcStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass   = 0;
    dependencies[1].dstSubpass   = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies   = dependencies.data();

    if(vkCreateRenderPass(appContext.baseContext.device, &renderPassInfo,
                          nullptr, &shadowPass.renderPassContext.renderPass)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

    descriptorSetLayouts.push_back(shadowPass.transformDescriptorSetLayout);


    createGraphicsPipeline(appContext, shadowPass.renderPassContext,
                           shadowPass.renderPassContext.pipelineLayouts[0],
                           shadowPass.renderPassContext.graphicsPipelines[0], renderPassDescription,
                           shadowPass.renderPassContext.descriptorSetLayouts, false);

    shadowPass.renderPassContext.renderPassDescription = renderPassDescription;


    const uint32_t SHADOW_MAP_WIDTH  = 1920;
    const uint32_t SHADOW_MAP_HEIGHT = 1920;

    shadowPass.shadowMapWidth  = SHADOW_MAP_WIDTH;
    shadowPass.shadowMapHeight = SHADOW_MAP_HEIGHT;

    VkFormat depthFormat = findDepthFormat(appContext.baseContext);

    for(size_t i = 0; i < MAX_CASCADES; i++) {
        ImageResources& currentDepthImage = shadowPass.depthImages[i];

        createImage(appContext.baseContext, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, 1,
                    1, VK_SAMPLE_COUNT_1_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    currentDepthImage.image, currentDepthImage.memory);

        currentDepthImage.imageView =
            createImageView(appContext.baseContext, currentDepthImage.image,
                            depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

        initializeShadowDepthBuffer(appContext, shadowPass, SHADOW_MAP_WIDTH,
                                    SHADOW_MAP_HEIGHT, i);
    }
}

void initializeShadowDepthBuffer(const ApplicationVulkanContext& appContext,
                                 ShadowPass&                     shadowPass,
                                 uint32_t                        width,
                                 uint32_t                        height,
                                 uint32_t                        index) {
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = shadowPass.renderPassContext.renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments    = &shadowPass.depthImages[index].imageView;
    framebufferInfo.width           = width;
    framebufferInfo.height          = height;
    framebufferInfo.layers          = 1;

    if(vkCreateFramebuffer(appContext.baseContext.device, &framebufferInfo,
                           nullptr, &shadowPass.depthFrameBuffers[index])
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create framebuffer!");
    }
}

void createGraphicsPipeline(const ApplicationVulkanContext& appContext,
                            RenderPassContext&              renderPass,
                            VkPipelineLayout&               pipelineLayout,
                            VkPipeline&                     graphicsPipeline,
                            const RenderPassDescription& renderPassDescription,
                            std::vector<VkDescriptorSetLayout>& layouts,
                            bool useFragmentShader) {

    VkShaderModule vertShaderModule =
        createShaderModule(appContext.baseContext, renderPassDescription.vertexShader);
    VkShaderModule fragShaderModule =
        createShaderModule(appContext.baseContext, renderPassDescription.fragmentShader);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName  = "main";

    // Can set compile time constants here
    // -> Let compiler optimize
    // vertShaderStageInfo.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName  = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    VkPipelineShaderStageCreateInfo shaderStagesNoFrag[] = {vertShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription    = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions   = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.depthBiasEnable  = VK_FALSE;
    if(renderPassDescription.enableDepthBias) {
        rasterizer.depthBiasEnable = VK_TRUE;
    }


    rasterizer.rasterizerDiscardEnable = VK_FALSE;

    // Enable Wireframe rendering here, requires GPU feature to be enabled
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

    rasterizer.lineWidth = 1.0f;
    // rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.cullMode  = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable       = VK_TRUE;
    depthStencil.depthWriteEnable      = VK_TRUE;
    depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable     = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_TRUE;
    multisampling.minSampleShading     = .2f;
    multisampling.rasterizationSamples = appContext.graphicSettings.useMsaa ?
                                             appContext.graphicSettings.msaaSamples :
                                             VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // colorBlendAttachment.blendEnable = VK_FALSE;

    colorBlendAttachment.blendEnable         = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;


    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable     = VK_FALSE;
    colorBlending.logicOp           = VK_LOGIC_OP_COPY;  // Optional
    colorBlending.attachmentCount   = 1;
    colorBlending.pAttachments      = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;  // Optional
    colorBlending.blendConstants[1] = 0.0f;  // Optional
    colorBlending.blendConstants[2] = 0.0f;  // Optional
    colorBlending.blendConstants[3] = 0.0f;  // Optional

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                 VK_DYNAMIC_STATE_SCISSOR};

    if(renderPassDescription.enableDepthBias) {
        dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
    }

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = layouts.size();
    pipelineLayoutInfo.pSetLayouts    = layouts.data();

    pipelineLayoutInfo.pushConstantRangeCount =
        renderPassDescription.pushConstantRanges.size();
    pipelineLayoutInfo.pPushConstantRanges =
        renderPassDescription.pushConstantRanges.data();

    if(vkCreatePipelineLayout(appContext.baseContext.device,
                              &pipelineLayoutInfo, nullptr, &pipelineLayout)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    if(useFragmentShader) {
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages    = shaderStages;
    } else {
        pipelineInfo.stageCount = 1;
        pipelineInfo.pStages    = shaderStagesNoFrag;
    }

    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = nullptr;  // Optional
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.pDepthStencilState  = &depthStencil;

    pipelineInfo.layout = pipelineLayout;

    pipelineInfo.renderPass = renderPass.renderPass;
    pipelineInfo.subpass    = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;  // Optional
    pipelineInfo.basePipelineIndex  = -1;              // Optional

    if(vkCreateGraphicsPipelines(appContext.baseContext.device, VK_NULL_HANDLE,
                                 1, &pipelineInfo, nullptr, &graphicsPipeline)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(appContext.baseContext.device, fragShaderModule, nullptr);
    vkDestroyShaderModule(appContext.baseContext.device, vertShaderModule, nullptr);
}

void createFrameBuffers(ApplicationVulkanContext& appContext, RenderContext& renderContext) {
    if(appContext.swapchainContext.swapChainFramebuffers.size()
       != appContext.swapchainContext.swapChainImageViews.size()) {
        throw std::runtime_error("failed to create framebuffers, need one to one matching of framebuffers to swapchain images");
    }

    for(size_t i = 0; i < appContext.swapchainContext.swapChainImageViews.size(); i++) {

        std::vector<VkImageView> attachments;

        if(appContext.graphicSettings.useMsaa) {
            attachments.push_back(appContext.swapchainContext.colorImage.imageView);
            //attachments.push_back(appContext.swapchainContext.depthImage.imageView);
            attachments.push_back(
                renderContext.renderPasses.mainPass.depthAttachment.imageView);
            attachments.push_back(appContext.swapchainContext.swapChainImageViews[i]);
        } else {
            attachments.push_back(appContext.swapchainContext.swapChainImageViews[i]);
            // attachments.push_back(appContext.swapchainContext.depthImage.imageView);
            attachments.push_back(
                renderContext.renderPasses.mainPass.depthAttachment.imageView);
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass =
            renderContext.renderPasses.mainPass.renderPassContext.renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = appContext.swapchainContext.swapChainExtent.width;
        framebufferInfo.height = appContext.swapchainContext.swapChainExtent.height;
        framebufferInfo.layers = 1;

        if(vkCreateFramebuffer(appContext.baseContext.device, &framebufferInfo, nullptr,
                               &appContext.swapchainContext.swapChainFramebuffers[i])
           != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

// Builds a graphics pipeline and stores it in the secondary slot
void buildSecondaryGraphicsPipeline(const ApplicationVulkanContext& appContext,
                                    RenderPassContext&              renderPass,
                                    bool useFragShader) {
    compileShader(renderPass.renderPassDescription.vertexShader,
                  appContext.baseContext.maxSupportedMinorVersion);
    compileShader(renderPass.renderPassDescription.fragmentShader,
                  appContext.baseContext.maxSupportedMinorVersion);
    if(renderPass.graphicsPipelines[!renderPass.activePipelineIndex] != VK_NULL_HANDLE) {
        vkDestroyPipeline(appContext.baseContext.device,
                          renderPass.graphicsPipelines[!renderPass.activePipelineIndex],
                          nullptr);
        vkDestroyPipelineLayout(appContext.baseContext.device,
                                renderPass.pipelineLayouts[!renderPass.activePipelineIndex],
                                nullptr);
    }

    createGraphicsPipeline(appContext, renderPass,
                           renderPass.pipelineLayouts[!renderPass.activePipelineIndex],
                           renderPass.graphicsPipelines[!renderPass.activePipelineIndex],
                           renderPass.renderPassDescription,
                           renderPass.descriptorSetLayouts, useFragShader);
}

// Swaps to the secondary graphics pipeline if it is valid
bool swapGraphicsPipeline(const ApplicationVulkanContext& appContext,
                          RenderPassContext&              renderPass) {
    if(renderPass.graphicsPipelines[!renderPass.activePipelineIndex] != VK_NULL_HANDLE) {
        renderPass.activePipelineIndex = !renderPass.activePipelineIndex;
        return true;
    } else {
        return false;
    }
}
VkDescriptorSetLayoutBinding createLayoutBinding(uint32_t binding,
                                                 uint32_t descriptorCount,
                                                 VkDescriptorType descriptorType,
                                                 VkShaderStageFlags shaderStageFlags) {
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding         = binding;
    layoutBinding.descriptorType  = descriptorType;
    layoutBinding.descriptorCount = descriptorCount;

    layoutBinding.stageFlags = shaderStageFlags;

    return layoutBinding;
}
VkDescriptorSetLayoutBinding createMaterialsBufferLayoutBinding(uint32_t binding,
                                                                uint32_t descriptorCount,
                                                                VkShaderStageFlags shaderStageFlags) {
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding         = binding;
    layoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = descriptorCount;

    layoutBinding.stageFlags = shaderStageFlags;

    return layoutBinding;
}

VkPushConstantRange createPushConstantRange(uint32_t offset, uint32_t size, VkShaderStageFlags flags) {
    VkPushConstantRange pushConstantRange;

    pushConstantRange.offset = offset;
    pushConstantRange.size   = size;

    pushConstantRange.stageFlags = flags;

    return pushConstantRange;
}
// @IMGUI
// Heavily inspired by "https://github.com/ocornut/imgui/blob/master/examples/example_glfw_vulkan/main.cpp"
void initializeImGui(const ApplicationVulkanContext& appContext, RenderContext& renderContext) {
    // Create ImGui Context and set style
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();

    // Create Descriptor Pool for ImGui
    VkDescriptorPoolSize poolSizes[] = {
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
                           &renderContext.imguiContext.descriptorPool);

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
    initInfo.DescriptorPool = renderContext.imguiContext.descriptorPool;
    // TODO: check for interference here
    initInfo.Subpass   = 0;
    initInfo.Allocator = VK_NULL_HANDLE;
    // TODO: find out if these are the correct values
    initInfo.MinImageCount = 2;
    initInfo.ImageCount    = imageCount;
    // TODO: could implement to check for errors
    initInfo.CheckVkResultFn = nullptr;

    if(appContext.graphicSettings.useMsaa) {
        initInfo.MSAASamples = appContext.graphicSettings.msaaSamples;
    }

    ImGui_ImplVulkan_Init(&initInfo,
                          renderContext.renderPasses.mainPass.renderPassContext.renderPass);

    // Upload fonts to GPU
    VkCommandBuffer commandBuffer =
        beginSingleTimeCommands(appContext.baseContext, appContext.commandContext);
    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
    endSingleTimeCommands(appContext.baseContext, appContext.commandContext, commandBuffer);
}
void createBlankAttachment(const ApplicationVulkanContext& context,
                           VkAttachmentDescription&        attachment,
                           VkSampleCountFlagBits           sampleCount,
                           VkImageLayout                   initialLayout,
                           VkImageLayout                   finalLayout) {
    attachment.format  = context.swapchainContext.swapChainImageFormat;
    attachment.samples = sampleCount;

    attachment.loadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    attachment.initialLayout = initialLayout;
    attachment.finalLayout   = finalLayout;
}

/*
* Used to create framebuffer attachments for deferred rendering. Code inspired by SashaWillems sample.
* (https://github.com/SaschaWillems/Vulkan/blob/master/examples/deferred/deferred.cpp)
*/
void createDeferredAttachment(const ApplicationVulkanContext& context,
                              VkFormat                        format,
                              VkImageUsageFlagBits            usage,
                              ImageResources&                 imageResources) {
    VkImageAspectFlags aspectMask = 0;
    VkImageLayout      imageLayout;

    imageResources.imageFormat = format;

    // need extra stuff for the depth layout
    if(usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
        aspectMask  = VK_IMAGE_ASPECT_COLOR_BIT;
        imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    if(usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if(format >= VK_FORMAT_D16_UNORM_S8_UINT)
            aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    assert(aspectMask > 0);

    VkImageCreateInfo imageCreateInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format    = format;
    imageCreateInfo.extent.width = context.swapchainContext.swapChainExtent.width;
    imageCreateInfo.extent.height = context.swapchainContext.swapChainExtent.height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels    = 1;
    imageCreateInfo.arrayLayers  = 1;
    imageCreateInfo.samples      = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling       = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage        = usage | VK_IMAGE_USAGE_SAMPLED_BIT;

    VkMemoryAllocateInfo memAllocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    VkMemoryRequirements memReqs;

    vkCreateImage(context.baseContext.device, &imageCreateInfo, nullptr,
                  &imageResources.image);
    vkGetImageMemoryRequirements(context.baseContext.device, imageResources.image, &memReqs);
    memAllocInfo.allocationSize = memReqs.size;

    memAllocInfo.memoryTypeIndex =
        findMemoryType(context.baseContext, memReqs.memoryTypeBits,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(context.baseContext.device, &memAllocInfo, nullptr,
                     &imageResources.memory);
    vkBindImageMemory(context.baseContext.device, imageResources.image,
                      imageResources.memory, 0);

    VkImageViewCreateInfo imageViewCreateInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    imageViewCreateInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format                          = format;
    imageViewCreateInfo.subresourceRange                = {};
    imageViewCreateInfo.subresourceRange.aspectMask     = aspectMask;
    imageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
    imageViewCreateInfo.subresourceRange.levelCount     = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount     = 1;
    imageViewCreateInfo.image                           = imageResources.image;
    vkCreateImageView(context.baseContext.device, &imageViewCreateInfo, nullptr,
                      &imageResources.imageView);
}

void createDescriptorPool(const VulkanBaseContext& baseContext, RenderContext& renderContext) {
    // TODO try to find a better way to do descriptorPools

    std::vector<VkDescriptorPoolSize> poolSizes;

    uint32_t maxSets = 0;

    uint32_t shadowTransformCount = 1;
    maxSets += shadowTransformCount;

    VkDescriptorPoolSize shadowTransformPoolSize;
    shadowTransformPoolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    shadowTransformPoolSize.descriptorCount = shadowTransformCount;
    poolSizes.push_back(shadowTransformPoolSize);

    uint32_t mainTransformCount = 2;
    maxSets += mainTransformCount;

    VkDescriptorPoolSize mainTransformPoolSize;
    mainTransformPoolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    mainTransformPoolSize.descriptorCount = mainTransformCount;
    poolSizes.push_back(mainTransformPoolSize);

    uint32_t mainMaterialCount = 1;
    maxSets += mainMaterialCount;

    VkDescriptorPoolSize mainMaterialPoolSize;
    mainMaterialPoolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    mainMaterialPoolSize.descriptorCount = mainMaterialCount;
    poolSizes.push_back(mainMaterialPoolSize);

    uint32_t mainTextureCount = 100;  // hardcoded max texture limit because I
                                      // don't want to also recreate this after
                                      // texture creation
    maxSets += mainTextureCount;

    VkDescriptorPoolSize mainTexturePoolSize;
    mainTexturePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    mainTexturePoolSize.descriptorCount = mainTextureCount;
    poolSizes.push_back(mainTexturePoolSize);

    uint32_t mainDepthCount = 1;
    maxSets += mainDepthCount;

    VkDescriptorPoolSize mainDepthPoolSize;
    mainDepthPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    mainDepthPoolSize.descriptorCount = mainDepthCount;
    poolSizes.push_back(mainDepthPoolSize);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes    = poolSizes.data();
    poolInfo.maxSets       = maxSets;

    if(vkCreateDescriptorPool(baseContext.device, &poolInfo, nullptr,
                              &renderContext.descriptorPool)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}


void createShadowPassDescriptorSets(const ApplicationVulkanContext& appContext,
                                    RenderContext& renderContext) {
    ShadowPass& shadowPass = renderContext.renderPasses.shadowPass;

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = renderContext.descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &renderContext.renderPasses.shadowPass.transformDescriptorSetLayout;

    if(vkAllocateDescriptorSets(appContext.baseContext.device, &allocInfo,
                                &shadowPass.transformDescriptorSet)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    std::vector<VkWriteDescriptorSet> descriptorWrites;

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = shadowPass.transformBuffer.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range  = MAX_CASCADES * sizeof(glm::mat4);

    VkWriteDescriptorSet descriptorWrite;
    descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.pNext           = nullptr;
    descriptorWrite.dstSet          = shadowPass.transformDescriptorSet;
    descriptorWrite.dstBinding      = SceneBindings::eCamera;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo     = &bufferInfo;

    descriptorWrites.emplace_back(descriptorWrite);

    vkUpdateDescriptorSets(appContext.baseContext.device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
}

void createShadowPassResources(const ApplicationVulkanContext& appContext,
                               RenderContext&                  renderContext) {

    createBufferResources(appContext, MAX_CASCADES * sizeof(glm::mat4),
                          renderContext.renderPasses.shadowPass.transformBuffer);
}

void cleanShadowPass(const VulkanBaseContext& baseContext, const ShadowPass& shadowPass) {
    vkDestroyBuffer(baseContext.device, shadowPass.transformBuffer.buffer, nullptr);
    vkFreeMemory(baseContext.device, shadowPass.transformBuffer.bufferMemory, nullptr);

    cleanupRenderPassContext(baseContext, shadowPass.renderPassContext);

    vkDestroyDescriptorSetLayout(baseContext.device,
                                 shadowPass.transformDescriptorSetLayout, nullptr);

    for(size_t i = 0; i < MAX_CASCADES; i++) {
        ImageResources currentDepthImage = shadowPass.depthImages[i];

        vkDestroyImageView(baseContext.device, currentDepthImage.imageView, nullptr);

        vkDestroyImage(baseContext.device, currentDepthImage.image, nullptr);
        vkFreeMemory(baseContext.device, currentDepthImage.memory, nullptr);

        vkDestroyFramebuffer(baseContext.device, shadowPass.depthFrameBuffers[i], nullptr);
    }
}

void createMainPassResources(const ApplicationVulkanContext& appContext,
                             RenderContext&                  renderContext,
                             Scene&                          scene) {

    createBufferResources(appContext, sizeof(SceneTransform),
                          renderContext.renderPasses.mainPass.transformBuffer);

    createBufferResources(appContext, sizeof(LightingInformation),
                          renderContext.renderPasses.mainPass.lightingBuffer);

    createBufferResources(appContext, MAX_CASCADES * sizeof(SplitDummyStruct),
                          renderContext.renderPasses.mainPass.cascadeSplitsBuffer);

    createMaterialsBuffer(appContext, renderContext, scene);
}

void createDepthSampler(const ApplicationVulkanContext& appContext, MainPass& mainPass) {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType     = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    samplerInfo.anisotropyEnable = VK_FALSE;

    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp     = VK_COMPARE_OP_NEVER;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod     = 0.0f;
    samplerInfo.maxLod     = 0.0f;

    if(vkCreateSampler(appContext.baseContext.device, &samplerInfo, nullptr,
                       &mainPass.depthSampler)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void createMainPassDescriptorSetLayouts(const ApplicationVulkanContext& appContext,
                                        MainPass& mainPass,
                                        Scene&    scene) {
    std::vector<VkDescriptorSetLayoutBinding> transformBindings;
    std::vector<VkDescriptorSetLayoutBinding> materialBindings;
    std::vector<VkDescriptorSetLayoutBinding> depthBindings;
    std::vector<VkDescriptorSetLayoutBinding> gBufferBindings;


    transformBindings.push_back(
        createLayoutBinding(SceneBindings::eCamera, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                            getStageFlag(ShaderStage::VERTEX_SHADER)));

    transformBindings.push_back(
        createLayoutBinding(SceneBindings::eLight, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                            getStageFlag(ShaderStage::VERTEX_SHADER)));

    transformBindings.push_back(
        createLayoutBinding(SceneBindings::eLighting, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                            getStageFlag(ShaderStage::FRAGMENT_SHADER)));

    materialBindings.push_back(createLayoutBinding(
        MaterialsBindings::eMaterials, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        getStageFlag(ShaderStage::VERTEX_SHADER) | getStageFlag(ShaderStage::FRAGMENT_SHADER)));

    materialBindings.push_back(createLayoutBinding(
        MaterialsBindings::eTextures, scene.getSceneData().textures.size(),
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        getStageFlag(ShaderStage::FRAGMENT_SHADER)));

    materialBindings.push_back(
        createLayoutBinding(MaterialsBindings::eSkybox, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            getStageFlag(ShaderStage::FRAGMENT_SHADER)));

    depthBindings.push_back(
        createLayoutBinding(DepthBindings::eShadowDepthBuffer, MAX_CASCADES,
                            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            getStageFlag(ShaderStage::FRAGMENT_SHADER)));

    depthBindings.push_back(
        createLayoutBinding(DepthBindings::eCascadeSplits, 1,
                            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                            getStageFlag(ShaderStage::FRAGMENT_SHADER)));

    depthBindings.push_back(
        createLayoutBinding(DepthBindings::eLightVPs, 1,
                            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                            getStageFlag(ShaderStage::FRAGMENT_SHADER)));

    // samplers for accessing gBuffer
    gBufferBindings.push_back(
        createLayoutBinding(GBufferBindings::ePosition, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            getStageFlag(ShaderStage::FRAGMENT_SHADER)));

    gBufferBindings.push_back(
        createLayoutBinding(GBufferBindings::eNormal, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            getStageFlag(ShaderStage::FRAGMENT_SHADER)));

    gBufferBindings.push_back(
        createLayoutBinding(GBufferBindings::eAlbedo, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            getStageFlag(ShaderStage::FRAGMENT_SHADER)));

    gBufferBindings.push_back(
        createLayoutBinding(GBufferBindings::ePBR, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            getStageFlag(ShaderStage::FRAGMENT_SHADER)));

    gBufferBindings.push_back(
        createLayoutBinding(GBufferBindings::eDepth, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            getStageFlag(ShaderStage::FRAGMENT_SHADER)));

    createDescriptorSetLayout(appContext.baseContext,
                              mainPass.transformDescriptorSetLayout, transformBindings);

    createDescriptorSetLayout(appContext.baseContext,
                              mainPass.materialDescriptorSetLayout, materialBindings);

    createDescriptorSetLayout(appContext.baseContext,
                              mainPass.depthDescriptorSetLayout, depthBindings);

    createDescriptorSetLayout(appContext.baseContext,
                              mainPass.gBufferDescriptorSetLayout, gBufferBindings);

    auto& mainPassLayouts = mainPass.renderPassContext.descriptorSetLayouts;
    mainPassLayouts.clear();

    mainPassLayouts.push_back(mainPass.transformDescriptorSetLayout);
    mainPassLayouts.push_back(mainPass.materialDescriptorSetLayout);
    mainPassLayouts.push_back(mainPass.depthDescriptorSetLayout);
    mainPassLayouts.push_back(mainPass.gBufferDescriptorSetLayout);
}

void cleanMainPassDescriptorLayouts(const VulkanBaseContext& baseContext,
                                    const MainPass&          mainPass) {
    vkDestroyDescriptorSetLayout(baseContext.device,
                                 mainPass.transformDescriptorSetLayout, nullptr);

    vkDestroyDescriptorSetLayout(baseContext.device,
                                 mainPass.materialDescriptorSetLayout, nullptr);

    vkDestroyDescriptorSetLayout(baseContext.device,
                                 mainPass.depthDescriptorSetLayout, nullptr);

    vkDestroyDescriptorSetLayout(baseContext.device,
                                 mainPass.gBufferDescriptorSetLayout, nullptr);
}

void createMainPassDescriptorSets(const ApplicationVulkanContext& appContext,
                                  RenderContext&                  renderContext,
                                  Scene&                          scene) {

    MainPass& mainPass = renderContext.renderPasses.mainPass;

    VkDescriptorSetAllocateInfo transformAllocInfo{};
    transformAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    transformAllocInfo.descriptorPool     = renderContext.descriptorPool;
    transformAllocInfo.descriptorSetCount = 1;
    transformAllocInfo.pSetLayouts = &mainPass.transformDescriptorSetLayout;

    if(vkAllocateDescriptorSets(appContext.baseContext.device, &transformAllocInfo,
                                &mainPass.transformDescriptorSet)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    std::vector<VkWriteDescriptorSet> descriptorWrites;


    VkDescriptorBufferInfo transformBufferInfo{};
    transformBufferInfo.buffer = mainPass.transformBuffer.buffer;
    transformBufferInfo.offset = 0;
    transformBufferInfo.range  = VK_WHOLE_SIZE;

    VkWriteDescriptorSet transformDescriptorWrite;
    transformDescriptorWrite.sType  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    transformDescriptorWrite.pNext  = nullptr;
    transformDescriptorWrite.dstSet = mainPass.transformDescriptorSet;
    transformDescriptorWrite.dstBinding      = SceneBindings::eCamera;
    transformDescriptorWrite.dstArrayElement = 0;
    transformDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    transformDescriptorWrite.descriptorCount = 1;
    transformDescriptorWrite.pBufferInfo     = &transformBufferInfo;

    descriptorWrites.emplace_back(transformDescriptorWrite);


    VkDescriptorBufferInfo lightTransformBufferInfo{};
    lightTransformBufferInfo.buffer =
        renderContext.renderPasses.shadowPass.transformBuffer.buffer;
    lightTransformBufferInfo.offset = 0;
    lightTransformBufferInfo.range  = VK_WHOLE_SIZE;

    VkWriteDescriptorSet lightTransformDescriptorWrite;
    lightTransformDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    lightTransformDescriptorWrite.pNext      = nullptr;
    lightTransformDescriptorWrite.dstSet     = mainPass.transformDescriptorSet;
    lightTransformDescriptorWrite.dstBinding = SceneBindings::eLight;
    lightTransformDescriptorWrite.dstArrayElement = 0;
    lightTransformDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightTransformDescriptorWrite.descriptorCount = 1;
    lightTransformDescriptorWrite.pBufferInfo     = &lightTransformBufferInfo;

    descriptorWrites.emplace_back(lightTransformDescriptorWrite);

    VkDescriptorBufferInfo lightingInformationBufferInfo{};
    lightingInformationBufferInfo.buffer = mainPass.lightingBuffer.buffer;
    lightingInformationBufferInfo.offset = 0;
    lightingInformationBufferInfo.range  = VK_WHOLE_SIZE;

    VkWriteDescriptorSet lightingInformationDescriptorWrite;
    lightingInformationDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    lightingInformationDescriptorWrite.pNext  = nullptr;
    lightingInformationDescriptorWrite.dstSet = mainPass.transformDescriptorSet;
    lightingInformationDescriptorWrite.dstBinding = SceneBindings::eLighting;
    lightingInformationDescriptorWrite.dstArrayElement = 0;
    lightingInformationDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightingInformationDescriptorWrite.descriptorCount = 1;
    lightingInformationDescriptorWrite.pBufferInfo = &lightingInformationBufferInfo;

    descriptorWrites.emplace_back(lightingInformationDescriptorWrite);


    VkDescriptorSetAllocateInfo materialAllocInfo{};
    materialAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    materialAllocInfo.descriptorPool     = renderContext.descriptorPool;
    materialAllocInfo.descriptorSetCount = 1;
    materialAllocInfo.pSetLayouts = &mainPass.materialDescriptorSetLayout;

    VkResult materialAllocateRes =
        vkAllocateDescriptorSets(appContext.baseContext.device, &materialAllocInfo,
                                 &mainPass.materialDescriptorSet);

    if(materialAllocateRes != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    VkDescriptorBufferInfo materialBufferInfo{};
    materialBufferInfo.buffer = mainPass.materialBuffer.buffer;
    materialBufferInfo.offset = 0;
    materialBufferInfo.range  = VK_WHOLE_SIZE;

    VkWriteDescriptorSet materialDescriptorWrite;
    materialDescriptorWrite.sType      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    materialDescriptorWrite.pNext      = nullptr;
    materialDescriptorWrite.dstSet     = mainPass.materialDescriptorSet;
    materialDescriptorWrite.dstBinding = MaterialsBindings::eMaterials;
    materialDescriptorWrite.dstArrayElement = 0;
    materialDescriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    materialDescriptorWrite.descriptorCount = 1;
    materialDescriptorWrite.pBufferInfo     = &materialBufferInfo;

    descriptorWrites.emplace_back(materialDescriptorWrite);

    std::vector<VkDescriptorImageInfo> textureSamplers;
    for(auto& texture : scene.getSceneData().textures) {
        textureSamplers.emplace_back(texture.descriptorInfo);
    }

    VkWriteDescriptorSet texturesWrite{};
    texturesWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    texturesWrite.dstSet          = mainPass.materialDescriptorSet;
    texturesWrite.dstBinding      = MaterialsBindings::eTextures;
    texturesWrite.dstArrayElement = 0;
    texturesWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    texturesWrite.descriptorCount = textureSamplers.size();
    texturesWrite.pImageInfo      = textureSamplers.data();

    descriptorWrites.emplace_back(texturesWrite);

    // Sykbox
    VkWriteDescriptorSet skyboxWrite{};
    skyboxWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    skyboxWrite.dstSet          = mainPass.materialDescriptorSet;
    skyboxWrite.dstBinding      = MaterialsBindings::eSkybox;
    skyboxWrite.dstArrayElement = 0;
    skyboxWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    skyboxWrite.descriptorCount = 1;
    skyboxWrite.pImageInfo      = &scene.getSceneData().cubemap.descriptorInfo;

    descriptorWrites.emplace_back(skyboxWrite);

    // create gBuffer descriptor set
    VkDescriptorSetAllocateInfo gBufferAllocInfo{};
    gBufferAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    gBufferAllocInfo.descriptorPool     = renderContext.descriptorPool;
    gBufferAllocInfo.descriptorSetCount = 1;
    gBufferAllocInfo.pSetLayouts        = &mainPass.gBufferDescriptorSetLayout;

    VkResult gBufferAllocateRes =
        vkAllocateDescriptorSets(appContext.baseContext.device, &gBufferAllocInfo,
                                 &mainPass.gBufferDescriptorSet);

    if(gBufferAllocateRes != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
    // fill content of gBuffer descriptor set
    updateGBufferDescriptor(appContext, renderContext);

    VkDescriptorSetAllocateInfo depthAllocInfo{};
    depthAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    depthAllocInfo.descriptorPool     = renderContext.descriptorPool;
    depthAllocInfo.descriptorSetCount = 1;
    depthAllocInfo.pSetLayouts        = &mainPass.depthDescriptorSetLayout;

    auto depthAllocateRes =
        vkAllocateDescriptorSets(appContext.baseContext.device, &depthAllocInfo,
                                 &mainPass.depthDescriptorSet);

    if(depthAllocateRes != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    std::array<VkDescriptorImageInfo, MAX_CASCADES> imageInfos{};

    for(size_t i = 0; i < MAX_CASCADES; i++) {
        VkDescriptorImageInfo imageInfo;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        imageInfo.imageView =
            renderContext.renderPasses.shadowPass.depthImages[i].imageView;
        imageInfo.sampler = renderContext.renderPasses.mainPass.depthSampler;

        imageInfos[i] = imageInfo;
    }

    VkWriteDescriptorSet depthDescriptorWrite;
    depthDescriptorWrite.sType      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    depthDescriptorWrite.pNext      = nullptr;
    depthDescriptorWrite.dstSet     = mainPass.depthDescriptorSet;
    depthDescriptorWrite.dstBinding = DepthBindings::eShadowDepthBuffer;
    depthDescriptorWrite.dstArrayElement = 0;
    depthDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    depthDescriptorWrite.descriptorCount = MAX_CASCADES;
    depthDescriptorWrite.pImageInfo      = imageInfos.data();

    descriptorWrites.emplace_back(depthDescriptorWrite);


    VkDescriptorBufferInfo cascadeSplitBufferInfo{};
    cascadeSplitBufferInfo.buffer = mainPass.cascadeSplitsBuffer.buffer;
    cascadeSplitBufferInfo.offset = 0;
    cascadeSplitBufferInfo.range  = VK_WHOLE_SIZE;

    VkWriteDescriptorSet depthCascadeSplitsWrite;
    depthCascadeSplitsWrite.sType  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    depthCascadeSplitsWrite.pNext  = nullptr;
    depthCascadeSplitsWrite.dstSet = mainPass.depthDescriptorSet;
    depthCascadeSplitsWrite.dstBinding      = DepthBindings::eCascadeSplits;
    depthCascadeSplitsWrite.dstArrayElement = 0;
    depthCascadeSplitsWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    depthCascadeSplitsWrite.descriptorCount = 1;
    depthCascadeSplitsWrite.pBufferInfo     = &cascadeSplitBufferInfo;

    descriptorWrites.emplace_back(depthCascadeSplitsWrite);


    VkDescriptorBufferInfo inverseLightVPBufferInfo{};
    inverseLightVPBufferInfo.buffer = renderContext.renderPasses.shadowPass.transformBuffer.buffer;
    inverseLightVPBufferInfo.offset = 0;
    inverseLightVPBufferInfo.range  = VK_WHOLE_SIZE;

    VkWriteDescriptorSet inverseLightVPWrite;
    inverseLightVPWrite.sType  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    inverseLightVPWrite.pNext  = nullptr;
    inverseLightVPWrite.dstSet = mainPass.depthDescriptorSet;
    inverseLightVPWrite.dstBinding      = DepthBindings::eLightVPs;
    inverseLightVPWrite.dstArrayElement = 0;
    inverseLightVPWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    inverseLightVPWrite.descriptorCount = 1;
    inverseLightVPWrite.pBufferInfo     = &inverseLightVPBufferInfo;

    descriptorWrites.emplace_back(inverseLightVPWrite);


    vkUpdateDescriptorSets(appContext.baseContext.device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
}

/*
 * Needs to be called on resize to assure that the descriptor has the updated image extents.
 */
void updateGBufferDescriptor(const ApplicationVulkanContext& appContext,
                             RenderContext&                  renderContext) {
    MainPass& mainPass = renderContext.renderPasses.mainPass;
    std::vector<VkWriteDescriptorSet> descriptorWrites;

    std::vector<VkDescriptorImageInfo> gBufferDescriptorImageInfos(5);
    for(int i = 0; i < 5; i++) {
        gBufferDescriptorImageInfos[i].sampler = mainPass.framebufferAttachmentSampler;
        gBufferDescriptorImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    gBufferDescriptorImageInfos[0].imageView = mainPass.positionAttachment.imageView;
    gBufferDescriptorImageInfos[1].imageView = mainPass.normalAttachment.imageView;
    gBufferDescriptorImageInfos[2].imageView = mainPass.albedoAttachment.imageView;
    gBufferDescriptorImageInfos[3].imageView =
        mainPass.aoRoughnessMetallicAttachment.imageView;
    gBufferDescriptorImageInfos[4].imageView = mainPass.depthAttachment.imageView;
    // depth needs special treatment
    gBufferDescriptorImageInfos[4].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    // Position
    VkWriteDescriptorSet gBufferPositionWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    gBufferPositionWrite.dstSet          = mainPass.gBufferDescriptorSet;
    gBufferPositionWrite.dstBinding      = GBufferBindings::ePosition;
    gBufferPositionWrite.dstArrayElement = 0;
    gBufferPositionWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    gBufferPositionWrite.descriptorCount = 1;
    gBufferPositionWrite.pImageInfo      = &gBufferDescriptorImageInfos[0];

    descriptorWrites.emplace_back(gBufferPositionWrite);

    // Normal
    VkWriteDescriptorSet gBufferNormalWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    gBufferNormalWrite.dstSet          = mainPass.gBufferDescriptorSet;
    gBufferNormalWrite.dstBinding      = GBufferBindings::eNormal;
    gBufferNormalWrite.dstArrayElement = 0;
    gBufferNormalWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    gBufferNormalWrite.descriptorCount = 1;
    gBufferNormalWrite.pImageInfo      = &gBufferDescriptorImageInfos[1];

    descriptorWrites.emplace_back(gBufferNormalWrite);

    // Albedo
    VkWriteDescriptorSet gBufferAlbedoWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    gBufferAlbedoWrite.dstSet          = mainPass.gBufferDescriptorSet;
    gBufferAlbedoWrite.dstBinding      = GBufferBindings::eAlbedo;
    gBufferAlbedoWrite.dstArrayElement = 0;
    gBufferAlbedoWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    gBufferAlbedoWrite.descriptorCount = 1;
    gBufferAlbedoWrite.pImageInfo      = &gBufferDescriptorImageInfos[2];

    descriptorWrites.emplace_back(gBufferAlbedoWrite);

    // AO Roughness Metallic
    VkWriteDescriptorSet gBufferPBRWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    gBufferPBRWrite.dstSet          = mainPass.gBufferDescriptorSet;
    gBufferPBRWrite.dstBinding      = GBufferBindings::ePBR;
    gBufferPBRWrite.dstArrayElement = 0;
    gBufferPBRWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    gBufferPBRWrite.descriptorCount = 1;
    gBufferPBRWrite.pImageInfo      = &gBufferDescriptorImageInfos[3];

    descriptorWrites.emplace_back(gBufferPBRWrite);

    // Depth
    VkWriteDescriptorSet gBufferDepthWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    gBufferDepthWrite.dstSet          = mainPass.gBufferDescriptorSet;
    gBufferDepthWrite.dstBinding      = GBufferBindings::eDepth;
    gBufferDepthWrite.dstArrayElement = 0;
    gBufferDepthWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    gBufferDepthWrite.descriptorCount = 1;
    gBufferDepthWrite.pImageInfo      = &gBufferDescriptorImageInfos[4];

    descriptorWrites.emplace_back(gBufferDepthWrite);

    vkUpdateDescriptorSets(appContext.baseContext.device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
}


void cleanMainPass(const VulkanBaseContext& baseContext, const MainPass& mainPass) {
    vkDestroySampler(baseContext.device, mainPass.depthSampler, nullptr);

    vkDestroyBuffer(baseContext.device, mainPass.transformBuffer.buffer, nullptr);
    vkFreeMemory(baseContext.device, mainPass.transformBuffer.bufferMemory, nullptr);

    vkDestroyBuffer(baseContext.device, mainPass.lightingBuffer.buffer, nullptr);
    vkFreeMemory(baseContext.device, mainPass.lightingBuffer.bufferMemory, nullptr);

    vkDestroyBuffer(baseContext.device, mainPass.cascadeSplitsBuffer.buffer, nullptr);
    vkFreeMemory(baseContext.device, mainPass.cascadeSplitsBuffer.bufferMemory, nullptr);

    vkDestroyBuffer(baseContext.device, mainPass.materialBuffer.buffer, nullptr);
    vkFreeMemory(baseContext.device, mainPass.materialBuffer.bufferMemory, nullptr);

    // destroy pipelines
    cleanVisualizationPipeline(baseContext, mainPass);

    cleanPrimaryLightingPipeline(baseContext, mainPass);

    cleanSkyboxPipeline(baseContext, mainPass);

    cleanupRenderPassContext(baseContext, mainPass.renderPassContext);

    cleanMainPassDescriptorLayouts(baseContext, mainPass);
}

void cleanDeferredPass(const VulkanBaseContext& baseContext, const MainPass& mainPass) {
    cleanDeferredFramebuffer(baseContext, mainPass);

    // pipelines
    cleanGeometryPassPipeline(baseContext, mainPass);
}

void cleanDeferredFramebuffer(const VulkanBaseContext& baseContext,
                              const MainPass&          mainPass) {
    // attachment sampler
    vkDestroySampler(baseContext.device, mainPass.framebufferAttachmentSampler, nullptr);

    // framebuffer attachments
    vkDestroyImageView(baseContext.device, mainPass.positionAttachment.imageView, nullptr);
    vkDestroyImage(baseContext.device, mainPass.positionAttachment.image, nullptr);
    vkFreeMemory(baseContext.device, mainPass.positionAttachment.memory, nullptr);

    vkDestroyImageView(baseContext.device, mainPass.normalAttachment.imageView, nullptr);
    vkDestroyImage(baseContext.device, mainPass.normalAttachment.image, nullptr);
    vkFreeMemory(baseContext.device, mainPass.normalAttachment.memory, nullptr);

    vkDestroyImageView(baseContext.device, mainPass.albedoAttachment.imageView, nullptr);
    vkDestroyImage(baseContext.device, mainPass.albedoAttachment.image, nullptr);
    vkFreeMemory(baseContext.device, mainPass.albedoAttachment.memory, nullptr);

    vkDestroyImageView(baseContext.device,
                       mainPass.aoRoughnessMetallicAttachment.imageView, nullptr);
    vkDestroyImage(baseContext.device, mainPass.aoRoughnessMetallicAttachment.image, nullptr);
    vkFreeMemory(baseContext.device, mainPass.aoRoughnessMetallicAttachment.memory, nullptr);

    vkDestroyImageView(baseContext.device, mainPass.depthAttachment.imageView, nullptr);
    vkDestroyImage(baseContext.device, mainPass.depthAttachment.image, nullptr);
    vkFreeMemory(baseContext.device, mainPass.depthAttachment.memory, nullptr);

    // framebuffer
    vkDestroyFramebuffer(baseContext.device, mainPass.gBuffer, nullptr);

    // render pass
    vkDestroyRenderPass(baseContext.device, mainPass.geometryPass, nullptr);
}

void createBufferResources(const ApplicationVulkanContext& appContext,
                           VkDeviceSize                    bufferSize,
                           BufferResources&                bufferResources) {
    createBuffer(appContext.baseContext, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 bufferResources.buffer, bufferResources.bufferMemory);

    vkMapMemory(appContext.baseContext.device, bufferResources.bufferMemory, 0,
                bufferSize, 0, &bufferResources.bufferMemoryMapping);
}
/*
 * Uploads all the registered materials to the GPU. Needs to be called before
 * the scene is rendered, but after all needed materials are added to the scene.
 */
void createMaterialsBuffer(const ApplicationVulkanContext& appContext,
                           RenderContext&                  renderContext,
                           Scene&                          scene) {
    VkDeviceSize bufferSize = sizeof(Material) * scene.getSceneData().materials.size();

    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(appContext.baseContext, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(appContext.baseContext.device, stagingBufferMemory, 0, bufferSize, 0, &data);

    // We use Host Coherent Memory to make sure data is synchronized, could also manually flush Memory Ranges
    memcpy(data, scene.getSceneData().materials.data(), (size_t)bufferSize);
    vkUnmapMemory(appContext.baseContext.device, stagingBufferMemory);

    auto& materialsBuffer = renderContext.renderPasses.mainPass.materialBuffer;

    createBuffer(appContext.baseContext, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                     | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, materialsBuffer.buffer,
                 materialsBuffer.bufferMemory);

    copyBuffer(appContext.baseContext, appContext.commandContext, stagingBuffer,
               materialsBuffer.buffer, bufferSize);

    vkDestroyBuffer(appContext.baseContext.device, stagingBuffer, nullptr);
    vkFreeMemory(appContext.baseContext.device, stagingBufferMemory, nullptr);
}


void createVisualizationPipeline(const ApplicationVulkanContext& appContext,
                                 const RenderContext&            renderContext,
                                 MainPass&                       mainPass) {

    Shader vertexShader;
    vertexShader.shaderStage      = ShaderStage::VERTEX_SHADER;
    vertexShader.shaderSourceName = "shadowVis.vert";
    vertexShader.sourceDirectory  = "res/shaders/source/";
    vertexShader.spvDirectory     = "res/shaders/spv/";

    Shader fragmentShader;
    fragmentShader.shaderStage      = ShaderStage::FRAGMENT_SHADER;
    fragmentShader.shaderSourceName = "shadowVis.frag";
    fragmentShader.sourceDirectory  = "res/shaders/source/";
    fragmentShader.spvDirectory     = "res/shaders/spv/";

    VkShaderModule vertShaderModule =
        createShaderModule(appContext.baseContext, vertexShader, true);
    VkShaderModule fragShaderModule =
        createShaderModule(appContext.baseContext, fragmentShader, true);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName  = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName  = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Vertex::getBindingDescription();

    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount   = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions      = VK_NULL_HANDLE;
    vertexInputInfo.pVertexAttributeDescriptions    = VK_NULL_HANDLE;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;

    rasterizer.rasterizerDiscardEnable = VK_FALSE;

    // Enable Wireframe rendering here, requires GPU feature to be enabled
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

    rasterizer.lineWidth = 1.0f;
    // rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.cullMode        = VK_CULL_MODE_NONE;
    rasterizer.frontFace       = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable       = VK_TRUE;
    depthStencil.depthWriteEnable      = VK_TRUE;
    depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable     = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_TRUE;
    multisampling.minSampleShading     = .2f;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;


    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable     = VK_FALSE;
    colorBlending.logicOp           = VK_LOGIC_OP_COPY;  // Optional
    colorBlending.attachmentCount   = 1;
    colorBlending.pAttachments      = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;  // Optional
    colorBlending.blendConstants[1] = 0.0f;  // Optional
    colorBlending.blendConstants[2] = 0.0f;  // Optional
    colorBlending.blendConstants[3] = 0.0f;  // Optional

    std::vector<VkDynamicState>      dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                      VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts    = &mainPass.depthDescriptorSetLayout;

    VkPushConstantRange pushConstantRange;
    pushConstantRange = createPushConstantRange(0, sizeof(ShadowControlPushConstant),
                                                getStageFlag(ShaderStage::FRAGMENT_SHADER));

    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;

    if(vkCreatePipelineLayout(appContext.baseContext.device, &pipelineLayoutInfo,
                              nullptr, &mainPass.visualizePipelineLayout)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages    = shaderStages;

    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = nullptr;  // Optional
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.pDepthStencilState  = &depthStencil;

    pipelineInfo.layout = mainPass.visualizePipelineLayout;

    pipelineInfo.renderPass = mainPass.renderPassContext.renderPass;
    pipelineInfo.subpass    = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;  // Optional
    pipelineInfo.basePipelineIndex  = -1;              // Optional

    if(vkCreateGraphicsPipelines(appContext.baseContext.device, VK_NULL_HANDLE, 1,
                                 &pipelineInfo, nullptr, &mainPass.visualizePipeline)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(appContext.baseContext.device, fragShaderModule, nullptr);
    vkDestroyShaderModule(appContext.baseContext.device, vertShaderModule, nullptr);
}
void cleanVisualizationPipeline(const VulkanBaseContext& baseContext, const MainPass& mainPass) {
    vkDestroyPipeline(baseContext.device, mainPass.visualizePipeline, nullptr);
    vkDestroyPipelineLayout(baseContext.device, mainPass.visualizePipelineLayout, nullptr);
}

void createSkyboxPipeline(const ApplicationVulkanContext& appContext,
                          const RenderContext&            renderContext,
                          MainPass&                       mainPass) {
    Shader vertexShader;
    vertexShader.shaderStage      = ShaderStage::VERTEX_SHADER;
    vertexShader.shaderSourceName = "skybox.vert";
    vertexShader.sourceDirectory  = "res/shaders/source/";
    vertexShader.spvDirectory     = "res/shaders/spv/";

    Shader fragmentShader;
    fragmentShader.shaderStage      = ShaderStage::FRAGMENT_SHADER;
    fragmentShader.shaderSourceName = "skybox.frag";
    fragmentShader.sourceDirectory  = "res/shaders/source/";
    fragmentShader.spvDirectory     = "res/shaders/spv/";

    VkShaderModule vertShaderModule =
        createShaderModule(appContext.baseContext, vertexShader, true);
    VkShaderModule fragShaderModule =
        createShaderModule(appContext.baseContext, fragmentShader, true);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName  = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName  = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Vertex::getBindingDescription();

    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions   = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;

    rasterizer.rasterizerDiscardEnable = VK_FALSE;

    // Enable Wireframe rendering here, requires GPU feature to be enabled
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

    rasterizer.lineWidth = 1.0f;
    // rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.cullMode        = VK_CULL_MODE_NONE;
    rasterizer.frontFace       = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable  = VK_TRUE;
    depthStencil.depthWriteEnable = VK_FALSE;
    // in the shader we set the depth of the screen quad to 1.0, so this has to be lesser-equals
    depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable     = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_TRUE;
    multisampling.minSampleShading     = .2f;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable     = VK_FALSE;
    colorBlending.logicOp           = VK_LOGIC_OP_COPY;  // Optional
    colorBlending.attachmentCount   = 1;
    colorBlending.pAttachments      = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;  // Optional
    colorBlending.blendConstants[1] = 0.0f;  // Optional
    colorBlending.blendConstants[2] = 0.0f;  // Optional
    colorBlending.blendConstants[3] = 0.0f;  // Optional

    std::vector<VkDynamicState>      dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                      VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    descriptorSetLayouts.push_back(mainPass.transformDescriptorSetLayout);
    descriptorSetLayouts.push_back(mainPass.materialDescriptorSetLayout);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
    pipelineLayoutInfo.pSetLayouts    = descriptorSetLayouts.data();

    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges    = VK_NULL_HANDLE;

    if(vkCreatePipelineLayout(appContext.baseContext.device, &pipelineLayoutInfo,
                              nullptr, &mainPass.skyboxPipelineLayout)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages    = shaderStages;

    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = nullptr;  // Optional
    pipelineInfo.pColorBlendState   = &colorBlending;
    pipelineInfo.pDynamicState      = &dynamicState;
    pipelineInfo.pDepthStencilState = &depthStencil;

    pipelineInfo.layout = mainPass.skyboxPipelineLayout;

    pipelineInfo.renderPass = mainPass.renderPassContext.renderPass;
    pipelineInfo.subpass    = mainPass.skyboxSubpassID;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;  // Optional
    pipelineInfo.basePipelineIndex  = -1;              // Optional

    if(vkCreateGraphicsPipelines(appContext.baseContext.device, VK_NULL_HANDLE, 1,
                                 &pipelineInfo, nullptr, &mainPass.skyboxPipeline)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(appContext.baseContext.device, fragShaderModule, nullptr);
    vkDestroyShaderModule(appContext.baseContext.device, vertShaderModule, nullptr);
}

void cleanSkyboxPipeline(const VulkanBaseContext& baseContext, const MainPass& mainPass) {
    vkDestroyPipeline(baseContext.device, mainPass.skyboxPipeline, nullptr);
    vkDestroyPipelineLayout(baseContext.device, mainPass.skyboxPipelineLayout, nullptr);
}

// TODO: adjust this
void createPrimaryLightingPipeline(const ApplicationVulkanContext& appContext,
                                   const RenderContext& renderContext,
                                   MainPass&            mainPass) {
    Shader vertexShader;
    vertexShader.shaderStage      = ShaderStage::VERTEX_SHADER;
    vertexShader.shaderSourceName = "primaryLighting.vert";
    vertexShader.sourceDirectory  = "res/shaders/source/";
    vertexShader.spvDirectory     = "res/shaders/spv/";

    Shader fragmentShader;
    fragmentShader.shaderStage      = ShaderStage::FRAGMENT_SHADER;
    fragmentShader.shaderSourceName = "primaryLighting.frag";
    fragmentShader.sourceDirectory  = "res/shaders/source/";
    fragmentShader.spvDirectory     = "res/shaders/spv/";

    VkShaderModule vertShaderModule =
        createShaderModule(appContext.baseContext, vertexShader, true);
    VkShaderModule fragShaderModule =
        createShaderModule(appContext.baseContext, fragmentShader, true);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName  = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName  = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Vertex::getBindingDescription();

    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions   = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;

    rasterizer.rasterizerDiscardEnable = VK_FALSE;

    // Enable Wireframe rendering here, requires GPU feature to be enabled
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

    rasterizer.lineWidth = 1.0f;
    // rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.cullMode        = VK_CULL_MODE_NONE;
    rasterizer.frontFace       = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable  = VK_TRUE;
    depthStencil.depthWriteEnable = VK_FALSE;
    // in the shader we set the depth of the screen quad to 1.0, so this has to be lesser-equals
    depthStencil.depthCompareOp        = VK_COMPARE_OP_GREATER;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable     = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_TRUE;
    multisampling.minSampleShading     = .2f;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable     = VK_FALSE;
    colorBlending.logicOp           = VK_LOGIC_OP_COPY;  // Optional
    colorBlending.attachmentCount   = 1;
    colorBlending.pAttachments      = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;  // Optional
    colorBlending.blendConstants[1] = 0.0f;  // Optional
    colorBlending.blendConstants[2] = 0.0f;  // Optional
    colorBlending.blendConstants[3] = 0.0f;  // Optional

    std::vector<VkDynamicState>      dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                      VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    descriptorSetLayouts.push_back(mainPass.transformDescriptorSetLayout);
    descriptorSetLayouts.push_back(mainPass.depthDescriptorSetLayout);
    descriptorSetLayouts.push_back(mainPass.gBufferDescriptorSetLayout);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
    pipelineLayoutInfo.pSetLayouts    = descriptorSetLayouts.data();

    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges    = VK_NULL_HANDLE;

    if(vkCreatePipelineLayout(appContext.baseContext.device, &pipelineLayoutInfo,
                              nullptr, &mainPass.primaryLightingPipelineLayout)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages    = shaderStages;

    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = nullptr;  // Optional
    // TODO: see if it still works with this line commented out
    pipelineInfo.pColorBlendState   = &colorBlending;
    pipelineInfo.pDynamicState      = &dynamicState;
    pipelineInfo.pDepthStencilState = &depthStencil;

    pipelineInfo.layout = mainPass.primaryLightingPipelineLayout;

    pipelineInfo.renderPass = mainPass.renderPassContext.renderPass;
    pipelineInfo.subpass    = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;  // Optional
    pipelineInfo.basePipelineIndex  = -1;              // Optional

    if(vkCreateGraphicsPipelines(appContext.baseContext.device, VK_NULL_HANDLE, 1,
                                 &pipelineInfo, nullptr, &mainPass.primaryLightingPipeline)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(appContext.baseContext.device, fragShaderModule, nullptr);
    vkDestroyShaderModule(appContext.baseContext.device, vertShaderModule, nullptr);
}

void cleanPrimaryLightingPipeline(const VulkanBaseContext& baseContext,
                                  const MainPass&          mainPass) {
    vkDestroyPipeline(baseContext.device, mainPass.primaryLightingPipeline, nullptr);
    vkDestroyPipelineLayout(baseContext.device,
                            mainPass.primaryLightingPipelineLayout, nullptr);
}

void createGeometryPassPipeline(const ApplicationVulkanContext& appContext,
                                const RenderContext&            renderContext,
                                const RenderPassDescription& renderPassDescription,
                                MainPass& mainPass) {
    Shader vertexShader;
    vertexShader.shaderStage      = ShaderStage::VERTEX_SHADER;
    vertexShader.shaderSourceName = "geometryPass.vert";
    vertexShader.sourceDirectory  = "res/shaders/source/";
    vertexShader.spvDirectory     = "res/shaders/spv/";

    Shader fragmentShader;
    fragmentShader.shaderStage      = ShaderStage::FRAGMENT_SHADER;
    fragmentShader.shaderSourceName = "geometryPass.frag";
    fragmentShader.sourceDirectory  = "res/shaders/source/";
    fragmentShader.spvDirectory     = "res/shaders/spv/";

    VkShaderModule vertShaderModule =
        createShaderModule(appContext.baseContext, vertexShader, true);
    VkShaderModule fragShaderModule =
        createShaderModule(appContext.baseContext, fragmentShader, true);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName  = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName  = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Vertex::getBindingDescription();

    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions   = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.depthBiasEnable  = VK_FALSE;
    if(renderPassDescription.enableDepthBias) {
        rasterizer.depthBiasEnable = VK_TRUE;
    }
    rasterizer.rasterizerDiscardEnable = VK_FALSE;

    // Enable Wireframe rendering here, requires GPU feature to be enabled
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

    rasterizer.lineWidth       = 1.0f;
    rasterizer.cullMode        = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace       = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable  = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable     = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_TRUE;
    multisampling.minSampleShading     = .2f;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    std::array<VkPipelineColorBlendAttachmentState, 4> colorBlendAttachments{};
    for(int i = 0; i < 4; i++) {
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
        colorBlendAttachments[i] = colorBlendAttachment;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable     = VK_FALSE;
    colorBlending.logicOp           = VK_LOGIC_OP_COPY;  // Optional
    colorBlending.attachmentCount   = colorBlendAttachments.size();
    colorBlending.pAttachments      = colorBlendAttachments.data();
    colorBlending.blendConstants[0] = 0.0f;  // Optional
    colorBlending.blendConstants[1] = 0.0f;  // Optional
    colorBlending.blendConstants[2] = 0.0f;  // Optional
    colorBlending.blendConstants[3] = 0.0f;  // Optional

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                 VK_DYNAMIC_STATE_SCISSOR};
    if(renderPassDescription.enableDepthBias) {
        dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
    }

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount =
        mainPass.renderPassContext.descriptorSetLayouts.size();
    pipelineLayoutInfo.pSetLayouts =
        mainPass.renderPassContext.descriptorSetLayouts.data();

    pipelineLayoutInfo.pushConstantRangeCount =
        renderPassDescription.pushConstantRanges.size();
    pipelineLayoutInfo.pPushConstantRanges =
        renderPassDescription.pushConstantRanges.data();

    if(vkCreatePipelineLayout(appContext.baseContext.device, &pipelineLayoutInfo,
                              nullptr, &mainPass.geometryPassPipelineLayout)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages    = shaderStages;
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = nullptr;  // Optional
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.pDepthStencilState  = &depthStencil;

    pipelineInfo.layout     = mainPass.geometryPassPipelineLayout;
    pipelineInfo.renderPass = mainPass.geometryPass;
    pipelineInfo.subpass    = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;  // Optional
    pipelineInfo.basePipelineIndex  = -1;              // Optional

    if(vkCreateGraphicsPipelines(appContext.baseContext.device, VK_NULL_HANDLE, 1,
                                 &pipelineInfo, nullptr, &mainPass.geometryPassPipeline)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(appContext.baseContext.device, fragShaderModule, nullptr);
    vkDestroyShaderModule(appContext.baseContext.device, vertShaderModule, nullptr);
}

void cleanGeometryPassPipeline(const VulkanBaseContext& baseContext, const MainPass& mainPass) {
    vkDestroyPipeline(baseContext.device, mainPass.geometryPassPipeline, nullptr);
    vkDestroyPipelineLayout(baseContext.device, mainPass.geometryPassPipelineLayout, nullptr);
}