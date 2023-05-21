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
    tinygltf::Model    model;
    tinygltf::TinyGLTF loader;
    std::string        errors;
    std::string        warnings;
    bool               success;

    // set custom image loading function
    loader.SetImageLoader(imageLoaderFunction, nullptr);
    success = loader.LoadASCIIFromFile(&model, &errors, &warnings, filename);

    if(!warnings.empty()) {
        printf("Warn: %s\n", warnings.c_str());
    }

    if(!errors.empty()) {
        printf("Err: %s\n", errors.c_str());
    }

    if(!success) {
        printf("Failed to parse glTF\n");
    } else {
        // grab per node transformation
        for(auto& node : model.nodes) {
            transforms.push_back(getTransform(node));
            int meshIndex = node.mesh;
        }
    }
    return success;
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

Material ModelLoader::createMaterial(tinygltf::Material material) {
    return Material();
}

Mesh ModelLoader::createMesh(tinygltf::Mesh mesh, tinygltf::Accessor accessors) {
    Mesh geometryMesh;
    return geometryMesh;
}
