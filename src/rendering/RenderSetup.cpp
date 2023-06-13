#include <stdexcept>
#include <filesystem>
#include "RenderSetup.h"
#include "vulkan/VulkanUtils.h"
#include "utils/FileUtils.h"
#include "rendering/host_device.h"
#include "vulkan/VulkanSetup.h"

void initializeSimpleSceneRenderContext(ApplicationVulkanContext& appContext,
                                        RenderContext& renderContext) {
    auto& settings     = renderContext.renderSettings;
    settings.fov       = glm::radians(45.0f);
    settings.nearPlane = 0.1;
    settings.farPlane  = 1000;

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

    shadowPassDescription.pushConstantRanges.push_back(
        createPushConstantRange(0, sizeof(glm::mat4), ShaderStage::VERTEX_SHADER));

    renderSetupDescription.shadowPassDescription = shadowPassDescription;

    // --- Main Render Pass
    RenderPassDescription mainRenderPassDescription;

    mainRenderPassDescription.vertexShader.shaderStage = ShaderStage::VERTEX_SHADER;
    mainRenderPassDescription.vertexShader.shaderSourceName = "mainPass.vert";
    mainRenderPassDescription.vertexShader.sourceDirectory = "res/shaders/source/";
    mainRenderPassDescription.vertexShader.spvDirectory = "res/shaders/spv/";

    mainRenderPassDescription.fragmentShader.shaderStage = ShaderStage::FRAGMENT_SHADER;
    mainRenderPassDescription.fragmentShader.shaderSourceName = "mainPass.frag";
    mainRenderPassDescription.fragmentShader.sourceDirectory = "res/shaders/source/";
    mainRenderPassDescription.fragmentShader.spvDirectory = "res/shaders/spv/";

    mainRenderPassDescription.pushConstantRanges.push_back(
        createPushConstantRange(0, sizeof(glm::mat4), ShaderStage::VERTEX_SHADER));

    renderSetupDescription.mainRenderPassDescription = mainRenderPassDescription;

    initializeRenderContext(appContext, renderContext, renderSetupDescription);
}

void initializeRenderContext(ApplicationVulkanContext& appContext,
                             RenderContext&            renderContext,
                             const RenderSetupDescription& renderSetupDescription) {

    createDescriptorPool(appContext.baseContext, renderContext);

    // --- Shadow Pass

    initializeShadowPass(appContext, renderContext.renderPasses.shadowPass,
                         renderSetupDescription.shadowPassDescription);

    createShadowPassResources(appContext, renderContext);
    createShadowPassDescriptorSets(appContext, renderContext);

    // --- Main Render Pass
    initializeMainRenderPass(appContext, renderContext,
                             renderSetupDescription.mainRenderPassDescription);


    renderContext.renderSetupDescription = renderSetupDescription;
    createFrameBuffers(appContext, renderContext);

    renderContext.usesImgui = renderSetupDescription.enableImgui;
    if(renderContext.usesImgui) {
        initializeImGui(appContext, renderContext);
    }
}

void initializeMainRenderPass(const ApplicationVulkanContext& appContext,
                              RenderContext&                  renderContext,
                              const RenderPassDescription& renderPassDescription) {

    std::vector<VkDescriptorSetLayoutBinding> transformBindings;
    std::vector<VkDescriptorSetLayoutBinding> materialBindings;
    std::vector<VkDescriptorSetLayoutBinding> depthBindings;

    transformBindings.push_back(
        createLayoutBinding(SceneBindings::eCamera, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                            getStageFlag(ShaderStage::VERTEX_SHADER)));

    materialBindings.push_back(createLayoutBinding(
        MaterialsBindings::eMaterials, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        getStageFlag(ShaderStage::VERTEX_SHADER) | getStageFlag(ShaderStage::FRAGMENT_SHADER)));

    depthBindings.push_back(
        createLayoutBinding(DepthBindings::eShadowDepthBuffer, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            getStageFlag(ShaderStage::FRAGMENT_SHADER)));

    createDescriptorSetLayout(appContext.baseContext,
                              renderContext.renderPasses.mainPass.transformDescriptorSetLayout,
                              transformBindings);

    createDescriptorSetLayout(appContext.baseContext,
                              renderContext.renderPasses.mainPass.materialDescriptorSetLayout,
                              materialBindings);

    createDescriptorSetLayout(appContext.baseContext,
                              renderContext.renderPasses.mainPass.depthDescriptorSetLayout,
                              depthBindings);

    createMainRenderPass(appContext, renderContext);


    auto& mainPassSetLayouts =
        renderContext.renderPasses.mainPass.renderPassContext.descriptorSetLayouts;

    mainPassSetLayouts.push_back(renderContext.renderPasses.mainPass.transformDescriptorSetLayout);
    mainPassSetLayouts.push_back(renderContext.renderPasses.mainPass.materialDescriptorSetLayout);
    mainPassSetLayouts.push_back(renderContext.renderPasses.mainPass.depthDescriptorSetLayout);

    createGraphicsPipeline(
        appContext, renderContext.renderPasses.mainPass.renderPassContext,
        renderContext.renderPasses.mainPass.renderPassContext.pipelineLayouts[0],
        renderContext.renderPasses.mainPass.renderPassContext.graphicsPipelines[0],
        renderPassDescription, mainPassSetLayouts);

    renderContext.renderPasses.mainPass.renderPassContext.renderPassDescription =
        renderPassDescription;

    createDepthSampler(appContext, renderContext.renderPasses.mainPass);
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
    createBlankAttachment(appContext, depthAttachment, sampleCount, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    depthAttachment.format = findDepthFormat(appContext.baseContext);
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp =
        VK_ATTACHMENT_STORE_OP_DONT_CARE;  // TODO this will probably need to change for shadow mapping ?

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


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

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
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

    dependencies[1].srcSubpass   = 0;
    dependencies[1].dstSubpass   = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dstAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies   = dependencies.data();

    if(vkCreateRenderPass(appContext.baseContext.device, &renderPassInfo, nullptr,
                          &renderContext.renderPasses.mainPass.renderPassContext.renderPass)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void initializeShadowPass(const ApplicationVulkanContext& appContext,
                          ShadowPass&                     shadowPass,
                          const RenderPassDescription& renderPassDescription) {

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
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    depthAttachment.format  = findDepthFormat(appContext.baseContext);
    depthAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
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

    /*
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;

    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;  //
    | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; dependency.srcAccessMask = 0;

    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;  //
    | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; dependency.dstAccessMask =
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;  // |
    */

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
                           shadowPass.renderPassContext.descriptorSetLayouts);

    shadowPass.renderPassContext.renderPassDescription = renderPassDescription;

    const uint32_t SHADOW_MAP_WIDTH  = 500;
    const uint32_t SHADOW_MAP_HEIGHT = 500;

    shadowPass.shadowMapWidth  = SHADOW_MAP_WIDTH;
    shadowPass.shadowMapHeight = SHADOW_MAP_HEIGHT;

    VkFormat depthFormat = findDepthFormat(appContext.baseContext);

    createImage(appContext.baseContext, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, 1,
                VK_SAMPLE_COUNT_1_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                shadowPass.depthImage.image, shadowPass.depthImage.memory);

    shadowPass.depthImage.imageView =
        createImageView(appContext.baseContext, shadowPass.depthImage.image,
                        depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

    transitionImageLayout(appContext.baseContext, appContext.commandContext,
                          shadowPass.depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                          VK_IMAGE_ASPECT_DEPTH_BIT, 1);

    initializeShadowDepthBuffer(appContext, shadowPass, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
}

void initializeShadowDepthBuffer(const ApplicationVulkanContext& appContext,
                                 ShadowPass&                     shadowPass,
                                 uint32_t                        width,
                                 uint32_t                        height) {
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = shadowPass.renderPassContext.renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments    = &shadowPass.depthImage.imageView;
    framebufferInfo.width           = width;
    framebufferInfo.height          = height;
    framebufferInfo.layers          = 1;

    if(vkCreateFramebuffer(appContext.baseContext.device, &framebufferInfo,
                           nullptr, &shadowPass.depthFrameBuffer)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create framebuffer!");
    }
}

void createGraphicsPipeline(const ApplicationVulkanContext& appContext,
                            RenderPassContext&              renderPass,
                            VkPipelineLayout&               pipelineLayout,
                            VkPipeline&                     graphicsPipeline,
                            const RenderPassDescription& renderPassDescription,
                            const std::vector<VkDescriptorSetLayout>& layouts) {

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

    std::vector<VkDynamicState>      dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                      VK_DYNAMIC_STATE_SCISSOR};
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
            attachments.push_back(appContext.swapchainContext.depthImage.imageView);
            attachments.push_back(appContext.swapchainContext.swapChainImageViews[i]);
        } else {
            attachments.push_back(appContext.swapchainContext.swapChainImageViews[i]);
            attachments.push_back(appContext.swapchainContext.depthImage.imageView);
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
                                    RenderPassContext& renderPass) {
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
                           renderPass.renderPassDescription, renderPass.descriptorSetLayouts);
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
VkPushConstantRange createPushConstantRange(uint32_t offset, uint32_t size, ShaderStage shaderStage) {
    VkPushConstantRange pushConstantRange;

    pushConstantRange.offset = offset;
    pushConstantRange.size   = size;

    pushConstantRange.stageFlags = getStageFlag(shaderStage);

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

void createDescriptorPool(const VulkanBaseContext& baseContext, RenderContext& renderContext) {
    // TODO try to find a better way to do descriptorPools

    std::vector<VkDescriptorPoolSize> poolSizes;

    VkDescriptorPoolSize shadowTransformPoolSize;
    shadowTransformPoolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    shadowTransformPoolSize.descriptorCount = 1;
    poolSizes.push_back(shadowTransformPoolSize);


    VkDescriptorPoolSize mainTransformPoolSize;
    mainTransformPoolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    mainTransformPoolSize.descriptorCount = 1;
    poolSizes.push_back(mainTransformPoolSize);

    VkDescriptorPoolSize mainMaterialPoolSize;
    mainMaterialPoolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    mainMaterialPoolSize.descriptorCount = 1;
    poolSizes.push_back(mainMaterialPoolSize);

    VkDescriptorPoolSize mainDepthPoolSize;
    mainDepthPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    mainDepthPoolSize.descriptorCount = 1;
    poolSizes.push_back(mainDepthPoolSize);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes    = poolSizes.data();
    poolInfo.maxSets       = 4;

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
    bufferInfo.range  = sizeof(SceneTransform);

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

    createBufferResources(appContext, sizeof(SceneTransform),
                          renderContext.renderPasses.shadowPass.transformBuffer);
}

void cleanShadowPass(const VulkanBaseContext& baseContext, const ShadowPass& shadowPass) {
    vkDestroyBuffer(baseContext.device, shadowPass.transformBuffer.buffer, nullptr);
    vkFreeMemory(baseContext.device, shadowPass.transformBuffer.bufferMemory, nullptr);

    cleanupRenderPassContext(baseContext, shadowPass.renderPassContext);

    vkDestroyDescriptorSetLayout(baseContext.device,
                                 shadowPass.transformDescriptorSetLayout, nullptr);

    vkDestroyImageView(baseContext.device, shadowPass.depthImage.imageView, nullptr);

    vkDestroyImage(baseContext.device, shadowPass.depthImage.image, nullptr);
    vkFreeMemory(baseContext.device, shadowPass.depthImage.memory, nullptr);

    vkDestroyFramebuffer(baseContext.device, shadowPass.depthFrameBuffer, nullptr);
}

void createMainPassResources(const ApplicationVulkanContext& appContext,
                             RenderContext&                  renderContext,
                             const std::vector<Material>&    materials) {

    createBufferResources(appContext, sizeof(SceneTransform),
                          renderContext.renderPasses.mainPass.transformBuffer);

    createMaterialsBuffer(appContext, renderContext, materials);
}

void createDepthSampler(const ApplicationVulkanContext& appContext, MainPass& mainPass) {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType     = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    samplerInfo.anisotropyEnable = VK_FALSE;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp     = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod     = 0.0f;
    samplerInfo.maxLod     = 0.0f;

    if(vkCreateSampler(appContext.baseContext.device, &samplerInfo, nullptr,
                       &mainPass.depthSampler)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void createMainPassDescriptorSets(const ApplicationVulkanContext& appContext,
                                  RenderContext& renderContext) {

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


    VkDescriptorSetAllocateInfo materialAllocInfo{};
    materialAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    materialAllocInfo.descriptorPool     = renderContext.descriptorPool;
    materialAllocInfo.descriptorSetCount = 1;
    materialAllocInfo.pSetLayouts = &mainPass.materialDescriptorSetLayout;

    if(vkAllocateDescriptorSets(appContext.baseContext.device, &materialAllocInfo,
                                &mainPass.materialDescriptorSet)
       != VK_SUCCESS) {
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

    vkUpdateDescriptorSets(appContext.baseContext.device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);


    VkDescriptorSetAllocateInfo depthAllocInfo{};
    depthAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    depthAllocInfo.descriptorPool     = renderContext.descriptorPool;
    depthAllocInfo.descriptorSetCount = 1;
    depthAllocInfo.pSetLayouts        = &mainPass.depthDescriptorSetLayout;

    auto res = vkAllocateDescriptorSets(appContext.baseContext.device, &depthAllocInfo,
                                        &mainPass.depthDescriptorSet);

    if(res != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    VkDescriptorImageInfo imageInfo;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = renderContext.renderPasses.shadowPass.depthImage.imageView;
    imageInfo.sampler = renderContext.renderPasses.mainPass.depthSampler;

    VkWriteDescriptorSet depthDescriptorWrite;
    depthDescriptorWrite.sType      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    depthDescriptorWrite.pNext      = nullptr;
    depthDescriptorWrite.dstSet     = mainPass.depthDescriptorSet;
    depthDescriptorWrite.dstBinding = DepthBindings::eShadowDepthBuffer;
    depthDescriptorWrite.dstArrayElement = 0;
    depthDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    depthDescriptorWrite.descriptorCount = 1;
    depthDescriptorWrite.pImageInfo      = &imageInfo;

    descriptorWrites.emplace_back(depthDescriptorWrite);

    vkUpdateDescriptorSets(appContext.baseContext.device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
}

void cleanMainPass(const VulkanBaseContext& baseContext, const MainPass& mainPass) {
    vkDestroySampler(baseContext.device, mainPass.depthSampler, nullptr);

    vkDestroyBuffer(baseContext.device, mainPass.transformBuffer.buffer, nullptr);
    vkFreeMemory(baseContext.device, mainPass.transformBuffer.bufferMemory, nullptr);

    vkDestroyBuffer(baseContext.device, mainPass.materialBuffer.buffer, nullptr);
    vkFreeMemory(baseContext.device, mainPass.materialBuffer.bufferMemory, nullptr);

    cleanupRenderPassContext(baseContext, mainPass.renderPassContext);

    vkDestroyDescriptorSetLayout(baseContext.device,
                                 mainPass.transformDescriptorSetLayout, nullptr);

    vkDestroyDescriptorSetLayout(baseContext.device,
                                 mainPass.materialDescriptorSetLayout, nullptr);

    vkDestroyDescriptorSetLayout(baseContext.device,
                                 mainPass.depthDescriptorSetLayout, nullptr);
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
                           const std::vector<Material>&    materials) {
    VkDeviceSize bufferSize = sizeof(Material) * materials.size();

    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(appContext.baseContext, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(appContext.baseContext.device, stagingBufferMemory, 0, bufferSize, 0, &data);

    // We use Host Coherent Memory to make sure data is synchronized, could also manually flush Memory Ranges
    memcpy(data, materials.data(), (size_t)bufferSize);
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
