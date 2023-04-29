#include <stdexcept>
#include <array>
#include "VulkanRenderer.h"


VulkanRenderer::VulkanRenderer(VulkanContext &context)
        : m_Context(context) {
    createSyncObjects(context);
}


void VulkanRenderer::cleanVulkanRessources() {
    vkDestroySemaphore(m_Context.device, m_ImageAvailableSemaphore, nullptr);
    vkDestroySemaphore(m_Context.device, m_RenderFinishedSemaphore, nullptr);
    vkDestroyFence(m_Context.device, m_InFlightFence, nullptr);
}

void VulkanRenderer::render() {
    vkWaitForFences(m_Context.device, 1, &m_InFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_Context.device, 1, &m_InFlightFence);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(m_Context.device, m_Context.swapChain, UINT64_MAX, m_ImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(m_Context.commandBuffer, 0);

    recordCommandBuffer(m_Context, imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_Context.commandBuffer;

    VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_Context.graphicsQueue, 1, &submitInfo, m_InFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_Context.swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(m_Context.presentQueue, &presentInfo);
}

void VulkanRenderer::recordCommandBuffer(VulkanContext &context, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(context.commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = context.renderPass;
    renderPassInfo.framebuffer = context.swapChainFramebuffers[imageIndex];

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = context.swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(context.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(context.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context.graphicsPipeline);


    VkBuffer vertexBuffers[] = {context.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(context.commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(context.commandBuffer, context.indexBuffer, 0, VK_INDEX_TYPE_UINT32);


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
    viewport.width = static_cast<float>(context.swapChainExtent.width);
    viewport.height = static_cast<float>(context.swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(context.commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = context.swapChainExtent;
    vkCmdSetScissor(context.commandBuffer, 0, 1, &scissor);

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

    vkCmdDrawIndexed(context.commandBuffer, 3, 1, 0, 0, 0);

    vkCmdEndRenderPass(context.commandBuffer);

    if (vkEndCommandBuffer(context.commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void VulkanRenderer::createSyncObjects(VulkanContext &context) {
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

    if (vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(context.device, &fenceInfo, nullptr, &m_InFlightFence) != VK_SUCCESS) {

        throw std::runtime_error("failed to create synchronization objects for a frame!");
    }

}
