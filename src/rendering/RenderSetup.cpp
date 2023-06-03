#include <stdexcept>
#include <filesystem>
#include "RenderSetup.h"
#include "vulkan/VulkanUtils.h"
#include "utils/FileUtils.h"
#include "rendering/host_device.h"

void initializeSimpleSceneRenderContext(ApplicationVulkanContext &appContext, RenderContext &renderContext) {
    auto &settings = renderContext.renderSettings;
    settings.fov = glm::radians(45.0f);
    settings.nearPlane = 0.1;
    settings.farPlane = 1000;

    RenderSetupDescription renderSetupDescription;
    renderSetupDescription.enableImgui = true;

    renderSetupDescription.vertexShader.shaderStage = ShaderStage::VERTEX_SHADER;
    renderSetupDescription.vertexShader.shaderSourceName = "simpleScene.vert";
    renderSetupDescription.vertexShader.sourceDirectory = "res/shaders/source/";
    renderSetupDescription.vertexShader.spvDirectory = "res/shaders/spv/";

    renderSetupDescription.fragmentShader.shaderStage = ShaderStage::FRAGMENT_SHADER;
    renderSetupDescription.fragmentShader.shaderSourceName = "simpleScene.frag";
    renderSetupDescription.fragmentShader.sourceDirectory = "res/shaders/source/";
    renderSetupDescription.fragmentShader.spvDirectory = "res/shaders/spv/";

    renderSetupDescription.bindings.push_back(createUniformBufferLayoutBinding(SceneBindings::eCamera, 1, ShaderStage::VERTEX_SHADER));
    /*
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 2;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings.push_back(samplerLayoutBinding);
    */

    renderSetupDescription.pushConstantRanges.push_back(createPushConstantRange(0, sizeof(glm::mat4), ShaderStage::VERTEX_SHADER));

    initializeRenderContext(appContext, renderContext, renderSetupDescription);
}

void initializeRenderContext(ApplicationVulkanContext &appContext, RenderContext &renderContext, const RenderSetupDescription &renderSetupDescription) {
    createDescriptorSetLayout(appContext.baseContext, renderContext.renderPassContext, renderSetupDescription.bindings);
    createMaterialsBufferDescriptorSet(appContext.baseContext, renderContext.renderPassContext);

    initializeRenderPassContext(appContext, renderContext, renderSetupDescription);
    createFrameBuffers(appContext, renderContext);
    renderContext.renderSetupDescription = renderSetupDescription;

    renderContext.usesImgui = renderSetupDescription.enableImgui;
    if (renderContext.usesImgui) {
        initializeImGui(appContext, renderContext);
    }
}

void initializeRenderPassContext(const ApplicationVulkanContext &appContext, RenderContext &renderContext, const RenderSetupDescription &renderSetupDescription) {
    createRenderPass(appContext, renderContext);

    createGraphicsPipeline(appContext,
                           renderContext.renderPassContext,
                           renderContext.renderPassContext.pipelineLayouts[0],
                           renderContext.renderPassContext.graphicsPipelines[0],
                           renderSetupDescription);
}

void createDescriptorSetLayout(const VulkanBaseContext &context, RenderPassContext &renderContext, const std::vector<VkDescriptorSetLayoutBinding> &bindings) {
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr, &renderContext.descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void createMaterialsBufferDescriptorSet(const VulkanBaseContext& context,
    RenderPassContext& renderContext) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    VkDescriptorSetLayoutBinding materialsBinding;
    materialsBinding.binding         = MaterialsBindings::eMaterials;
    materialsBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    materialsBinding.descriptorCount = 1;
    materialsBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings.push_back(materialsBinding);

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings    = bindings.data();

    if(vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr,
                                   &renderContext.materialsDescriptorSetLayout)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create materials descriptor set layout!");
    }
}

void cleanupRenderContext(const VulkanBaseContext &baseContext, RenderContext &renderContext) {
    if (renderContext.usesImgui) {
        cleanupImGuiContext(baseContext, renderContext);
    }

    vkDestroyDescriptorSetLayout(baseContext.device,
                                 renderContext.renderPassContext.descriptorSetLayout,
                                 nullptr);
    vkDestroyDescriptorSetLayout(baseContext.device,
                                 renderContext.renderPassContext.materialsDescriptorSetLayout,
                                 nullptr);

    // delete graphics pipeline(s) (if the 2nd one was created, delete that too)
    vkDestroyPipeline(baseContext.device, renderContext.renderPassContext.graphicsPipelines[0], nullptr);
    vkDestroyPipelineLayout(baseContext.device, renderContext.renderPassContext.pipelineLayouts[0], nullptr);

    if(renderContext.renderPassContext.graphicsPipelines[1] != VK_NULL_HANDLE) {
        vkDestroyPipeline(baseContext.device, renderContext.renderPassContext.graphicsPipelines[1], nullptr);
        vkDestroyPipelineLayout(baseContext.device, renderContext.renderPassContext.pipelineLayouts[1], nullptr);
    }

    vkDestroyRenderPass(baseContext.device, renderContext.renderPassContext.renderPass, nullptr);
}

void cleanupImGuiContext(const VulkanBaseContext& baseContext, RenderContext& renderContext) {
    vkDestroyDescriptorPool(baseContext.device, renderContext.imguiContext.descriptorPool, nullptr);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void createRenderPass(const ApplicationVulkanContext &appContext, RenderContext &renderContext) {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = appContext.swapchainContext.swapChainImageFormat;

    colorAttachment.samples = appContext.graphicSettings.useMsaa ? appContext.graphicSettings.msaaSamples : VK_SAMPLE_COUNT_1_BIT;

    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; //This is overwritten when using msaa to better layout


    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat(appContext.baseContext);
    depthAttachment.samples = appContext.graphicSettings.useMsaa ? appContext.graphicSettings.msaaSamples : VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::vector<VkAttachmentDescription> attachments({colorAttachment, depthAttachment});

    // TODO: if samples is VK_SAMPLE_COUNT_1_BIT Vulkan expects no resolveAttachment
    // This has not been tested yet -> might be error source
    // Might also be important in createFrameBuffers ? (not sure, might also only save some memory)

    if (appContext.graphicSettings.useMsaa) {
        // when using Msaa, the color attachment is not the attachment presenting
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = appContext.swapchainContext.swapChainImageFormat;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        subpass.pResolveAttachments = &colorAttachmentResolveRef;

        attachments.push_back(colorAttachmentResolve);
    }

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;

    if (renderContext.usesImgui) {
        // @IMGUI
        dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;

        // @IMGUI
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    } else {
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;

        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }


    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(appContext.baseContext.device, &renderPassInfo, nullptr, &renderContext.renderPassContext.renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void createGraphicsPipeline(const ApplicationVulkanContext &appContext,
                            RenderPassContext &renderContext,
                            VkPipelineLayout& pipelineLayout,
                            VkPipeline&       graphicsPipeline,
                            const RenderSetupDescription &renderSetupDescription) {

    VkShaderModule vertShaderModule = createShaderModule(appContext.baseContext, renderSetupDescription.vertexShader);
    VkShaderModule fragShaderModule = createShaderModule(appContext.baseContext, renderSetupDescription.fragmentShader);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    // Can set compile time constants here
    // -> Let compiler optimize
    // vertShaderStageInfo.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;

    rasterizer.rasterizerDiscardEnable = VK_FALSE;

    // Enable Wireframe rendering here, requires GPU feature to be enabled
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

    rasterizer.lineWidth = 1.0f;
    // rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_TRUE;
    multisampling.minSampleShading = .2f;
    multisampling.rasterizationSamples = appContext.graphicSettings.useMsaa ? appContext.graphicSettings.msaaSamples : VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
    // colorBlendAttachment.blendEnable = VK_FALSE;

    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;


    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // get both descriptor set layouts
    std::vector<VkDescriptorSetLayout> layouts;
    layouts.push_back(renderContext.descriptorSetLayout);
    layouts.push_back(renderContext.materialsDescriptorSetLayout);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    pipelineLayoutInfo.setLayoutCount = layouts.size();
    pipelineLayoutInfo.pSetLayouts = layouts.data();

    VkPushConstantRange pushConstantRange;
    //this push constant range starts at the beginning
    pushConstantRange.offset = 0;
    //this push constant range takes up the size of a MeshPushConstants struct
    pushConstantRange.size = sizeof(glm::mat4);
    //this push constant range is accessible only in the vertex shader
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;


    pipelineLayoutInfo.pushConstantRangeCount = renderSetupDescription.pushConstantRanges.size();
    pipelineLayoutInfo.pPushConstantRanges = renderSetupDescription.pushConstantRanges.data();

    if(vkCreatePipelineLayout(appContext.baseContext.device, &pipelineLayoutInfo, nullptr,
                              &pipelineLayout)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.pDepthStencilState = &depthStencil;

    pipelineInfo.layout = pipelineLayout;

    pipelineInfo.renderPass = renderContext.renderPass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if(vkCreateGraphicsPipelines(appContext.baseContext.device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                 nullptr, &graphicsPipeline)
       !=
       VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(appContext.baseContext.device, fragShaderModule, nullptr);
    vkDestroyShaderModule(appContext.baseContext.device, vertShaderModule, nullptr);
}

void createFrameBuffers(ApplicationVulkanContext &appContext, RenderContext &renderContext) {
    if (appContext.swapchainContext.swapChainFramebuffers.size() != appContext.swapchainContext.swapChainImageViews.size()) {
        throw std::runtime_error("failed to create framebuffers, need one to one matching of framebuffers to swapchain images");
    }

    for (size_t i = 0; i < appContext.swapchainContext.swapChainImageViews.size(); i++) {

        std::vector<VkImageView> attachments;

        if (appContext.graphicSettings.useMsaa) {
            attachments.push_back(appContext.swapchainContext.colorImage.imageView);
            attachments.push_back(appContext.swapchainContext.depthImage.imageView);
            attachments.push_back(appContext.swapchainContext.swapChainImageViews[i]);
        } else {
            attachments.push_back(appContext.swapchainContext.swapChainImageViews[i]);
            attachments.push_back(appContext.swapchainContext.depthImage.imageView);
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderContext.renderPassContext.renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = appContext.swapchainContext.swapChainExtent.width;
        framebufferInfo.height = appContext.swapchainContext.swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(appContext.baseContext.device, &framebufferInfo, nullptr, &appContext.swapchainContext.swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }

}

// Builds a graphics pipeline and stores it in the secondary slot
void buildSecondaryGraphicsPipeline(const ApplicationVulkanContext &appContext, RenderContext &renderContext) {
    compileShader(renderContext.renderSetupDescription.vertexShader, appContext.baseContext.maxSupportedMinorVersion);
    compileShader(renderContext.renderSetupDescription.fragmentShader, appContext.baseContext.maxSupportedMinorVersion);
    if(renderContext.renderPassContext.graphicsPipelines[!renderContext.renderPassContext.activePipelineIndex] != VK_NULL_HANDLE) {
        vkDestroyPipeline(appContext.baseContext.device,
                          renderContext.renderPassContext.graphicsPipelines[!renderContext.renderPassContext.activePipelineIndex], nullptr);
        vkDestroyPipelineLayout(appContext.baseContext.device,
                                renderContext.renderPassContext.pipelineLayouts[!renderContext.renderPassContext.activePipelineIndex],
                                nullptr);
    }

    createGraphicsPipeline(appContext,
                           renderContext.renderPassContext,
                           renderContext.renderPassContext.pipelineLayouts[!renderContext.renderPassContext.activePipelineIndex],
                           renderContext.renderPassContext.graphicsPipelines[!renderContext.renderPassContext.activePipelineIndex],
                           renderContext.renderSetupDescription);
}

// Swaps to the secondary graphics pipeline if it is valid
bool swapGraphicsPipeline(const ApplicationVulkanContext &appContext, RenderContext &renderContext) {
    if(renderContext.renderPassContext.graphicsPipelines[!renderContext.renderPassContext.activePipelineIndex] != VK_NULL_HANDLE) {
        renderContext.renderPassContext.activePipelineIndex = !renderContext.renderPassContext.activePipelineIndex;
        return true;
    } else {
        return false;
    }
}

VkDescriptorSetLayoutBinding createUniformBufferLayoutBinding(uint32_t binding, uint32_t descriptorCount, ShaderStage shaderStage) {
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = descriptorCount;

    layoutBinding.stageFlags = getStageFlag(shaderStage);

    return layoutBinding;
}

VkPushConstantRange createPushConstantRange(uint32_t offset, uint32_t size, ShaderStage shaderStage) {
    VkPushConstantRange pushConstantRange;

    pushConstantRange.offset = offset;
    pushConstantRange.size = size;

    pushConstantRange.stageFlags = getStageFlag(shaderStage);

    return pushConstantRange;
}

// @IMGUI
// Heavily inspired by "https://github.com/ocornut/imgui/blob/master/examples/example_glfw_vulkan/main.cpp"
void initializeImGui(const ApplicationVulkanContext& appContext, RenderContext &renderContext) {
    // Create ImGui Context and set style
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    // TODO: is this line necessary?
    //io.IniFilename = nullptr;
    //io.LogFilename = nullptr;
    ImGui::StyleColorsDark();

    // Create Descriptor Pool for ImGui
    //ImGui_ImplGlfw_InitForVulkan(appContext.window->getWindowHandle(), true);
    // TODO: need to call
    // "vkDestroyDescriptorPool(g_Device, g_DescriptorPool, nullptr);" somewhere!!
    //       -> save handle to descriptor pool somewhere
    VkDescriptorPoolSize poolSizes[]    = {
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
    initInfo.Subpass        = 0;
    initInfo.Allocator      = VK_NULL_HANDLE;
    // TODO: find out if these are the correct values
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = imageCount;
    // TODO: could implement to check for errors
    initInfo.CheckVkResultFn = nullptr;

    if (appContext.graphicSettings.useMsaa) {
        initInfo.MSAASamples = appContext.graphicSettings.msaaSamples;
    }

    ImGui_ImplVulkan_Init(&initInfo, renderContext.renderPassContext.renderPass);

    // Upload fonts to GPU
    VkCommandBuffer commandBuffer =
            beginSingleTimeCommands(appContext.baseContext, appContext.commandContext);
    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
    endSingleTimeCommands(appContext.baseContext, appContext.commandContext, commandBuffer);
}