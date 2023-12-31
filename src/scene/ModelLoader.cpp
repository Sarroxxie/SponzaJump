#include "ModelLoader.h"
#include "vulkan/VulkanUtils.h"
#include <glm/gtx/quaternion.hpp>
#include <functional>

// byte size of float in the glTF2.0 file
constexpr int FLOAT_SIZE            = 4;
constexpr int COMPONENT_TYPE_UBYTE  = 5121;
constexpr int COMPONENT_TYPE_USHORT = 5123;
constexpr int COMPONENT_TYPE_UINT   = 5125;

constexpr char* TEXTURES_DIRECTORY_NAME      = "textures/";
constexpr int   TEXTURES_DIRECTORY_NAME_SIZE = 9;
constexpr char* ASSETS_DIRECTORY_PATH        = "res/assets/";

/*
 * As image loading is done inside a custom function later, this is only needed
 * to ensure that the path to the image gets set correctly for later extraction.
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
                            VulkanBaseContext   context,
                            CommandContext      commandContext) {
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

    // 1. create Models
    for(auto& gltfMesh : gltfModel.meshes) {
        // a "Mesh" from the glTF file is called a "Model" in our program
        Model model = createModelFromMesh(gltfModel, gltfMesh, context, commandContext);
        models.push_back(model);
    }

    // 2. create Materials
    for(auto& gltfMaterial : gltfModel.materials) {
        Material material = createMaterial(gltfMaterial, offsets.texturesOffset,
                                           gltfModel.textures, gltfModel.images,
                                           context, commandContext);
        materials.push_back(material);
    }

    // 3. create ModelInstances and PointLights (from list of Nodes)
    for(auto& node : gltfModel.nodes) {
        if(node.mesh != -1) {
            // node references a mesh
            ModelInstance instance = nodeToInstance(gltfModel, node);
            instances.push_back(instance);
        } else {
            // node references a light source (probably)
            PointLight pointLight;
            if(nodeToLight(gltfModel, node, pointLight)) {
                lights.push_back(pointLight);
            }
        }
    }
    return true;
}

/*
 * Takes a mesh out of the glTF Mesh and transforms it into the internally used
 * Model. This function checks if the data used by this glTF Mesh is already
 * converted into the internal format and only sets the reference in this case.
 * If the data is not yet created, it will create Vertex and Index Buffers so
 * the Model can be used for GPU rendering later.
 */
Model ModelLoader::createModelFromMesh(tinygltf::Model&  gltfModel,
                                       tinygltf::Mesh&   gltfMesh,
                                       VulkanBaseContext context,
                                       CommandContext    commandContext) {
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
            model.meshPartIndices.push_back((int)meshParts.size() - 1 + offsets.meshPartsOffset);
            meshLookups.push_back(MeshLookup(primitive, meshes.size() - 1));
        } else {
            // Mesh exists, so we need to see if there exists a MeshPart
            // pointing at that Mesh and sharing the material index
            bool meshPartExists = false;
            for(int i = 0; i < meshParts.size(); i++) {
                if(meshParts[i].meshIndex == (meshIndex + offsets.meshesOffset)
                   && meshParts[i].materialIndex
                          == (primitive.material + offsets.materialsOffset)) {
                    meshPartExists = true;
                    model.meshPartIndices.push_back(i + offsets.meshPartsOffset);
                    break;
                }
            }
            if(!meshPartExists) {
                // create the MeshPart that points to the found Mesh and
                // the material of the primitive
                meshParts.push_back(MeshPart(meshIndex + offsets.meshesOffset,
                                             primitive.material + offsets.materialsOffset));
                model.meshPartIndices.push_back(meshParts.size() - 1 + offsets.meshPartsOffset);
            }
        }
    }
    return model;
}

/*
 * Checks if a node references a light source.
 * If the node is a light source, this function returns true and writes its
 * contents into the paramter "pointLight". If the node is not a light source,
 * this function returns false and does not modify anything.
 */
bool ModelLoader::nodeToLight(tinygltf::Model& gltfModel, tinygltf::Node& node, PointLight& pointLight) {
    const std::string extensionName = "KHR_lights_punctual";
    // TODO: find exact conversion factor
    //       I tried to match the look of the light to its blender equivalent and this factor seemed to be ok
    const float INTENSITY_FACTOR = 1.0 / 5500.0;

    for(auto entry : node.extensions) {
        // if the node contains the "KHR_lights_punctual"-extension, it is considered a light source
        if(entry.first == extensionName) {
            int lightID           = entry.second.Get("light").GetNumberAsInt();
            tinygltf::Light light = gltfModel.lights[lightID];
            // converting to something usable by the shader
            float intensity = light.intensity * INTENSITY_FACTOR;
            pointLight.intensity =
                glm::vec3(light.color[0] * intensity,
                          light.color[1] * intensity,
                          light.color[2] * intensity);
            // TODO: find a good way to calculate radius from intensity
            //        -> this calculation seems to yield pretty acceptable results
            pointLight.radius = glm::sqrt(intensity) * 2.3;
            if(node.translation.size() == 3) {
                pointLight.position = glm::vec3(node.translation[0], node.translation[1],
                                                node.translation[2]);
            } else {
                pointLight.position = glm::vec3(0);
            }
            return true;
        }
    }
    return false;
}

/*
* Creates a ModelInstance from the data inside a node of the glTF model.
*/
ModelInstance ModelLoader::nodeToInstance(tinygltf::Model& gltfModel, tinygltf::Node& node) {
    ModelInstance instance(node.mesh + offsets.modelsOffset);

    instance.name = node.name;

    if(node.translation.size() == 3) {
        instance.translation.x = node.translation[0];
        instance.translation.y = node.translation[1];
        instance.translation.z = node.translation[2];
    } else {
        instance.translation = glm::vec3(0);
    }

    if(node.scale.size() == 3) {
        instance.scaling.x = node.scale[0];
        instance.scaling.y = node.scale[1];
        instance.scaling.z = node.scale[2];
    } else {
        instance.scaling = glm::vec3(1);
    }

    if(node.rotation.size() == 4) {
        glm::quat q;

        q.x = node.rotation[0];
        q.y = node.rotation[1];
        q.z = node.rotation[2];
        q.w = node.rotation[3];

        instance.rotation = eulerAngles(q);
    } else {
        instance.rotation = glm::vec3(0);
    }
    int meshIndex = node.mesh;

    std::string posPrimitiveName = "POSITION";
    std::vector<tinygltf::Primitive> primitives = gltfModel.meshes[meshIndex].primitives;

    instance.min = glm::vec3(-1);
    instance.max = glm::vec3(1);

    if(!primitives.empty()) {
        // initialize min and max from the first primitive
        extractBoundsFromPrimitive(gltfModel, primitives[0], instance.min,
                                   instance.max);
        // grab maximum over all primitives (skipping the first one that was used for initialization)
        for(int primitiveID = 1; primitiveID < primitives.size(); primitiveID++) {
            glm::vec3 min, max;
            extractBoundsFromPrimitive(gltfModel, primitives[primitiveID], min, max);
            instance.min = glm::min(instance.min, min);
            instance.max = glm::max(instance.max, max);
        }
    }
    return instance;
}

/*
* min and max are the outputs
*/
void ModelLoader::extractBoundsFromPrimitive(tinygltf::Model &gltfModel,
                                tinygltf::Primitive &primitive,
                                glm::vec3& min,
                                glm::vec3& max) {
    std::string posPrimitiveName = "POSITION";
    if(primitive.attributes.count(posPrimitiveName)) {
        int accessorIndex = primitive.attributes.at(posPrimitiveName);

        tinygltf::Accessor acc = gltfModel.accessors[accessorIndex];

        if(acc.minValues.size() == 3) {
            min.x = acc.minValues[0];
            min.y = acc.minValues[1];
            min.z = acc.minValues[2];
        }

        if(acc.maxValues.size() == 3) {
            max.x = acc.maxValues[0];
            max.y = acc.maxValues[1];
            max.z = acc.maxValues[2];
        }
    }
}

/*
 * Calculates transformation matrix from scale, rotation and translation of a Node.
 */
glm::mat4 ModelLoader::getTransform(tinygltf::Node node) {
    glm::mat4 transform = glm::mat4(1.0f);

    if(node.translation.size() != 0) {
        transform *= glm::translate(glm::mat4(1), glm::vec3(node.translation[0],
                                                            node.translation[1],
                                                            node.translation[2]));
    }
    if(node.rotation.size() != 0) {
        glm::quat quat;
        quat.x = node.rotation[0];
        quat.y = node.rotation[1];
        quat.z = node.rotation[2];
        quat.w = node.rotation[3];

        transform *= glm::toMat4(quat);
    }
    if(node.scale.size() != 0) {
        transform *= glm::scale(glm::mat4(1),
                                glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
    }
    return transform;
}

/*
 * Creates a material by loading all the needed textures and setting IDs in the
 * material to point at them.
 */
Material ModelLoader::createMaterial(tinygltf::Material& gltfMaterial,
                                     int                 texturesOffset,
                                     std::vector<tinygltf::Texture>& gltfTextures,
                                     std::vector<tinygltf::Image>& gltfImages,
                                     VulkanBaseContext             context,
                                     CommandContext commandContext) {
    Material material;
    auto     pbrSection = gltfMaterial.pbrMetallicRoughness;
    if(pbrSection.baseColorTexture.index != -1) {
        int imageIndex = gltfTextures[pbrSection.baseColorTexture.index].source;
        std::string uri = gltfImages[imageIndex].uri;
        int textureID = createTexture(uri, texturesOffset, VK_FORMAT_R8G8B8A8_SRGB,
                                      context, commandContext);
        material.albedoTextureID = textureID;
    } else {
        material.albedo = glm::vec3(pbrSection.baseColorFactor[0],
                                    pbrSection.baseColorFactor[1],
                                    pbrSection.baseColorFactor[2]);
    }
    if(gltfMaterial.normalTexture.index != -1) {
        int imageIndex  = gltfTextures[gltfMaterial.normalTexture.index].source;
        std::string uri = gltfImages[imageIndex].uri;
        int textureID = createTexture(uri, texturesOffset, VK_FORMAT_R8G8B8A8_UNORM,
                                      context, commandContext);
        material.normalTextureID = textureID;
    }
    if(gltfMaterial.occlusionTexture.index != -1) {
        int imageIndex = gltfTextures[gltfMaterial.occlusionTexture.index].source;
        std::string uri = gltfImages[imageIndex].uri;
        int textureID = createTexture(uri, texturesOffset, VK_FORMAT_R8G8B8A8_UNORM,
                                      context, commandContext);
        material.aoRoughnessMetallicTextureID = textureID;
    }
    if(pbrSection.metallicRoughnessTexture.index != -1
       && material.aoRoughnessMetallicTextureID == -1) {
        int imageIndex = gltfTextures[pbrSection.metallicRoughnessTexture.index].source;
        std::string uri = gltfImages[imageIndex].uri;
        int textureID = createTexture(uri, texturesOffset, VK_FORMAT_R8G8B8A8_UNORM,
                                      context, commandContext);
        material.aoRoughnessMetallicTextureID = textureID;
    }
    material.aoRoughnessMetallic.r = 1;
    material.aoRoughnessMetallic.g = pbrSection.roughnessFactor;
    material.aoRoughnessMetallic.b = pbrSection.metallicFactor;
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
    Mesh                      mesh;
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    // TODO: needs try catch and error handling

    tinygltf::Accessor position = accessors[primitive.attributes.at("POSITION")];
    tinygltf::Accessor normal   = accessors[primitive.attributes.at("NORMAL")];
    tinygltf::Accessor tangents = accessors[primitive.attributes.at("TANGENT")];
    tinygltf::Accessor texCoord0 = accessors[primitive.attributes.at("TEXCOORD_0")];

    // every vertex has to support internal format
    assert(position.count == normal.count && position.count == tangents.count
           && position.count == texCoord0.count);

    int   positionBufferIndex = bufferViews[position.bufferView].buffer;
    int   positionOffset      = bufferViews[position.bufferView].byteOffset;
    auto* positionBuffer      = &buffers[positionBufferIndex].data;

    int   normalBufferIndex = bufferViews[normal.bufferView].buffer;
    int   normalOffset      = bufferViews[normal.bufferView].byteOffset;
    auto* normalBuffer      = &buffers[normalBufferIndex].data;

    int   tangentsBufferIndex = bufferViews[tangents.bufferView].buffer;
    int   tangentsOffset      = bufferViews[tangents.bufferView].byteOffset;
    auto* tangentsBuffer      = &buffers[tangentsBufferIndex].data;

    int   texCoordBufferIndex = bufferViews[texCoord0.bufferView].buffer;
    int   texCoordOffset      = bufferViews[texCoord0.bufferView].byteOffset;
    auto* texCoordBuffer      = &buffers[texCoordBufferIndex].data;

    // convert vertices into internal format
    for(int i = 0; i < position.count; i++) {
        Vertex vert;
        // last number from the multiplication is the number of components in the vector
        auto positionAddress = &(*positionBuffer)[positionOffset + i * FLOAT_SIZE * 3];
        auto normalAddress = &(*normalBuffer)[normalOffset + i * FLOAT_SIZE * 3];
        auto tangentsAddress = &(*tangentsBuffer)[tangentsOffset + i * FLOAT_SIZE * 4];
        auto texCoordAddress = &(*texCoordBuffer)[texCoordOffset + i * FLOAT_SIZE * 2];

        // extract data from byte array
        vert.pos      = bytesToVec3(positionAddress);
        vert.nrm      = bytesToVec3(normalAddress);
        vert.tangents = bytesToVec4(tangentsAddress);
        vert.texCoord = bytesToVec2(texCoordAddress);
        vertices.push_back(vert);
    }

    // default parsing function (if indices are stored as byte)
    auto parseIndex = bytesFromUnsignedCharToUnsignedInt;
    // size of one index in bytes
    int indexStride   = 1;
    int componentType = accessors[primitive.indices].componentType;

    // choose parsing function based on the numeric type that the index is stored as in the glTF file
    if(componentType == COMPONENT_TYPE_UINT) {
        parseIndex  = bytesToUnsignedInt;
        indexStride = 4;
    } else if(componentType == COMPONENT_TYPE_USHORT) {
        parseIndex  = bytesFromUnsignedShortToUnsignedInt;
        indexStride = 2;
    }

    int   indexBufferIndex = bufferViews[primitive.indices].buffer;
    int   indexOffset      = bufferViews[primitive.indices].byteOffset;
    auto* indexBuffer      = &buffers[indexBufferIndex].data;

    // convert indices into internal format
    for(int i = 0; i < accessors[primitive.indices].count; i++) {
        auto indexAddress  = &(*indexBuffer)[indexOffset + i * indexStride];
        unsigned int index = parseIndex(indexAddress);
        indices.push_back(index);
    }

    mesh.verticesCount = vertices.size();
    mesh.indicesCount  = indices.size();
    createMeshBuffers(context, commandContext, vertices, indices, mesh);
    // TODO: calculate Mesh.radius for later frustum culling
    return mesh;
}

int ModelLoader::createTexture(std::string       uri,
                               int               texturesOffset,
                               VkFormat          format,
                               VulkanBaseContext context,
                               CommandContext    commandContext) {
    // TODO: check for duplicate URIs -> need access to the "textures" list of the scene

    Texture texture;
    // the URI that is saved with the texture is the name of the file inside the "textures/" directory
    uri = uri.substr(uri.find(TEXTURES_DIRECTORY_NAME) + TEXTURES_DIRECTORY_NAME_SIZE);
    texture.uri = uri;
    // prepend the path to the "textures/" directory so the image loader finds the file
    std::string path = std::string(ASSETS_DIRECTORY_PATH)
                       + std::string(TEXTURES_DIRECTORY_NAME) + uri;
    // TODO: whether to create mipmaps or not should be a render setting (maybe also max mip levels)
    createTextureImage(context, commandContext, path, format, texture, true);

    textures.push_back(texture);
    return textures.size() - 1 + texturesOffset;
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
