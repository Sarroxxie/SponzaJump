#include "ModelLoader.h"
#include <glm/gtx/quaternion.hpp>

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

bool ModelLoader::loadModel(const std::string& filename) {
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
    // 1. create Meshes
    for(auto& gltfMesh : gltfModel.meshes) {
        // a "Mesh" from the glTF file is called a "Model" in our program
        Model model;
        for(auto& primitive : gltfMesh.primitives) {
            int meshPartIndex = -1;
            int meshIndex     = findGeometryData(primitive);
            if(meshIndex < 0) {
                // Mesh not yet created, so create a new one alongside a MeshPart
                Mesh mesh = createMesh(primitive);
                meshes.emplace_back(mesh);
                meshParts.emplace_back(MeshPart(meshes.size() - 1, primitive.material));
                model.meshPartIndices.emplace_back(meshParts.size() - 1);
                meshLookups.emplace_back(MeshLookup(primitive, meshes.size() - 1));
            } else {
                // Mesh exists, so we need to see if there exists a MeshPart
                // pointing at that Mesh and sharing the material index
                bool meshPartExists = false;
                for(int i = 0; i < meshParts.size(); i++) {
                    if(meshParts[i].meshIndex == meshIndex
                       && meshParts[i].materialIndex == primitive.material) {
                        meshPartExists = true;
                        model.meshPartIndices.emplace_back(i);
                        break;
                    }
                }
                if(!meshPartExists) {
                    // create the MeshPart that points to the found Mesh and
                    // the material of the primitive
                    meshParts.emplace_back(MeshPart(meshIndex, primitive.material));
                    model.meshPartIndices.emplace_back(meshParts.size() - 1);
                }
            }
        }
        models.emplace_back(model);
    }
    // 2. create Materials
    for(auto& gltfMaterial : gltfModel.materials) {
        materials.emplace_back(createMaterial(gltfMaterial, gltfModel));
    }

    // 3. create ModelInstances (from list of Nodes)
    for(auto& node : gltfModel.nodes) {
        ModelInstance instance(&models[node.mesh]);
        glm::mat4     transform = getTransform(node);
        instances.emplace_back(instance);
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
        textures.emplace_back(createTexture(
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
        textures.emplace_back(createTexture(
            gltfImages[gltfTextures[gltfMaterial.normalTexture.index].source].uri));
        material.normalTextureID = textures.size() - 1;
    }
    if(gltfMaterial.occlusionTexture.index != -1) {
        textures.emplace_back(createTexture(
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
 * TODO: implement this
 * Gets the geometry data from the primitive and stores it in a Mesh.
 */
Mesh ModelLoader::createMesh(tinygltf::Primitive& primitive) {
    return Mesh();
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
