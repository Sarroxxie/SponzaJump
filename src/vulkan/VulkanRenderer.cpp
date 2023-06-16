#include <stdexcept>
#include <array>
#include "VulkanRenderer.h"
#include "VulkanSetup.h"

#include <iostream>
#include "rendering/RenderContext.h"
#include "rendering/RenderSetup.h"
#include "rendering/host_device.h"

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
        if(ImGui::Button("Swap Pipeline")) {
            swapToSecondaryPipeline();
        }
        ImGui::End();
    }

    scene.doCameraUpdate(m_RenderContext);

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

    // TODO update shadow transform buffer
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

    recordMainRenderPass(scene, imageIndex);

    VkImageMemoryBarrier barrier{};
    barrier.sType     = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image                           = m_RenderContext.renderPasses.shadowPass.depthImage.image;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;


    vkCmdPipelineBarrier(m_Context.commandContext.commandBuffer, sourceStage, destinationStage, 0, 0,
                         nullptr, 0, nullptr, 1, &barrier);

    if(vkEndCommandBuffer(m_Context.commandContext.commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void VulkanRenderer::recordShadowPass(Scene& scene, uint32_t imageIndex) {
    ShadowPass& shadowPass = m_RenderContext.renderPasses.shadowPass;

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass  = shadowPass.renderPassContext.renderPass;
    renderPassInfo.framebuffer = shadowPass.depthFrameBuffer;

    // TODO store this in shadow pass struct ?
    VkExtent2D shadowExtent;
    shadowExtent.width  = shadowPass.shadowMapWidth;
    shadowExtent.height = shadowPass.shadowMapHeight;

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = shadowExtent;

    VkClearValue clearValue;
    clearValue.depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues    = &clearValue;

    vkCmdBeginRenderPass(m_Context.commandContext.commandBuffer,
                         &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(m_Context.commandContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      shadowPass.renderPassContext
                          .graphicsPipelines[shadowPass.renderPassContext.activePipelineIndex]);

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = shadowExtent.width;
    viewport.height   = shadowExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_Context.commandContext.commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = shadowExtent;
    vkCmdSetScissor(m_Context.commandContext.commandBuffer, 0, 1, &scissor);

    // Without Index Buffer
    // vkCmdDraw(commandBuffer, vertices.size(), 1, 0, 0);


    SceneTransform sceneTransform;

    sceneTransform.perspectiveTransform =
        getPerspectiveMatrix(m_RenderContext.renderSettings,
                             shadowPass.shadowMapWidth, shadowPass.shadowMapHeight);

    // one tutorial says openGL has different convention for Y coordinates in
    // clip space than vulkan, need to flip it
    sceneTransform.perspectiveTransform[1][1] *= -1;

    sceneTransform.cameraTransform = shadowPass.lightCamera.getCameraMatrix();

    // PushConstants would be more efficient for often changing small data buffers

    memcpy(m_RenderContext.renderPasses.shadowPass.transformBuffer.bufferMemoryMapping,
           &sceneTransform, sizeof(SceneTransform));


    // bind DescriptorSet 0 (Camera Transformations)
    vkCmdBindDescriptorSets(
        m_Context.commandContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        shadowPass.renderPassContext
            .pipelineLayouts[shadowPass.renderPassContext.activePipelineIndex],
        0, 1, &m_RenderContext.renderPasses.shadowPass.transformDescriptorSet, 0, nullptr);

    // render entities
    for(EntityId id : SceneView<ModelInstance, Transformation>(scene)) {
        auto* modelComponent     = scene.getComponent<ModelInstance>(id);
        auto* transformComponent = scene.getComponent<Transformation>(id);

        for(auto& meshPartIndex : scene.getModels()[modelComponent->modelID].meshPartIndices) {
            MeshPart     meshPart = scene.getMeshParts()[meshPartIndex];
            Mesh         mesh     = scene.getMeshes()[meshPart.meshIndex];
            VkBuffer     vertexBuffers[] = {mesh.vertexBuffer};
            VkDeviceSize offsets[]       = {0};

            vkCmdBindVertexBuffers(m_Context.commandContext.commandBuffer, 0, 1,
                                   vertexBuffers, offsets);

            vkCmdBindIndexBuffer(m_Context.commandContext.commandBuffer,
                                 mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            glm::mat4 transform = transformComponent->getMatrix();

            vkCmdPushConstants(m_Context.commandContext.commandBuffer,
                               shadowPass.renderPassContext
                                   .pipelineLayouts[shadowPass.renderPassContext.activePipelineIndex],
                               VK_SHADER_STAGE_VERTEX_BIT,
                               0,  // offset
                               sizeof(glm::mat4), &transform);

            vkCmdDrawIndexed(m_Context.commandContext.commandBuffer,
                             mesh.indicesCount, 1, 0, 0, 0);
        }
    }

    // render non-entity models
    for(auto& instance : scene.getInstances()) {
        Model     model          = scene.getModels()[instance.modelID];
        glm::mat4 transformation = instance.transformation;
        for(auto& meshPartIndex : model.meshPartIndices) {
            MeshPart     meshPart = scene.getMeshParts()[meshPartIndex];
            Mesh         mesh     = scene.getMeshes()[meshPart.meshIndex];
            VkBuffer     vertexBuffers[] = {mesh.vertexBuffer};
            VkDeviceSize offsets[]       = {0};

            vkCmdBindVertexBuffers(m_Context.commandContext.commandBuffer, 0, 1,
                                   vertexBuffers, offsets);

            vkCmdBindIndexBuffer(m_Context.commandContext.commandBuffer,
                                 mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdPushConstants(m_Context.commandContext.commandBuffer,
                               shadowPass.renderPassContext
                                   .pipelineLayouts[shadowPass.renderPassContext.activePipelineIndex],
                               VK_SHADER_STAGE_VERTEX_BIT,
                               0,  // offset
                               sizeof(glm::mat4), &transformation);

            vkCmdDrawIndexed(m_Context.commandContext.commandBuffer,
                             mesh.indicesCount, 1, 0, 0, 0);
        }
    }

    vkCmdEndRenderPass(m_Context.commandContext.commandBuffer);
}

void VulkanRenderer::recordMainRenderPass(Scene& scene, uint32_t imageIndex) {
    RenderPassContext& mainRenderPass = m_RenderContext.renderPasses.mainPass.renderPassContext;

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType      = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = mainRenderPass.renderPass;
    renderPassInfo.framebuffer =
        m_Context.swapchainContext.swapChainFramebuffers[imageIndex];

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_Context.swapchainContext.swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color        = {{1.0f, 1.0f, 1.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues    = clearValues.data();

    vkCmdBeginRenderPass(m_Context.commandContext.commandBuffer,
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
    vkCmdSetViewport(m_Context.commandContext.commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_Context.swapchainContext.swapChainExtent;
    vkCmdSetScissor(m_Context.commandContext.commandBuffer, 0, 1, &scissor);

    if (m_RenderContext.imguiData.visualizeShadowBuffer) {
        vkCmdBindPipeline(m_Context.commandContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          m_RenderContext.renderPasses.mainPass.visualizePipeline);

        vkCmdBindDescriptorSets(
            m_Context.commandContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_RenderContext.renderPasses.mainPass.visualizePipelineLayout, 0,
            1, &m_RenderContext.renderPasses.mainPass.visDepthDescriptorSet, 0, nullptr);

        vkCmdDraw(m_Context.commandContext.commandBuffer, 6, 1, 0, 0);

    } else {
        vkCmdBindPipeline(m_Context.commandContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          mainRenderPass.graphicsPipelines[mainRenderPass.activePipelineIndex]);


        // Without Index Buffer
        // vkCmdDraw(commandBuffer, vertices.size(), 1, 0, 0);

        // bind DescriptorSet 0 (Camera Transformations)
        vkCmdBindDescriptorSets(
            m_Context.commandContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mainRenderPass.pipelineLayouts[mainRenderPass.activePipelineIndex], 0,
            1, &m_RenderContext.renderPasses.mainPass.transformDescriptorSet, 0, nullptr);

        // TODO: from my understanding, this descriptor set only has to be bound
        //       once, but I got errors when doing so
        //        -> should make it possible for performance reasons
        //        -> use separate command buffer that is only updated when the graphics pipeline is changed

        // bind DescriptorSet 1 (Materials)
        vkCmdBindDescriptorSets(
            m_Context.commandContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mainRenderPass.pipelineLayouts[mainRenderPass.activePipelineIndex], 1,
            1, &m_RenderContext.renderPasses.mainPass.materialDescriptorSet, 0, nullptr);

        vkCmdBindDescriptorSets(
            m_Context.commandContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mainRenderPass.pipelineLayouts[mainRenderPass.activePipelineIndex], 2,
            1, &m_RenderContext.renderPasses.mainPass.depthDescriptorSet, 0, nullptr);


        // create PushConstant object and initialize with default values
        PushConstant pushConstant;
        pushConstant.transformation = glm::mat4(1);
        pushConstant.materialIndex  = 0;


        // TODO imgui and pipeline for quad to display texture
        // render entities
        for(EntityId id : SceneView<ModelInstance, Transformation>(scene)) {
            auto* modelComponent     = scene.getComponent<ModelInstance>(id);
            auto* transformComponent = scene.getComponent<Transformation>(id);

            // set transformation matrix of the model in the PushConstant
            pushConstant.transformation = transformComponent->getMatrix();

            for(auto& meshPartIndex : scene.getModels()[modelComponent->modelID].meshPartIndices) {
                MeshPart     meshPart = scene.getMeshParts()[meshPartIndex];
                Mesh         mesh     = scene.getMeshes()[meshPart.meshIndex];
                VkBuffer     vertexBuffers[] = {mesh.vertexBuffer};
                VkDeviceSize offsets[]       = {0};

                // set material index in the PushConstant
                pushConstant.materialIndex = meshPart.materialIndex;

                vkCmdBindVertexBuffers(m_Context.commandContext.commandBuffer,
                                       0, 1, vertexBuffers, offsets);

                vkCmdBindIndexBuffer(m_Context.commandContext.commandBuffer,
                                     mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

                /*
                 glm::mat4 transform = transformComponent->getMatrix();

    vkCmdPushConstants(m_Context.commandContext.commandBuffer,
          mainRenderPass.pipelineLayouts[mainRenderPass.activePipelineIndex],
          VK_SHADER_STAGE_VERTEX_BIT,
          0,  // offset
          sizeof(glm::mat4), &transform);

                */

                vkCmdPushConstants(
                    m_Context.commandContext.commandBuffer,
                    m_RenderContext.renderPasses.mainPass.renderPassContext
                        .pipelineLayouts[m_RenderContext.renderPasses.mainPass
                                             .renderPassContext.activePipelineIndex],
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,  // offset
                    sizeof(PushConstant), &pushConstant);

                vkCmdDrawIndexed(m_Context.commandContext.commandBuffer,
                                 mesh.indicesCount, 1, 0, 0, 0);
            }
        }

        // render non-entity models
        for(auto& instance : scene.getInstances()) {
            Model model = scene.getModels()[instance.modelID];

            // set transformation matrix of the model
            pushConstant.transformation = instance.transformation;

            for(auto& meshPartIndex : model.meshPartIndices) {
                MeshPart     meshPart = scene.getMeshParts()[meshPartIndex];
                Mesh         mesh     = scene.getMeshes()[meshPart.meshIndex];
                VkBuffer     vertexBuffers[] = {mesh.vertexBuffer};
                VkDeviceSize offsets[]       = {0};

                // set material index in the PushConstant
                pushConstant.materialIndex = meshPart.materialIndex;

                vkCmdBindVertexBuffers(m_Context.commandContext.commandBuffer,
                                       0, 1, vertexBuffers, offsets);

                vkCmdBindIndexBuffer(m_Context.commandContext.commandBuffer,
                                     mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

                /*
                 vkCmdPushConstants(m_Context.commandContext.commandBuffer,
          mainRenderPass.pipelineLayouts[mainRenderPass.activePipelineIndex],
          VK_SHADER_STAGE_VERTEX_BIT,
          0,  // offset
          sizeof(glm::mat4), &transformation);


                 */

                vkCmdPushConstants(
                    m_Context.commandContext.commandBuffer,
                    m_RenderContext.renderPasses.mainPass.renderPassContext
                        .pipelineLayouts[m_RenderContext.renderPasses.mainPass
                                             .renderPassContext.activePipelineIndex],
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,  // offset
                    sizeof(PushConstant), &pushConstant);

                vkCmdDrawIndexed(m_Context.commandContext.commandBuffer,
                                 mesh.indicesCount, 1, 0, 0, 0);
            }
        }
    }

    if(m_RenderContext.usesImgui) {
        // @IMGUI
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                        m_Context.commandContext.commandBuffer);
    }

    vkCmdEndRenderPass(m_Context.commandContext.commandBuffer);
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
    SceneTransform sceneTransform;

    sceneTransform.perspectiveTransform =
        getPerspectiveMatrix(m_RenderContext.renderSettings,
                             m_Context.swapchainContext.swapChainExtent.width,
                             m_Context.swapchainContext.swapChainExtent.height);

    // one tutorial says openGL has different convention for Y coordinates in
    // clip space than vulkan, need to flip it
    sceneTransform.perspectiveTransform[1][1] *= -1;

    sceneTransform.cameraTransform = scene.getCameraRef().getCameraMatrix();

    // PushConstants would be more efficient for often changing small data buffers

    memcpy(m_RenderContext.renderPasses.mainPass.transformBuffer.bufferMemoryMapping,
           &sceneTransform, sizeof(SceneTransform));
    // memcpy(scene.getUniformBufferMapping(), &sceneTransform, sizeof(sceneTransform));
}

void VulkanRenderer::recompileToSecondaryPipeline() {
    buildSecondaryGraphicsPipeline(m_Context,
                                   m_RenderContext.renderPasses.mainPass.renderPassContext);
    buildSecondaryGraphicsPipeline(m_Context,
                                   m_RenderContext.renderPasses.shadowPass.renderPassContext);
}

void VulkanRenderer::swapToSecondaryPipeline() {
    swapGraphicsPipeline(m_Context, m_RenderContext.renderPasses.mainPass.renderPassContext);
    swapGraphicsPipeline(m_Context, m_RenderContext.renderPasses.shadowPass.renderPassContext);
}
ApplicationVulkanContext VulkanRenderer::getContext() {
    return m_Context;
}
void VulkanRenderer::setRenderContext(RenderContext& renderContext) {
    m_RenderContext = renderContext;
}
