#ifndef GRAPHICSPRAKTIKUM_RENDERSETUP_H
#define GRAPHICSPRAKTIKUM_RENDERSETUP_H

#include "vulkan/ApplicationContext.h"
#include "RenderContext.h"
#include "Shader.h"
#include "scene/Model.h"
#include "scene/Scene.h"


// ----- Sample Render Setups

RenderSetupDescription initializeSimpleSceneRenderContext(ApplicationVulkanContext& appContext,
                                                          RenderContext& renderContext,
                                                          Scene& scene);
// ---


void initializeRenderContext(ApplicationVulkanContext& appContext,
                             RenderContext&            renderContext,
                             const RenderSetupDescription& renderSetupDescription,
                             Scene& scene);

// ----- Shadow Pass


void initializeShadowPass(const ApplicationVulkanContext& appContext,
                          RenderContext&                  renderContext,
                          const RenderPassDescription& renderPassDescription);

void initializeShadowDepthBuffer(const ApplicationVulkanContext& appContext,
                                 ShadowPass&                     shadowPass,
                                 uint32_t                        width,
                                 uint32_t                        height,
                                 uint32_t                        index = 0);

void createShadowPassResources(const ApplicationVulkanContext& appContext,
                               RenderContext&                  renderContext);

void createShadowPassDescriptorSets(const ApplicationVulkanContext& appContext,
                                    RenderContext& renderContext);

void cleanShadowPass(const VulkanBaseContext& baseContext, const ShadowPass& shadowPass);

// -----


// TODO need this from merge ?
// void initializeRenderPassContext(const ApplicationVulkanContext &appContext,
// RenderContext &renderContext, const RenderSetupDescription &renderSetupDescription);


// TODO

/*

#define VK_CHECK_RESULT(f)																				\
{																										\
  VkResult res = (f);																					\
  if (res != VK_SUCCESS)																				\
  {																									\
    std::cout << "Fatal : VkResult is \"" << vks::tools::errorString(res) << "\"
in " << __FILE__ << " at line " << __LINE__ << "\n"; \
    assert(res == VK_SUCCESS);																		\
  }																									\
}
#endif
 */

// ----- Main Pass
void initializeMainRenderPass(const ApplicationVulkanContext& appContext,
                              RenderContext&                  renderContext,
                              const RenderPassDescription& renderPassDescription,
                              Scene& scene);

void createMainRenderPass(const ApplicationVulkanContext& appContext,
                          RenderContext&                  renderContext);

void createGeometryRenderPass(const ApplicationVulkanContext& appContext,
                           RenderContext&                  renderContext);

void createMainPassResources(const ApplicationVulkanContext& appContext,
                             RenderContext&                  renderContext,
                             Scene&                          scene);

void createMaterialsBuffer(const ApplicationVulkanContext& appContext,
                           RenderContext&                  renderContext,
                           Scene&                          scene);

void createMainPassDescriptorSetLayouts(const ApplicationVulkanContext& appContext,
                                        MainPass& mainPass,
                                        Scene&    scene);

void cleanMainPassDescriptorLayouts(const VulkanBaseContext& appContext,
                                    const MainPass&          mainPass);

void createMainPassDescriptorSets(const ApplicationVulkanContext& appContext,
                                  RenderContext&                  renderContext,
                                  Scene&                          scene);

void cleanMainPass(const VulkanBaseContext& baseContext, const MainPass& mainPass);

void cleanDeferredPass(const VulkanBaseContext& baseContext, const MainPass& mainPass);

void createDepthSampler(const ApplicationVulkanContext& appContext, MainPass& mainPass);

void createVisualizationPipeline(const ApplicationVulkanContext& appContext,
                                 const RenderContext&            renderContext,
                                 MainPass&                       mainPass);

void cleanVisualizationPipeline(const VulkanBaseContext& baseContext, const MainPass& mainPass);

void createSkyboxPipeline(const ApplicationVulkanContext& appContext,
                          const RenderContext&            renderContext,
                          MainPass&                       mainPass);

void cleanSkyboxPipeline(const VulkanBaseContext& baseContext, const MainPass& mainPass);

void createGeometryPassPipeline(const ApplicationVulkanContext& appContext,
                                const RenderContext&            renderContext,
                                const RenderPassDescription& renderPassDescription,
                                MainPass&                       mainPass);

void cleanGeometryPassPipeline(const VulkanBaseContext& baseContext, const MainPass& mainPass);

// -----


// ----- creation utils
void createGraphicsPipeline(const ApplicationVulkanContext& appContext,
                            RenderPassContext&              renderContext,
                            VkPipelineLayout&               pipelineLayout,
                            VkPipeline&                     graphicsPipeline,
                            const RenderPassDescription& renderSetupDescription,
                            std::vector<VkDescriptorSetLayout>& layouts,
                            bool useFragmentShader = true);

void createBlankAttachment(const ApplicationVulkanContext& context,
                           VkAttachmentDescription&        attachment,
                           VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT,
                           VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                           VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED);

void createDeferredAttachment(const ApplicationVulkanContext& context,
                              VkFormat                        format,
                              VkImageUsageFlagBits            usage,
                              ImageResources&                 imageResources);

void createDescriptorSetLayout(const VulkanBaseContext& context,
                               VkDescriptorSetLayout&   layout,
                               const std::vector<VkDescriptorSetLayoutBinding>& bindings);

VkDescriptorSetLayoutBinding createLayoutBinding(uint32_t binding,
                                                 uint32_t descriptorCount,
                                                 VkDescriptorType descriptorType,
                                                 VkShaderStageFlags shaderStageFlags);

VkPushConstantRange createPushConstantRange(uint32_t offset, uint32_t size, VkShaderStageFlags flags);

void createDescriptorPool(const VulkanBaseContext& baseContext, RenderContext& renderContext);

void createBufferResources(const ApplicationVulkanContext& appContext,
                           VkDeviceSize                    bufferSize,
                           BufferResources&                bufferResources);
// ---


// ----- clean up
void cleanupRenderContext(const VulkanBaseContext& baseContext, RenderContext& renderContext);

void cleanupRenderPassContext(const VulkanBaseContext& baseContext,
                              const RenderPassContext& renderPassContext);

void cleanupImGuiContext(const VulkanBaseContext& baseContext, RenderContext& renderContext);
// ---

void createFrameBuffers(ApplicationVulkanContext& appContext, RenderContext& renderContext);

// ----- pipeline rebuild
void buildSecondaryGraphicsPipeline(const ApplicationVulkanContext& appContext,
                                    RenderPassContext&              renderPass,
                                    bool useFragShader = true);

bool swapGraphicsPipeline(const ApplicationVulkanContext& appContext,
                          RenderPassContext&              renderPass);
// ---


// ----- Imgui
void initializeImGui(const ApplicationVulkanContext& appContext, RenderContext& renderContext);
// ---
#endif  // GRAPHICSPRAKTIKUM_RENDERSETUP_H
