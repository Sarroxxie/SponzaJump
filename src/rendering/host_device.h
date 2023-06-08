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
    #define ALIGN_AS(a) alignas(a)
#else
    #define START_BINDING(a) const uint
    #define END_BINDING()
    #define ALIGN_AS(a)
#endif

// access for descriptor sets
// note that the names have to be unique, because they are unique in the shader
START_BINDING(SceneBindings)
    eCamera    = 0  // Global uniform containing camera matrices
END_BINDING();

START_BINDING(MaterialsBindings)
    eMaterials = 0,  // storage buffer containing materials
    eTextures  = 1      // storage buffer containing image views
END_BINDING();


// clang-format on

// copy of "Material"-struct from "scene/Model.h" for use on GPU
struct MaterialDescription
{
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
    // transformation matrix of the current instance
    ALIGN_AS(16) mat4 transformation;
    // index of the material (in the material buffer) for the current MeshPart
    ALIGN_AS(4) int materialIndex;
};