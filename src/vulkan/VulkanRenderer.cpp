#include <stdexcept>
#include <array>
#include "VulkanRenderer.h"
#include "VulkanSetup.h"

#include <iostream>
#include "rendering/RenderContext.h"
#include "rendering/RenderSetup.h"
#include "rendering/host_device.h"
#include "game/PlayerComponent.h"
#include "rendering/CSMUtils.h"

VulkanRenderer::VulkanRenderer(ApplicationVulkanContext& context, RenderContext& renderContext)
    : m_Context(context)
    , m_RenderContext(renderContext) {
    createSyncObjects(context.baseContext);
}


void VulkanRenderer::cleanVulkanRessources() {
    vkDestroySemaphore(m_Context.baseContext.device, m_ImageAvailableSemaphore, nullptr);
    vkDestroySemaphore(m_Context.baseContext.device, m_RenderFinishedSemaphore, nullptr);
    vkDestroyFence(m_Context.baseContext.device, m_InFlightFence, nullptr);
}

void VulkanRenderer::render(Scene& scene) {
    /*
    if (!scene.hasObject()) {
        std::cout << "Scene needs objects to be rendered" << std::endl;
        return;
    }
     */
    // reset imgui per frame counters
    m_RenderContext.imguiData.meshDrawCalls  = 0;
    m_RenderContext.imguiData.lightDrawCalls = 0;
    m_RenderContext.imguiData.shadowPassDrawCalls = 0;

    vkWaitForFences(m_Context.baseContext.device, 1, &m_InFlightFence, VK_TRUE, UINT64_MAX);

    // TODO this causes validation layer errors on some machines because
    // semaphore is signalled after recreation
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_Context.baseContext.device,
                                            m_Context.swapchainContext.swapChain,
                                            UINT64_MAX, m_ImageAvailableSemaphore,
                                            VK_NULL_HANDLE, &imageIndex);

    if(result == VK_ERROR_OUT_OF_DATE_KHR || m_Context.window->wasResized()) {
        recreateSwapChain(m_Context, m_RenderContext);
        m_Context.window->setResized(false);
        return;
    } else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(m_Context.baseContext.device, 1, &m_InFlightFence);

    vkResetCommandBuffer(m_Context.commandContext.commandBuffer, 0);

    if(m_RenderContext.usesImgui) {
        scene.registerSceneImgui(m_RenderContext);
        ImGui::Begin("Renderer");
        if(ImGui::Button("Recompile Shaders")) {
            recompileToSecondaryPipeline();
        }
        ImGui::End();
    }

    scene.doCameraUpdate(m_RenderContext);
    scene.doGameplayUpdate();

    updateUniformBuffer(scene);

    // TODO iterate render passes (or "hardcode" them), use Pipeline barriers
    // between frame buffer writes or whatever needed ?
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCmdPipelineBarrier.html
    // pipeline barriers implicit ordering for all stages before and after ?
    // https://themaister.net/blog/2019/08/14/yet-another-blog-explaining-vulkan-synchronization/

    recordCommandBuffer(scene, imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {m_ImageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores    = waitSemaphores;
    submitInfo.pWaitDstStageMask  = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &m_Context.commandContext.commandBuffer;

    // TODO Vulkan Layers throw error one reisze to very small window ?
    VkSemaphore signalSemaphores[]  = {m_RenderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    if(vkQueueSubmit(m_Context.baseContext.graphicsQueue, 1, &submitInfo, m_InFlightFence)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;

    VkSwapchainKHR swapChains[] = {m_Context.swapchainContext.swapChain};
    presentInfo.swapchainCount  = 1;
    presentInfo.pSwapchains     = swapChains;
    presentInfo.pImageIndices   = &imageIndex;

    vkQueuePresentKHR(m_Context.baseContext.presentQueue, &presentInfo);
}

void VulkanRenderer::recordCommandBuffer(Scene& scene, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags            = 0;        // Optional
    beginInfo.pInheritanceInfo = nullptr;  // Optional

    if(vkBeginCommandBuffer(m_Context.commandContext.commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    if(m_RenderContext.imguiData.shadows) {

        recordShadowPass(scene, imageIndex);

        VkMemoryBarrier memoryBarrier;
        memoryBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memoryBarrier.pNext         = VK_NULL_HANDLE;
        memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        memoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

        vkCmdPipelineBarrier(m_Context.commandContext.commandBuffer,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 1,
                             &memoryBarrier, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE);
    }

    recordGeometryPass(scene);

    VkMemoryBarrier memoryBarrier2;
    memoryBarrier2.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier2.pNext         = VK_NULL_HANDLE;
    memoryBarrier2.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    memoryBarrier2.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

    vkCmdPipelineBarrier(m_Context.commandContext.commandBuffer,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 1,
                         &memoryBarrier2, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE);

    recordMainRenderPass(scene, imageIndex);

    if(vkEndCommandBuffer(m_Context.commandContext.commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void VulkanRenderer::recordShadowPass(Scene& scene, uint32_t imageIndex) {
    ShadowPass&      shadowPass    = m_RenderContext.renderPasses.shadowPass;
    VkCommandBuffer& commandBuffer = m_Context.commandContext.commandBuffer;

    VkExtent2D shadowExtent;
    shadowExtent.width  = shadowPass.shadowMapWidth;
    shadowExtent.height = shadowPass.shadowMapHeight;


    VkClearValue clearValue;
    clearValue.depthStencil = {1.0f, 0};

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = shadowExtent.width;
    viewport.height   = shadowExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = shadowExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);


    int numberCascades = m_RenderContext.renderSettings.shadowMappingSettings.numberCascades;

    std::vector<glm::mat4>        VPMats(numberCascades);
    std::vector<SplitDummyStruct> splitDepths(numberCascades);


    glm::mat4 invViewProj = glm::inverse(
        getPerspectiveMatrix(m_RenderContext.renderSettings.perspectiveSettings,
                             m_Context.swapchainContext.swapChainExtent.width,
                             m_Context.swapchainContext.swapChainExtent.height)
        * scene.getCameraRef().getCameraMatrix());

    scene.getCameraRef().normalizeViewDir();

    calculateShadowCascades(m_RenderContext.renderSettings.perspectiveSettings, invViewProj,
                            m_RenderContext.renderSettings.shadowMappingSettings,
                            scene.getCameraRef().getViewDir(), VPMats, splitDepths);


    memcpy(m_RenderContext.renderPasses.mainPass.cascadeSplitsBuffer.bufferMemoryMapping,
           splitDepths.data(), numberCascades * sizeof(SplitDummyStruct));

    memcpy(m_RenderContext.renderPasses.shadowPass.transformBuffer.bufferMemoryMapping,
           VPMats.data(), numberCascades * sizeof(glm::mat4));


    for(size_t i = 0; i < MAX_CASCADES; i++) {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass  = shadowPass.renderPassContext.renderPass;
        renderPassInfo.framebuffer = shadowPass.depthFrameBuffers[i];

        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues    = &clearValue;

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = shadowExtent;

        vkCmdBeginRenderPass(commandBuffer,
                             &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(m_Context.commandContext.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPass.shadowPipeline);


        if(i < m_RenderContext.renderSettings.shadowMappingSettings.numberCascades) {


            vkCmdSetDepthBias(commandBuffer,
                              m_RenderContext.imguiData.depthBiasConstant, 0.0f,
                              m_RenderContext.imguiData.depthBiasSlope * (i + 1));

            vkCmdBindDescriptorSets(m_Context.commandContext.commandBuffer,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    shadowPass.shadowPipelineLayout, 0, 1,
                                    &m_RenderContext.renderPasses.shadowPass.transformDescriptorSet,
                                    0, nullptr);

            vkCmdBindDescriptorSets(m_Context.commandContext.commandBuffer,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    shadowPass.shadowPipelineLayout, 1, 1,
                                    &m_RenderContext.renderPasses.shadowPass.materialDescriptorSet,
                                    0, nullptr);

            ShadowPushConstant shadowPushConstant;
            shadowPushConstant.cascadeIndex  = i;
            shadowPushConstant.materialIndex = 0;

            EntityId playerID = -1;

            for(EntityId id : SceneView<PlayerComponent, Transformation>(scene)) {
                playerID = id;
            }

            for(EntityId id : SceneView<ModelComponent, Transformation>(scene)) {
                auto* modelComponent = scene.getComponent<ModelComponent>(id);
                auto* transformComponent = scene.getComponent<Transformation>(id);

                Model& model = scene.getSceneData().models[modelComponent->modelIndex];

                int counter = 0;
                for(auto& meshPartIndex : model.meshPartIndices) {
                    // this is fairly hardcoded so that the spiky mesh of the
                    // player has no shadow the meshes of the spikes are at
                    // position 1 and 2 (this is the hardcoded part)
                    if(!m_RenderContext.imguiData.playerSpikesShadow
                       && id == playerID && (counter == 1 || counter == 2)) {
                        counter++;
                        continue;
                    }
                    MeshPart& meshPart = scene.getSceneData().meshParts[meshPartIndex];
                    Mesh& mesh = scene.getSceneData().meshes[meshPart.meshIndex];
                    VkBuffer     vertexBuffers[] = {mesh.vertexBuffer};
                    VkDeviceSize offsets[]       = {0};

                    shadowPushConstant.materialIndex = meshPart.materialIndex;

                    vkCmdBindVertexBuffers(commandBuffer,
                                           0, 1, vertexBuffers, offsets);

                    vkCmdBindIndexBuffer(commandBuffer,
                                         mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

                    // only updates transformation matrix if the transformation has changed
                    shadowPushConstant.transform = transformComponent->getTransformationMatrix();

                    vkCmdPushConstants(m_Context.commandContext.commandBuffer,
                                       shadowPass.shadowPipelineLayout,
                                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                       0,  // offset
                                       sizeof(ShadowPushConstant), &shadowPushConstant);

                    m_RenderContext.imguiData.shadowPassDrawCalls++;
                    vkCmdDrawIndexed(commandBuffer,
                                     mesh.indicesCount, 1, 0, 0, 0);

                    counter++;
                }
            }
        }
        vkCmdEndRenderPass(commandBuffer);
    }
}

void VulkanRenderer::recordMainRenderPass(Scene& scene, uint32_t imageIndex) {
    MainPass&          mainPass       = m_RenderContext.renderPasses.mainPass;
    RenderPassContext& mainRenderPass = mainPass.renderPassContext;
    VkCommandBuffer&   commandBuffer  = m_Context.commandContext.commandBuffer;

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType      = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = mainRenderPass.renderPass;
    renderPassInfo.framebuffer =
        m_Context.swapchainContext.swapChainFramebuffers[imageIndex];

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_Context.swapchainContext.swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues    = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer,
                         &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width =
        static_cast<float>(m_Context.swapchainContext.swapChainExtent.width);
    viewport.height =
        static_cast<float>(m_Context.swapchainContext.swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_Context.swapchainContext.swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    if(m_RenderContext.imguiData.visualizeShadowBuffer) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          mainPass.visualizePipeline);

        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mainPass.visualizePipelineLayout, 0, 1,
            &mainPass.depthDescriptorSet, 0, nullptr);

        ShadowControlPushConstant& shadowControlPushConstant = mainPass.shadowControlPushConstant;

        ShadowMappingSettings shadowSettings =
            m_RenderContext.renderSettings.shadowMappingSettings;

        if(shadowSettings.cascadeVisIndex < 0)
            shadowSettings.cascadeVisIndex = 0;
        else if(shadowSettings.cascadeVisIndex >= shadowSettings.numberCascades)
            shadowSettings.cascadeVisIndex = shadowSettings.numberCascades - 1;

        shadowControlPushConstant.cascadeIndex = shadowSettings.cascadeVisIndex;

        vkCmdPushConstants(commandBuffer,
                           mainPass.visualizePipelineLayout,
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,  // offset
                           sizeof(ShadowControlPushConstant), &shadowControlPushConstant);

        vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    } else {
        // render screen quad for primary light source
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          mainPass.primaryLightingPipeline);

        // bind DescriptorSet 0 (Camera Transformations)
        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mainPass.primaryLightingPipelineLayout, 0,
            1, &mainPass.transformDescriptorSet, 0, nullptr);
        // bind DescriptorSet 1 (Shadow)
        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mainPass.primaryLightingPipelineLayout, 1,
            1, &mainPass.depthDescriptorSet, 0, nullptr);
        // bind DescriptorSet 2 (gBuffer)
        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mainPass.primaryLightingPipelineLayout, 2,
            1, &mainPass.gBufferDescriptorSet, 0, nullptr);
        // bind DescriptorSet 3 (IBL)
        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mainPass.primaryLightingPipelineLayout, 3,
            1, &mainPass.skyboxDescriptorSet, 0, nullptr);

        // create PushConstant object and initialize with default values
        PushConstant& pushConstant    = mainPass.pushConstant;
        pushConstant.transformation   = glm::mat4(1);
        pushConstant.worldCamPosition = scene.getCameraRef().getWorldPos();
        pushConstant.resolution =
            glm::ivec2(m_Context.swapchainContext.swapChainExtent.width,
                       m_Context.swapchainContext.swapChainExtent.height);
        pushConstant.materialIndex = 0;
        pushConstant.cascadeCount =
            m_RenderContext.renderSettings.shadowMappingSettings.numberCascades;

        pushConstant.controlFlags = 0;
        if(m_RenderContext.imguiData.doPCF)
            pushConstant.controlFlags |= PCF_CONTROL_BIT;
        if(m_RenderContext.renderSettings.shadowMappingSettings.visualizeCascades)
            pushConstant.controlFlags |= CASCADE_VIS_CONTROL_BIT;

        vkCmdPushConstants(commandBuffer,
                           mainPass.primaryLightingPipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,  // offset
                           sizeof(PushConstant), &pushConstant);

        vkCmdDraw(commandBuffer, 6, 1, 0, 0);
        
        if(m_RenderContext.imguiData.pointLights) {
            // render point lights for shading
            {
                vkCmdBindPipeline(commandBuffer,
                                  VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  mainPass.pointLightsPipeline);

                // bind DescriptorSet 0 (Camera Transformations)
                vkCmdBindDescriptorSets(
                    commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    mainPass.pointLightsPipelineLayout,
                    0, 1, &mainPass.transformDescriptorSet,
                    0, nullptr);

                // bind DescriptorSet 1 (gBuffer)
                vkCmdBindDescriptorSets(
                    commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    mainPass.pointLightsPipelineLayout,
                    1, 1, &mainPass.gBufferDescriptorSet,
                    0, nullptr);

                // bind point light mesh (it will remain the same for each light source
                Mesh     pointLightMesh  = scene.getSceneData().pointLightMesh;
                VkBuffer vertexBuffers[] = {pointLightMesh.vertexBuffer};
                VkDeviceSize offsets[]   = {0};
                vkCmdBindVertexBuffers(commandBuffer,
                                       0, 1, vertexBuffers, offsets);
                vkCmdBindIndexBuffer(commandBuffer,
                                     pointLightMesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

                PointLightPushConstant& pointLightPushConstant = mainPass.pointLightPushConstant;
                pointLightPushConstant.worldCamPosition =
                    scene.getCameraRef().getWorldPos();
                pointLightPushConstant.resolution =
                    glm::ivec2(m_Context.swapchainContext.swapChainExtent.width,
                               m_Context.swapchainContext.swapChainExtent.height);

                // draw icosphere per point light
                for(PointLight& pointLight : scene.getSceneData().lights) {
                    // build transformation matrix for vertex shader
                    glm::mat4 scaleMat =
                        glm::scale(glm::mat4(1), glm::vec3(pointLight.radius));
                    glm::mat4 translateMat =
                        glm::translate(glm::mat4(1), pointLight.position);

                    pointLightPushConstant.transformation = translateMat * scaleMat;
                    pointLightPushConstant.position  = pointLight.position;
                    pointLightPushConstant.intensity = pointLight.intensity;
                    pointLightPushConstant.radius    = pointLight.radius;

                    // sending push constant to GPU
                    vkCmdPushConstants(
                        commandBuffer,
                        mainPass.pointLightsPipelineLayout,
                        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                        0,  // offset
                        sizeof(PointLightPushConstant), &pointLightPushConstant);

                    vkCmdDrawIndexed(commandBuffer,
                                     pointLightMesh.indicesCount, 1, 0, 0, 0);
                }
            }
        }

        // render skybpx
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          mainPass.skyboxPipeline);

        // bind DescriptorSet 0 (Camera Transformations)
        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mainPass.skyboxPipelineLayout, 0, 1,
            &mainPass.transformDescriptorSet, 0, nullptr);

        // bind DescriptorSet 1 (Materials)
        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mainPass.skyboxPipelineLayout, 1, 1,
            &mainPass.skyboxDescriptorSet, 0, nullptr);

        SkyboxPushConstant& skyboxPushConstant = mainPass.skyboxPushConstant;
        skyboxPushConstant.exposure = m_RenderContext.imguiData.exposure;

        // sending push constant to GPU
        vkCmdPushConstants(commandBuffer,
                           mainPass.skyboxPipelineLayout,
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,  // offset
                           sizeof(SkyboxPushConstant), &skyboxPushConstant);

        vkCmdDraw(commandBuffer, 6, 1, 0, 0);
    }

    if(m_RenderContext.usesImgui) {
        // @IMGUI
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                        commandBuffer);
    }

    vkCmdEndRenderPass(commandBuffer);
}

void VulkanRenderer::recordGeometryPass(Scene& scene) {
    // geometry pass
    MainPass&          mainPass       = m_RenderContext.renderPasses.mainPass;
    RenderPassContext& mainRenderPass = mainPass.renderPassContext;
    VkCommandBuffer&   commandBuffer  = m_Context.commandContext.commandBuffer;

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass  = mainPass.geometryPass;
    renderPassInfo.framebuffer = mainPass.gBuffer;

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_Context.swapchainContext.swapChainExtent;

    std::array<VkClearValue, 4> clearValues{};
    clearValues[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
    clearValues[1].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
    clearValues[2].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
    clearValues[3].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues    = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer,
                         &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);


    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width =
        static_cast<float>(m_Context.swapchainContext.swapChainExtent.width);
    viewport.height =
        static_cast<float>(m_Context.swapchainContext.swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_Context.swapchainContext.swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // render meshes
    {
        vkCmdBindPipeline(commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, mainPass.geometryPassPipeline);

        // bind DescriptorSet 0 (Camera Transformations)
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mainPass.geometryPassPipelineLayout, 0, 1,
                                &mainPass.transformDescriptorSet, 0, nullptr);

        // TODO: from my understanding, this descriptor set only has to be bound
        //       once, but I got errors when doing so
        //        -> should make it possible for performance reasons
        //        -> use separate command buffer that is only updated when the graphics pipeline is changed

        // bind DescriptorSet 1 (Materials)
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mainPass.geometryPassPipelineLayout, 1, 1,
                                &mainPass.materialDescriptorSet, 0, nullptr);

        // bind DescriptorSet 2 (Shadows)
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mainPass.geometryPassPipelineLayout, 2, 1,
                                &mainPass.depthDescriptorSet, 0, nullptr);


        // create PushConstant object and initialize with default values
        PushConstant& pushConstant = mainPass.pushConstant;
        pushConstant.transformation   = glm::mat4(1);
        pushConstant.worldCamPosition = scene.getCameraRef().getWorldPos();
        pushConstant.materialIndex    = 0;
        pushConstant.cascadeCount =
            m_RenderContext.renderSettings.shadowMappingSettings.numberCascades;

        pushConstant.controlFlags = 0;
        if(m_RenderContext.imguiData.doPCF)
            pushConstant.controlFlags |= PCF_CONTROL_BIT;
        if(m_RenderContext.renderSettings.shadowMappingSettings.visualizeCascades)
            pushConstant.controlFlags |= CASCADE_VIS_CONTROL_BIT;

        for(EntityId id : SceneView<ModelComponent, Transformation>(scene)) {
            auto* modelComponent     = scene.getComponent<ModelComponent>(id);
            auto* transformComponent = scene.getComponent<Transformation>(id);

            Model& model = scene.getSceneData().models[modelComponent->modelIndex];

            // only update transformation matrix if the transformation has changed
            pushConstant.transformation = transformComponent->getTransformationMatrix();
            pushConstant.normalsTransformation =
                transformComponent->getNormalsTransformationMatrix();

            for(auto& meshPartIndex : model.meshPartIndices) {
                MeshPart& meshPart = scene.getSceneData().meshParts[meshPartIndex];
                Mesh&     mesh = scene.getSceneData().meshes[meshPart.meshIndex];
                VkBuffer vertexBuffers[] = {mesh.vertexBuffer};
                VkDeviceSize offsets[]   = {0};

                pushConstant.materialIndex = meshPart.materialIndex;

                vkCmdBindVertexBuffers(commandBuffer,
                                       0, 1, vertexBuffers, offsets);

                vkCmdBindIndexBuffer(commandBuffer,
                                     mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

                vkCmdPushConstants(commandBuffer,
                                   mainPass.geometryPassPipelineLayout,
                                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                   0,  // offset
                                   sizeof(PushConstant), &pushConstant);

                m_RenderContext.imguiData.meshDrawCalls++;
                vkCmdDrawIndexed(commandBuffer,
                                 mesh.indicesCount, 1, 0, 0, 0);
            }
        }
    }

    // render point lights for stencil shadow volumes
    if(m_RenderContext.imguiData.pointLights) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          m_RenderContext.renderPasses.mainPass.stencilPipeline);

        // bind point light mesh (it will remain the same for each light source
        Mesh         pointLightMesh  = scene.getSceneData().pointLightMesh;
        VkBuffer     vertexBuffers[] = {pointLightMesh.vertexBuffer};
        VkDeviceSize offsets[]       = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1,
                               vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer,
                             pointLightMesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        glm::mat4 projection =
            getPerspectiveMatrix(m_RenderContext.renderSettings.perspectiveSettings,
                                 m_Context.swapchainContext.swapChainExtent.width,
                                 m_Context.swapchainContext.swapChainExtent.height);
        // one tutorial says openGL has different convention for Y
        // coordinates in clip space than vulkan, need to flip it
        projection[1][1] *= -1;
        StencilPushConstant& stencilPushConstant = mainPass.stencilPushConstant;
        stencilPushConstant.projView =
            projection * scene.getCameraRef().getCameraMatrix();

        // sending push constant to GPU
        vkCmdPushConstants(commandBuffer,
                           m_RenderContext.renderPasses.mainPass.stencilPipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT,
                           0,  // offset
                           sizeof(StencilPushConstant), &stencilPushConstant);

        // draw icosphere per point light
        for(PointLight& pointLight : scene.getSceneData().lights) {
            // build transformation matrix for vertex shader
            glm::mat4 scaleMat = glm::scale(glm::mat4(1), glm::vec3(pointLight.radius));
            glm::mat4 translateMat = glm::translate(glm::mat4(1), pointLight.position);

            stencilPushConstant.transformation = translateMat * scaleMat;

            // sending push constant to GPU
            vkCmdPushConstants(commandBuffer,
                               m_RenderContext.renderPasses.mainPass.stencilPipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT,
                               0,  // offset
                               sizeof(StencilPushConstant), &stencilPushConstant);

            m_RenderContext.imguiData.lightDrawCalls++;

            vkCmdDrawIndexed(commandBuffer,
                             pointLightMesh.indicesCount, 1, 0, 0, 0);
        }
    }

    vkCmdEndRenderPass(commandBuffer);
}


void VulkanRenderer::createSyncObjects(VulkanBaseContext& baseContext) {
    /*
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    */

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    /*
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    }
     */

    if(vkCreateSemaphore(baseContext.device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphore) != VK_SUCCESS
       || vkCreateSemaphore(baseContext.device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphore) != VK_SUCCESS
       || vkCreateFence(baseContext.device, &fenceInfo, nullptr, &m_InFlightFence) != VK_SUCCESS) {

        throw std::runtime_error("failed to create synchronization objects for a frame!");
    }
}

void VulkanRenderer::updateUniformBuffer(Scene& scene) {
    glm::mat4 projection =
        getPerspectiveMatrix(m_RenderContext.renderSettings.perspectiveSettings,
                             m_Context.swapchainContext.swapChainExtent.width,
                             m_Context.swapchainContext.swapChainExtent.height);
    // one tutorial says openGL has different convention for Y coordinates in
    // clip space than vulkan, need to flip it
    projection[1][1] *= -1;

    CameraUniform& cameraUniform = m_RenderContext.uniforms.cameraUniform;
    cameraUniform.view = scene.getCameraRef().getCameraMatrix();
    cameraUniform.proj = projection;
    cameraUniform.viewInverse = glm::inverse(cameraUniform.view);
    cameraUniform.projInverse = glm::inverse(projection);

    // PushConstants would be more efficient for often changing small data buffers

    memcpy(m_RenderContext.renderPasses.mainPass.transformBuffer.bufferMemoryMapping,
           &cameraUniform, sizeof(CameraUniform));


    LightingInformation& lightingInformation = m_RenderContext.uniforms.lightingInformation;
    lightingInformation.cameraPosition = scene.getCameraRef().getWorldPos();
    lightingInformation.lightDirection =
        glm::normalize(m_RenderContext.renderSettings.shadowMappingSettings.lightDirection);
    lightingInformation.lightIntensity =
        m_RenderContext.renderSettings.lightingSetting.sunColor;
    lightingInformation.shadows = m_RenderContext.imguiData.shadows;

    memcpy(m_RenderContext.renderPasses.mainPass.lightingBuffer.bufferMemoryMapping,
           &lightingInformation, sizeof(LightingInformation));
    // memcpy(scene.getUniformBufferMapping(), &sceneTransform, sizeof(sceneTransform));
}

void VulkanRenderer::recompileToSecondaryPipeline() {
    std::cout << "\nRecompiling Shaders...\n";
    MainPass&          mainPass    = m_RenderContext.renderPasses.mainPass;
    ShadowPass& shadowPass = m_RenderContext.renderPasses.shadowPass;
    VulkanBaseContext& baseContext = m_Context.baseContext;

    vkDeviceWaitIdle(m_Context.baseContext.device);

    // rebuild shadow pipeline
    cleanShadowPipeline(baseContext, shadowPass);
    createShadowPipeline(m_Context, shadowPass);

    // rebuild shadow map visualization pipeline
    cleanVisualizationPipeline(baseContext,
                               mainPass);
    createVisualizationPipeline(m_Context, m_RenderContext,
                                mainPass);

    // rebuild geometry pass pipeline
    cleanGeometryPassPipeline(baseContext,
                              mainPass);
    createGeometryPassPipeline(
        m_Context, m_RenderContext,
        mainPass.renderPassContext.renderPassDescription,
        mainPass);

    // rebuild stencil pipeline
    cleanStencilPipeline(baseContext, mainPass);
    createStencilPipeline(
        m_Context, m_RenderContext,
        mainPass.renderPassContext.renderPassDescription,
        mainPass);

    // rebuild primary lighting pipeline
    cleanPrimaryLightingPipeline(baseContext,
                                 mainPass);
    createPrimaryLightingPipeline(
        m_Context, m_RenderContext,
        mainPass.renderPassContext.renderPassDescription,
        mainPass);

    // rebuild point lights pipeline
    cleanPointLightsPipeline(baseContext, mainPass);
    createPointLightsPipeline(
        m_Context, m_RenderContext,
        mainPass.renderPassContext.renderPassDescription,
        mainPass);

    // rebuild skybox pipeline
    cleanSkyboxPipeline(baseContext, mainPass);
    createSkyboxPipeline(m_Context, m_RenderContext,
                         mainPass);
}
ApplicationVulkanContext VulkanRenderer::getContext() {
    return m_Context;
}
void VulkanRenderer::setRenderContext(RenderContext& renderContext) {
    m_RenderContext = renderContext;
}
