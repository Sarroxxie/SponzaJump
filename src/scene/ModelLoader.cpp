#include "ModelLoader.h"
#include <glm/gtx/quaternion.hpp>
#include <functional>

// byte size of float in the glTF2.0 file
constexpr int FLOAT_SIZE            = 4;
constexpr int COMPONENT_TYPE_UBYTE  = 5121;
constexpr int COMPONENT_TYPE_USHORT = 5123;
constexpr int COMPONENT_TYPE_UINT   = 5125;

/**
 * TODO: keep track of uploaded textures somewhere in a map (uri + TextureStruct)
 *       -> probably create a class to handle all model loading
 *       -> will need to create a proper desriptor for objects and textures so the data can get stored
 *       -> use tinygltf only for initial loading, scrap the "tinygltf::model" after loading
 *       -> maybe name class "sceneResourceManager"
 */
bool imageLoaderFunction(tinygltf::Image*     image,
                         const int            image_idx,
                         std::string*         err,
                         std::string*         warn,
                         int                  req_width,
                         int                  req_height,
                         const unsigned char* bytes,
                         int                  size,
                         void*                user_data) {
    // TODO:
    // 1. check if the uri is already registered (image->uri is already set at this point)

    // 2. use stbi_load_from_memory() -> use the memory content from "bytes"

    // 3. upload the image to the GPU and register it (with URI as identifier)
    // -> same structure as noted in the todo above

    // 4. use stbi_image_free() to free image from CPU RAM

    // tinygltf::LoadImageData(image, image_idx, err, warn, req_width, req_height, bytes, size, user_data);
    return true;
}

/*
 * Creates a glm::vec2 from raw byte data. This function assumes that the OS
 * uses small-endians. Reads a total of 8 bytes at the specified address.
 */
glm::vec2 bytesToVec2(unsigned char* address) {
    glm::vec2 out(0);
    memcpy(&out.r, address, sizeof(float));
    memcpy(&out.g, address + 4, sizeof(float));
    return out;
}

/*
 * Creates a glm::vec3 from raw byte data. This function assumes that the OS
 * uses small-endians. Reads a total of 12 bytes at the specified address.
 */
glm::vec3 bytesToVec3(unsigned char* address) {
    glm::vec3 out(0);
    memcpy(&out.r, address, sizeof(float));
    memcpy(&out.g, address + 4, sizeof(float));
    memcpy(&out.b, address + 8, sizeof(float));
    return out;
}

/*
 * Creates a glm::vec4 from raw byte data. This function assumes that the OS
 * uses small-endians. Reads a total of 16 bytes at the specified address.
 */
glm::vec4 bytesToVec4(unsigned char* address) {
    glm::vec4 out(0);
    memcpy(&out.r, address, sizeof(float));
    memcpy(&out.g, address + 4, sizeof(float));
    memcpy(&out.b, address + 8, sizeof(float));
    memcpy(&out.a, address + 12, sizeof(float));
    return out;
}

/*
 * Reads an unsigned byte from 1 bytes at the specified address and casts it to
 * an unsigned int.
 */
unsigned int bytesFromUnsignedCharToUnsignedInt(unsigned char* address) {
    unsigned char out;
    memcpy(&out, address, sizeof(unsigned char));
    return (unsigned int)out;
}

/*
 * Reads an unsigned short from 2 bytes at the specified address and casts it to
 * an unsigned int.
 */
unsigned int bytesFromUnsignedShortToUnsignedInt(unsigned char* address) {
    unsigned short out;
    memcpy(&out, address, sizeof(unsigned short));
    return (unsigned int)out;
}

/*
 * Reads an unsigned int from 4 bytes at the specified address.
 */
unsigned int bytesToUnsignedInt(unsigned char* address) {
    unsigned int out;
    memcpy(&out, address, sizeof(unsigned int));
    return out;
}

/*
 * Creates vertex and index buffers on the GPU and stores a reference to the
 * handle in the mesh.
 */
void createMeshBuffers(VulkanBaseContext         context,
                       CommandContext            commandContext,
                       std::vector<Vertex>       vertices,
                       std::vector<unsigned int> indices,
                       Mesh&                     mesh) {
    createSampleVertexBuffer(context, commandContext, vertices, mesh);
    createSampleIndexBuffer(context, commandContext, indices, mesh);
}

/*
 * Creates a vertex buffer from the specified vertices vector and uploads it
 * to GPU. ALso stores a reference to the handle in the mesh.
 */
void createSampleVertexBuffer(VulkanBaseContext&  context,
                               CommandContext&     commandContext,
                               std::vector<Vertex> vertices,
                               Mesh&               mesh) {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    //void* data = (void*)vertices.data();
    void* data;
    vkMapMemory(context.device, stagingBufferMemory, 0, bufferSize, 0, &data);

    // We use Host Coherent Memory to make sure data is synchronized, could also manually flush Memory Ranges
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(context.device, stagingBufferMemory);

    createBuffer(context, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh.vertexBuffer,
                 mesh.vertexBufferMemory);

    copyBuffer(context, commandContext, stagingBuffer, mesh.vertexBuffer, bufferSize);

    vkDestroyBuffer(context.device, stagingBuffer, nullptr);
    vkFreeMemory(context.device, stagingBufferMemory, nullptr);
}

/*
 * Creates an index buffer from the specified indices vector and uploads it
 * to GPU. ALso stores a reference to the handle in the mesh.
 */
void createSampleIndexBuffer(VulkanBaseContext&        baseContext,
                              CommandContext&           commandContext,
                              std::vector<unsigned int> indices,
                              Mesh&                     mesh) {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(baseContext, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    //void* data = (void*)indices.data();
    void* data;
    vkMapMemory(baseContext.device, stagingBufferMemory, 0, bufferSize, 0, &data);

    // We use Host Coherent Memory to make sure data is synchronized, could also manually flush Memory Ranges
    memcpy(data, indices.data(), (size_t)bufferSize);
    vkUnmapMemory(baseContext.device, stagingBufferMemory);

    createBuffer(baseContext, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh.indexBuffer,
                 mesh.indexBufferMemory);

    copyBuffer(baseContext, commandContext, stagingBuffer, mesh.indexBuffer, bufferSize);

    vkDestroyBuffer(baseContext.device, stagingBuffer, nullptr);
    vkFreeMemory(baseContext.device, stagingBufferMemory, nullptr);
}

/*
 * Loads model from glTF2.0 file and converts everything to the internally used
 * format. Buffers get created for vertices, indices, materials and textures and
 * are uploaded to the GPU.
 */
bool ModelLoader::loadModel(const std::string&  filename,
                            ModelLoadingOffsets offsets,
                            VulkanBaseContext  context,
                            CommandContext     commandContext) {
    tinygltf::Model    gltfModel;
    tinygltf::TinyGLTF loader;
    std::string        errors;
    std::string        warnings;
    bool               success;

    // set custom image loading function
    loader.SetImageLoader(imageLoaderFunction, nullptr);
    success = loader.LoadASCIIFromFile(&gltfModel, &errors, &warnings, filename);

    if(!warnings.empty()) {
        printf("Warn: %s\n", warnings.c_str());
    }

    if(!errors.empty()) {
        printf("Err: %s\n", errors.c_str());
    }

    if(!success) {
        printf("Failed to parse glTF\n");
        return false;
    }

    this->offsets = offsets;

    // 1. create Meshes
    for(auto& gltfMesh : gltfModel.meshes) {
        // a "Mesh" from the glTF file is called a "Model" in our program
        Model model;
        for(auto& primitive : gltfMesh.primitives) {
            int meshPartIndex = -1;
            int meshIndex     = findGeometryData(primitive);
            if(meshIndex < 0) {
                // Mesh not yet created, so create a new one alongside a MeshPart
                Mesh mesh = createMesh(primitive, gltfModel.accessors, gltfModel.bufferViews,
                                       gltfModel.buffers, context, commandContext);
                meshes.push_back(mesh);
                meshParts.push_back(MeshPart(meshes.size() - 1 + offsets.meshesOffset,
                                             primitive.material + offsets.materialsOffset));
                model.meshPartIndices.push_back((int)meshParts.size() - 1
                                                + offsets.meshPartsOffset);
                meshLookups.push_back(MeshLookup(primitive, meshes.size() - 1));
            } else {
                // Mesh exists, so we need to see if there exists a MeshPart
                // pointing at that Mesh and sharing the material index
                bool meshPartExists = false;
                for(int i = 0; i < meshParts.size(); i++) {
                    if(meshParts[i].meshIndex == (meshIndex + offsets.meshesOffset)
                       && meshParts[i].materialIndex == (primitive.material + offsets.materialsOffset)) {
                        meshPartExists = true;
                        model.meshPartIndices.push_back(i + offsets.meshPartsOffset);
                        break;
                    }
                }
                if(!meshPartExists) {
                    std::cout << "in\n";
                    // create the MeshPart that points to the found Mesh and
                    // the material of the primitive
                    meshParts.push_back(MeshPart(meshIndex + offsets.meshesOffset,
                                                 primitive.material + offsets.materialsOffset));
                    model.meshPartIndices.push_back(meshParts.size() - 1
                                                    + offsets.meshPartsOffset);
                }
            }
        }
        models.push_back(model);
    }
    // 2. create Materials
    for(auto& gltfMaterial : gltfModel.materials) {
        materials.push_back(
            createMaterial(gltfMaterial, gltfModel.textures, gltfModel.images));
    }

    // 3. create ModelInstances (from list of Nodes)
    for(auto& node : gltfModel.nodes) {
        ModelInstance instance(node.mesh + offsets.modelsOffset);
        glm::mat4     transform = getTransform(node);
        instance.transformation = transform;
        instances.push_back(instance);
    }
    return true;
}

/*
 * Calculates transformation matrix from scale, rotation and translation of a Node.
 */
glm::mat4 ModelLoader::getTransform(tinygltf::Node node) {
    glm::mat4 transform = glm::mat4(1.0f);

    if(node.scale.size() != 0) {
        transform *= glm::scale(glm::mat4(1),
                                glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
    }
    if(node.rotation.size() != 0) {
        glm::quat quat(node.rotation[0], node.rotation[1], node.rotation[2],
                       node.rotation[3]);
        transform *= glm::toMat4(quat);
    }
    if(node.translation.size() != 0) {
        transform *= glm::translate(glm::mat4(1), glm::vec3(node.translation[0],
                                                            node.translation[1],
                                                            node.translation[2]));
    }
    return transform;
}

/*
 * Creates a material by loading all the needed textures and setting IDs in the
 * material to point at them.
 */
Material ModelLoader::createMaterial(tinygltf::Material& gltfMaterial,
                                     std::vector<tinygltf::Texture>& gltfTextures,
                                     std::vector<tinygltf::Image>& gltfImages) {
    Material material;
    material.name = gltfMaterial.name;
    if(gltfMaterial.pbrMetallicRoughness.baseColorTexture.index != -1) {
        textures.push_back(createTexture(
            gltfImages
                [gltfTextures[gltfMaterial.pbrMetallicRoughness.baseColorTexture.index]
                     .source]
                    .uri));
        material.albedoTextureID = textures.size() - 1;
    } else {
        material.albedo =
            glm::vec3(gltfMaterial.pbrMetallicRoughness.baseColorFactor[0],
                      gltfMaterial.pbrMetallicRoughness.baseColorFactor[1],
                      gltfMaterial.pbrMetallicRoughness.baseColorFactor[2]);
    }
    if(gltfMaterial.normalTexture.index != -1) {
        textures.push_back(createTexture(
            gltfImages[gltfTextures[gltfMaterial.normalTexture.index].source].uri));
        material.normalTextureID = textures.size() - 1;
    }
    if(gltfMaterial.occlusionTexture.index != -1) {
        textures.push_back(createTexture(
            gltfImages[gltfTextures[gltfMaterial.occlusionTexture.index].source].uri));
        material.aoRoughnessMetallicTextureID = textures.size() - 1;
    } else {
        material.aoRoughnessMetallic.r = 1;
    }
    if(gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index != -1
       && material.aoRoughnessMetallicTextureID == -1) {
        textures.emplace_back(createTexture(
            gltfImages[gltfTextures[gltfMaterial.pbrMetallicRoughness
                                        .metallicRoughnessTexture.index]
                           .source]
                .uri));
        material.aoRoughnessMetallicTextureID = textures.size() - 1;
    } else {
        material.aoRoughnessMetallic.g = gltfMaterial.pbrMetallicRoughness.roughnessFactor;
        material.aoRoughnessMetallic.b = gltfMaterial.pbrMetallicRoughness.metallicFactor;
    }
    return material;
}

/*
 * Gets the geometry data from the primitive and stores it in a Mesh.
 */
Mesh ModelLoader::createMesh(tinygltf::Primitive&              primitive,
                             std::vector<tinygltf::Accessor>   accessors,
                             std::vector<tinygltf::BufferView> bufferViews,
                             std::vector<tinygltf::Buffer>     buffers,
                             VulkanBaseContext                 context,
                             CommandContext                    commandContext) {
    Mesh                      out;
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;

    tinygltf::Accessor position = accessors[primitive.attributes.at("POSITION")];
    tinygltf::Accessor normal   = accessors[primitive.attributes.at("NORMAL")];
    tinygltf::Accessor tangents = accessors[primitive.attributes.at("TANGENT")];
    tinygltf::Accessor texCoord0 = accessors[primitive.attributes.at("TEXCOORD_0")];

    // every vertex has to have every attribute
    assert(position.count == normal.count && position.count == tangents.count
           && position.count == texCoord0.count);

    // convert vertices into internal format
    for(int i = 0; i < position.count; i++) {
        // last number from the multiplication is the number of components in the vector
        Vertex vert;
        vert.pos = bytesToVec3(
            &buffers[bufferViews[position.bufferView].buffer]
                 .data[bufferViews[position.bufferView].byteOffset + i * FLOAT_SIZE * 3]);
        vert.nrm = bytesToVec3(
            &buffers[bufferViews[normal.bufferView].buffer]
                 .data[bufferViews[normal.bufferView].byteOffset + i * FLOAT_SIZE * 3]);
        vert.tangents = bytesToVec4(
            &buffers[bufferViews[tangents.bufferView].buffer]
                 .data[bufferViews[tangents.bufferView].byteOffset + i * FLOAT_SIZE * 4]);
        vert.texCoord = bytesToVec2(
            &buffers[bufferViews[texCoord0.bufferView].buffer]
                 .data[bufferViews[texCoord0.bufferView].byteOffset + i * FLOAT_SIZE * 2]);
        vertices.push_back(vert);
    }

    // default parsing function (if indices are stored as byte)
    auto function = bytesFromUnsignedCharToUnsignedInt;
    // size of one index in bytes
    int indexStride = 1;

    // choose parsing function based on the numeric type that the index is stored as
    if(accessors[primitive.indices].componentType == COMPONENT_TYPE_UINT) {
        function    = bytesToUnsignedInt;
        indexStride = 4;
    } else if(accessors[primitive.indices].componentType == COMPONENT_TYPE_USHORT) {
        function    = bytesFromUnsignedShortToUnsignedInt;
        indexStride = 2;
    }

    // convert indices into internal format
    int componentType = accessors[primitive.indices].componentType;
    for(int i = 0; i < accessors[primitive.indices].count; i++) {
        unsigned int index =
            function(&buffers[bufferViews[primitive.indices].buffer]
                         .data[bufferViews[primitive.indices].byteOffset + i * indexStride]);
        indices.push_back(index);
    }

    out.verticesCount = vertices.size();
    out.indicesCount  = indices.size();
    createMeshBuffers(context, commandContext, vertices, indices, out);
    // TODO: calculate Mesh.radius for later frustum culling
    return out;
}

// TODO: implement this -> will need vulkan tutorial for it
Texture ModelLoader::createTexture(std::string uri) {
    return Texture();
}

/*
 * Returns the index of the geometry data from the "meshes" array if the
 * geometry of the primitive already exists, else -1.
 */
int ModelLoader::findGeometryData(tinygltf::Primitive& primitive) {
    for(auto& meshLookup : meshLookups) {
        if(primitive.indices != meshLookup.primitive.indices)
            continue;
        bool attributesMatch = true;
        if(primitive.attributes.size() == meshLookup.primitive.attributes.size()) {
            for(auto& attribute : primitive.attributes) {
                // if key is not found
                if(meshLookup.primitive.attributes.count(attribute.first) == 0) {
                    attributesMatch = false;
                    break;
                }
                // if attribute value does not match
                if(attribute.second
                   != meshLookup.primitive.attributes.at(attribute.first)) {
                    attributesMatch = false;
                    break;
                }
            }
            if(attributesMatch)
                return meshLookup.meshIndex;
        }
    }
    return -1;
}