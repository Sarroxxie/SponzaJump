#pragma once
#include "scene/RenderableObject.h"
#include "tiny_gltf.h"
#include <vector>

// in "rendering/host_device.h" is a copy of this called "MaterialDescription" for use on GPU
struct Material
{
    std::string name                         = "default";
    glm::vec3   albedo                       = glm::vec3(1, 0, 0);
    glm::vec3   aoRoughnessMetallic          = glm::vec3(1, 1, 0);
    // these IDs reference textures in the texture data array inside Scene
    int         albedoTextureID              = -1;
    int         normalTextureID              = -1;
    int         aoRoughnessMetallicTextureID = -1;

    // this is the unique identifier for new materials
    bool operator==(const Material& other) const {
        return albedo == other.albedo && aoRoughnessMetallic == other.aoRoughnessMetallic
               && albedoTextureID == other.albedoTextureID
               && normalTextureID == other.normalTextureID
               && aoRoughnessMetallicTextureID == other.aoRoughnessMetallicTextureID
               && name == other.name;
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
    // TODO: rewrite parts of the transformation struct (to use quaternions and store the mat4)
    Transformation transformation;
    Model*         model;

    public:
    ModelInstance(Model* model) : model(model) {}
};

// TODO: all of the following will be contained in the Scene, so the Scene itself is responsible for
//       keeping track of all the renerable objects, materials and textures
//
// 1. When a model is loaded, iterate over the MeshParts (I think they are called "primitives" in glTF)
//
// 2. per MeshPart from model loading, create a MeshPart struct and create the
//    needed vertex and index buffers
//
// 3. per MeshPart from model loading, create a Material struct each time a new one occurs
//     -> need an array on CPU to keep track of the already created materials
//     -> add reference to the material struct to MeshPart
//
// 4. per new Material found while model loading, register the needed textures and
//    upload them to the GPU
//     -> need an array on CPU to keep track of the already uploaded textures
//     -> load via stbi, create ImageView and so on (maybe give option for mipmaps)
// 
// 5. once as initialization, create the buffer with all the Materials on the GPU
// 
// 6. once as initialization, create the buffer with all the Textures on the GPU
//     -> see line 143 in "hello_vulkan.cpp" in other project for reference
//     -> need to store VkDescriptorImageInfo when uploading the Texture to GPU
//
// 7. in the render loop, when rendering a ModelInstance, update the PushConstant.transformation
//    (from ModelInstance.transformation)
//
// 8. in the render loop, when rendering a MeshPart, update the PushConstant.materialIndex
//    (from MeshPart.material)
//
// 9. everything will need a "destroy" functionality, when the scene is destroyed
//     -> in "Scene.cleanup" method


// The plan is to load all models once at the creation of a new scene and there
// will be no need to further modify the materials and textures buffers while
// running the renderer. Might have to get creative when running out of VRAM,
// but that would be a far larger problem

// note: changing materials (like color changing over time) will need additional
// functionality and maybe will just operate on another graphics pipeline and won't
// access the material buffer from GPU, but will need uniforms for every draw call