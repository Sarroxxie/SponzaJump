#pragma once
#include <tiny_gltf.h>
#include "scene/Model.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

struct VertexObj
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec4 tangents;
    glm::vec2 texCoord;
};

void createMeshBuffers(VulkanBaseContext         context,
                       CommandContext            commandContext,
                       std::vector<Vertex>       vertices,
                       std::vector<unsigned int> indices,
                       Mesh&                     mesh);
void createSampleVertexBuffer(VulkanBaseContext&  context,
                               CommandContext&     commandContext,
                               std::vector<Vertex> vertices,
                               Mesh&               mesh);
void createSampleIndexBuffer(VulkanBaseContext&        baseContext,
                              CommandContext&           commandContext,
                              std::vector<unsigned int> indices,
                              Mesh&                     mesh);

struct ModelLoadingOffsets
{
    int meshesOffset    = 0;
    int meshPartsOffset = 0;
    int texturesOffset  = 0;
    int materialsOffset = 0;
    int modelsOffset    = 0;
    int instancesOffset = 0;
};

class ModelLoader
{
  private:
    struct MeshLookup
    {
        tinygltf::Primitive primitive;

        // index in the "meshes" vector
        int meshIndex;

        MeshLookup(tinygltf::Primitive primitive, int meshIndex)
            : primitive(primitive)
            , meshIndex(meshIndex) {}
    };

  public:
    bool loadModel(const std::string&  filename,
                   ModelLoadingOffsets offsets,
                   VulkanBaseContext   context,
                   CommandContext      commandContext);

    ModelLoadingOffsets offsets;

    // stores per node transformation -> will be used to create instances from the models
    std::vector<Mesh>          meshes;
    std::vector<MeshPart>      meshParts;
    std::vector<Texture>       textures;
    std::vector<Material>      materials;
    std::vector<Model>         models;
    std::vector<ModelInstance> instances;

  private:
    glm::mat4 getTransform(tinygltf::Node node);
    Material  createMaterial(tinygltf::Material&             gltfMaterial,
                             int                             texturesOffset,
                             std::vector<tinygltf::Texture>& gltfTextures,
                             std::vector<tinygltf::Image>&   gltfImages);
    Mesh      createMesh(tinygltf::Primitive&              primitive,
                         std::vector<tinygltf::Accessor>   accessors,
                         std::vector<tinygltf::BufferView> bufferViews,
                         std::vector<tinygltf::Buffer>     buffers,
                         VulkanBaseContext                 context,
                         CommandContext                    commandContext);
    Texture   createTexture(std::string uri);
    int       findGeometryData(tinygltf::Primitive& primitive);

    std::vector<MeshLookup> meshLookups;
};