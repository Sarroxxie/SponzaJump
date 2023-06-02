/*
 * Inspired by a similar file used in
 * https://github.com/nvpro-samples/vk_raytracing_tutorial_KHR.
 *
 */

#ifdef __cplusplus
#include "glm/glm.hpp"
using vec3 = glm::vec3;
using mat4 = glm::mat4;
using uint = unsigned int;
#endif

// clang-format off
#ifdef __cplusplus  // Descriptor binding helper for C++ and GLSL
    #define START_BINDING(a) enum a {
    #define END_BINDING() }
#else
    #define START_BINDING(a) const uint
    #define END_BINDING()
#endif

// access for descriptor sets
START_BINDING(SceneBindings)
    eCamera    = 0,  // Global uniform containing camera matrices
    eMaterials = 1,      // Access to the object descriptions
    eTextures  = 2       // Access to textures
END_BINDING();
// clang-format on

// copy of "Material"-struct from "scene/Model.h" for use on GPU
struct MaterialDescription
{
    // TODO: find out if we even allow non-textured materials
    vec3 albedo;
    vec3 aoRoughnessMetallic;
    int  albedoTextureID;
    int  normalTextureID;
    int  aoRoughnessMetallicTextureID;
};

// used per camera
struct CameraUniform
{
    mat4 viewProj;
    mat4 viewInverse;
    mat4 projInverse;
};

struct PushConstant
{
    vec3 lightPosition;
    vec3 lightIntensity;
    // transformation matrix of the current instance
    mat4 transformation;
    // index of the material (in the material buffer) for the current MeshPart
    uint materialIndex;
};