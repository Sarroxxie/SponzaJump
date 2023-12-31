#pragma once
#include <vulkan/vulkan_core.h>
#include "vulkan/ApplicationContext.h"
#include "tiny_gltf.h"
#include <vector>
#include <glm/mat4x4.hpp>

// in "rendering/host_device.h" is a copy of this called "MaterialDescription" for use on GPU
struct Material
{
    // these default values match the defaults of tinygltf
    alignas(16) glm::vec3 albedo              = glm::vec3(1, 1, 1);
    alignas(16) glm::vec3 aoRoughnessMetallic = glm::vec3(1, 1, 1);
    // these IDs reference textures in the texture data array inside Scene
    alignas(4) int albedoTextureID              = -1;
    alignas(4) int normalTextureID              = -1;
    alignas(4) int aoRoughnessMetallicTextureID = -1;
    // this is the unique identifier for new materials
    bool operator==(const Material& other) const {
        return albedo == other.albedo && aoRoughnessMetallic == other.aoRoughnessMetallic
               && albedoTextureID == other.albedoTextureID
               && normalTextureID == other.normalTextureID
               && aoRoughnessMetallicTextureID == other.aoRoughnessMetallicTextureID;
    }
};

struct Texture
{
    std::string           uri = "";
    VkImage               image;
    VkDeviceMemory        imageMemory;
    VkImageView           imageView;
    VkSampler             sampler;
    VkDescriptorImageInfo descriptorInfo;

    void cleanup(VulkanBaseContext& baseContext) {
        vkDestroySampler(baseContext.device, sampler, nullptr);
        vkDestroyImageView(baseContext.device, imageView, nullptr);

        vkDestroyImage(baseContext.device, image, nullptr);
        vkFreeMemory(baseContext.device, imageMemory, nullptr);
    }
};

struct CubeMap
{
    // vulkan specified order for the cubemap layers is as follows (see vulkan spec 16.5.4:
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap16.html#_cube_map_face_selection_and_transformations)
    //
    // layer number  ->   cubemap face
    //       0       ->    positive X
    //       1       ->    negative X
    //       2       ->    positive Y
    //       3       ->    negative Y
    //       4       ->    positive Z
    //       5       ->    negative Z
    std::array<std::string, 6> paths;
    VkImage                    image;
    VkDeviceMemory             imageMemory;
    VkImageView                imageView;
    VkSampler                  sampler;
    VkDescriptorImageInfo      descriptorInfo;

    void cleanup(VulkanBaseContext& baseContext) {
        vkDestroySampler(baseContext.device, sampler, nullptr);
        vkDestroyImageView(baseContext.device, imageView, nullptr);

        vkDestroyImage(baseContext.device, image, nullptr);
        vkFreeMemory(baseContext.device, imageMemory, nullptr);
    }
};

struct PointLight
{
    glm::vec3 position;
    glm::vec3 intensity;
    float radius = 0;
};

struct Mesh
{
    uint32_t verticesCount;
    uint32_t indicesCount;
    // radius of the bounding sphere around the Mesh -> will be used for frustum culling
    float radius;

    VkBuffer       vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;

    VkBuffer       indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

    void cleanup(VulkanBaseContext& baseContext) {
        vkDestroyBuffer(baseContext.device, indexBuffer, nullptr);
        vkFreeMemory(baseContext.device, indexBufferMemory, nullptr);

        vkDestroyBuffer(baseContext.device, vertexBuffer, nullptr);
        vkFreeMemory(baseContext.device, vertexBufferMemory, nullptr);
    }
};

struct MeshPart
{
    // references Mesh stored in the std::vector<Mesh> inside Scene
    int meshIndex;
    // references Material stored in the std::vector<Material> inside Scene
    int materialIndex;

    MeshPart()
        : meshIndex(-1)
        , materialIndex(-1) {}

    MeshPart(int meshIndex,int materialIndex)
        : meshIndex(meshIndex)
        , materialIndex(materialIndex) {}

    bool operator==(const MeshPart& other) const {
        return meshIndex == other.meshIndex && materialIndex == other.materialIndex;
    }
};

struct Model
{
    std::vector<int> meshPartIndices;
};

struct ModelComponent {
    int modelIndex;
};

class ModelInstance
{
  public:
    // TODO: rewrite parts of the transformation struct (to use quaternions and store the mat4)
    // glm::mat4 transformation = glm::mat4(1.0f);

    std::string name;
    int       modelID;

    glm::vec3 translation;
    glm::vec3 rotation;
    glm::vec3 scaling;

    glm::vec3 min;
    glm::vec3 max;

  public:
    ModelInstance(int modelID)
        : modelID(modelID) {}
};