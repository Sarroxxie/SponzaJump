#pragma once
#include "scene/RenderableObject.h"
#include "tiny_gltf.h"
#include <vector>
#include <glm/mat4x4.hpp>

// in "rendering/host_device.h" is a copy of this called "MaterialDescription" for use on GPU
struct Material
{
    // these default values match the defaults of tinygltf
    glm::vec3 albedo              = glm::vec3(1, 1, 1);
    glm::vec3 aoRoughnessMetallic = glm::vec3(1, 1, 1);
    // these IDs reference textures in the texture data array inside Scene
    int         albedoTextureID              = -1;
    int         normalTextureID              = -1;
    int         aoRoughnessMetallicTextureID = -1;
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
    std::string uri = "";
    // TODO: needs handles to the GPU references
};

struct Mesh
{
    uint32_t verticesCount;
    uint32_t indicesCount;
    // radius of the bounding sphere around the Mesh -> will be used for frustum culling
    float radius;

    VkBuffer       vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkBuffer       indexBuffer;
    VkDeviceMemory indexBufferMemory;

    void cleanup(VulkanBaseContext& baseContext) {
        vkDestroyBuffer(baseContext.device, indexBuffer, nullptr);
        vkFreeMemory(baseContext.device, indexBufferMemory, nullptr);

        vkDestroyBuffer(baseContext.device, vertexBuffer, nullptr);
        vkFreeMemory(baseContext.device, vertexBufferMemory, nullptr);
    };
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

class ModelInstance
{
  public:
    // TODO: rewrite parts of the transformation struct (to use quaternions and store the mat4)
    glm::mat4 transformation = glm::mat4(1.0f);
    int       modelID;

  public:
    ModelInstance(int modelID)
        : modelID(modelID) {}
};