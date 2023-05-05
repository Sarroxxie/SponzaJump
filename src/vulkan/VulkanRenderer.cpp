#include <stdexcept>
#include <array>
#include "VulkanRenderer.h"
#include "VulkanSetup.h"

#include <iostream>
#include <algorithm>


VulkanRenderer::VulkanRenderer(ApplicationContext &context)
        : m_Context(context) {
    createSyncObjects(context.baseContext);
}


void VulkanRenderer::cleanVulkanRessources() {
    vkDestroySemaphore(m_Context.baseContext.device, m_ImageAvailableSemaphore, nullptr);
    vkDestroySemaphore(m_Context.baseContext.device, m_RenderFinishedSemaphore, nullptr);
    vkDestroyFence(m_Context.baseContext.device, m_InFlightFence, nullptr);
}

void VulkanRenderer::render(Scene &scene) {
    if (!scene.hasObject()) {
        std::cout << "Scene needs objects to be rendered" << std::endl;
        return;
    }

    vkWaitForFences(m_Context.baseContext.device, 1, &m_InFlightFence, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_Context.baseContext.device, m_Context.swapchainContext.swapChain, UINT64_MAX, m_ImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || m_Context.window->wasResized()) {
        recreateSwapChain(m_Context);
        m_Context.window->setResized(false);
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(m_Context.baseContext.device, 1, &m_InFlightFence);

    vkResetCommandBuffer(m_Context.commandContext.commandBuffer, 0);

    recordCommandBuffer(m_Context, m_Context.renderContext, scene, imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_Context.commandContext.commandBuffer;

    VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_Context.baseContext.graphicsQueue, 1, &submitInfo, m_InFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_Context.swapchainContext.swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(m_Context.baseContext.presentQueue, &presentInfo);
}

void VulkanRenderer::recordCommandBuffer(ApplicationContext &appContext, RenderContext &renderContext, Scene &scene, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(appContext.commandContext.commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderContext.renderPass;
    renderPassInfo.framebuffer = appContext.swapchainContext.swapChainFramebuffers[imageIndex];

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = appContext.swapchainContext.swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(appContext.commandContext.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(appContext.commandContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      renderContext.graphicsPipelines[renderContext.activePipelineIndex]);




    /*
     *
    std::optional<VulkanMesh>& vulkanMesh = mesh->getVulkanMeshObject();

    if (!vulkanMesh.has_value()) {
        throw std::runtime_error("Mesh is not initialized for Vulkan Engine!");
    }

    if (!vulkanMesh.value().isValid()) {
        throw std::runtime_error("Mesh can not be rendered!");
    }

    VkBuffer vertexBuffers[] = {vulkanMesh.value().getVertexBufferHandle() };
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(commandBuffer, vulkanMesh.value().getIndexBufferHandle(), 0, VK_INDEX_TYPE_UINT32);

     */

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(appContext.swapchainContext.swapChainExtent.width);
    viewport.height = static_cast<float>(appContext.swapchainContext.swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(appContext.commandContext.commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = appContext.swapchainContext.swapChainExtent;
    vkCmdSetScissor(appContext.commandContext.commandBuffer, 0, 1, &scissor);

    // Without Index Buffer
    // vkCmdDraw(commandBuffer, vertices.size(), 1, 0, 0);

    /*
    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout,
                            0,
                            1,
                            &descriptorSets[currentFrame],
                            0,
                            nullptr);
    */

    auto sceneObjects = scene.getObjects();

    for (auto &object: sceneObjects) {
        VkBuffer vertexBuffers[] = { object.vertexBuffer };
        VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(appContext.commandContext.commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(appContext.commandContext.commandBuffer, object.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(appContext.commandContext.commandBuffer, object.indicesCount, 1, 0, 0, 0);
    }

    /*
    VkBuffer vertexBuffers[] = {sceneObjects[0].vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(appContext.commandContext.commandBuffer, 0, 1, vertexBuffersVec.data(), offsets);

    vkCmdBindIndexBuffer(appContext.commandContext.commandBuffer, sceneObjects[0].indexBuffer, 0, VK_INDEX_TYPE_UINT32);


    vkCmdDrawIndexed(appContext.commandContext.commandBuffer, 3, 1, 0, 0, 0);
*/
    vkCmdEndRenderPass(appContext.commandContext.commandBuffer);

    if (vkEndCommandBuffer(appContext.commandContext.commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void VulkanRenderer::createSyncObjects(VulkanBaseContext &baseContext) {
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

    if (vkCreateSemaphore(baseContext.device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(baseContext.device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(baseContext.device, &fenceInfo, nullptr, &m_InFlightFence) != VK_SUCCESS) {

        throw std::runtime_error("failed to create synchronization objects for a frame!");
    }

}

void VulkanRenderer::recompileToSecondaryPipeline() {
    buildSecondaryGraphicsPipeline(m_Context);
}

void VulkanRenderer::swapToSecondaryPipeline() {
    swapGraphicsPipeline(m_Context);
}

ApplicationContext VulkanRenderer::getContext() {
    return m_Context;
}
