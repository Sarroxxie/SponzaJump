/*
 * Inspired by a similar file used in
 * https://github.com/nvpro-samples/vk_raytracing_tutorial_KHR.
 *
 */

#ifdef __cplusplus
#pragma once
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
    eCamera   = 0,  // Global uniform containing camera matrices
    eLight    = 1,  // Global uniform containing camera matrices
    eLighting = 2
END_BINDING();

START_BINDING(MaterialsBindings)
    eMaterials = 0,  // storage buffer containing materials
    eTextures  = 1,  // storage buffer containing image views
    eSkybox    = 3
END_BINDING();

START_BINDING(DepthBindings)
    eShadowDepthBuffer = 0,
    eCascadeSplits = 1,
    eLightVPs = 2
END_BINDING();

const uint MAX_CASCADES = 4;

// clang-format on

// copy of "Material"-struct from "scene/Model.h" for use on GPU
struct MaterialDescription
{
    ALIGN_AS(16) vec3 albedo;
    ALIGN_AS(16) vec3 aoRoughnessMetallic;
    ALIGN_AS(4) int albedoTextureID;
    ALIGN_AS(4) int normalTextureID;
    ALIGN_AS(4) int aoRoughnessMetallicTextureID;
};

struct PointLight
{
    ALIGN_AS(16) vec3 position;
    ALIGN_AS(16) vec3 intensity;
    ALIGN_AS(4) float radius;
};

// used per camera
struct CameraUniform
{
    ALIGN_AS(16) mat4 viewProj;
    ALIGN_AS(16) mat4 viewInverse;
    ALIGN_AS(16) mat4 projInverse;
};

// For some reason, if I bind it as float array it has Byteoffset of 16, since
// this will always be small anyways, I just use this dummy struct
struct SplitDummyStruct
{
    ALIGN_AS(16) float splitVal;
};

struct PushConstant
{
    // transformation matrix of the current instance
    ALIGN_AS(16) mat4 transformation;
    ALIGN_AS(16) vec3 worldCamPosition;
    // index of the material (in the material buffer) for the current MeshPart
    ALIGN_AS(4) int materialIndex;
    ALIGN_AS(4) int cascadeCount;
    ALIGN_AS(4) int doPCF;
};

struct ShadowPushConstant
{
    ALIGN_AS(16) mat4 transform;
    ALIGN_AS(4) int cascadeIndex;
};

struct ShadowControlPushConstant
{
    ALIGN_AS(4) int cascadeIndex;
};

struct LightingInformation
{
    ALIGN_AS(16) vec3 cameraPosition;
    ALIGN_AS(16) vec3 lightDirection;
    ALIGN_AS(16) vec3 lightIntensity;
    ALIGN_AS(4) int doPCF;
    // could store some omnidirectional light sources here for demonstration
};