/*
 * Inspired by a similar file used in
 * https://github.com/nvpro-samples/vk_raytracing_tutorial_KHR.
 *
 */

#ifdef __cplusplus
#pragma once
#include "glm/glm.hpp"
using ivec2 = glm::ivec2;
using vec3 = glm::vec3;
using mat3 = glm::mat3;
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
    eTextures  = 1   // storage buffer containing image views
END_BINDING();

START_BINDING(DepthBindings)
    eShadowDepthBuffer = 0,
    eCascadeSplits = 1,
    eLightVPs = 2
END_BINDING();

START_BINDING(GBufferBindings)
    eNormal = 0,
    eAlbedo = 1,
    ePBR = 2,
    eDepth = 3
END_BINDING();

START_BINDING(SkyboxBindings)
    eSkybox = 0,
    eIrradiance = 1,
    eRadiance = 2,
    eLUT = 3
END_BINDING();

const uint MAX_CASCADES = 4;

const uint PCF_CONTROL_BIT          = 0x01;
const uint CASCADE_VIS_CONTROL_BIT  = 0x02;

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

// used per camera
struct CameraUniform
{
    ALIGN_AS(16) mat4 view;
    ALIGN_AS(16) mat4 proj;
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
    // is only needed in the geometry pass to correctly transform normals
    ALIGN_AS(16) mat4 normalsTransformation;
    ALIGN_AS(16) vec3 worldCamPosition;
    // index of the material (in the material buffer) for the current MeshPart
    ALIGN_AS(4) int materialIndex;
    ALIGN_AS(4) int cascadeCount;
    ALIGN_AS(4) int controlFlags;
    ALIGN_AS(8) ivec2 resolution;
};

struct StencilPushConstant
{
    ALIGN_AS(16) mat4 transformation;
    ALIGN_AS(16) mat4 projView;
};

// used in the point light pipeline
struct PointLightPushConstant
{
    ALIGN_AS(16) mat4 transformation;
    ALIGN_AS(16) vec3 worldCamPosition;
    ALIGN_AS(4) float radius;
    ALIGN_AS(16) vec3 position;
    ALIGN_AS(16) vec3 intensity;
    ALIGN_AS(8) ivec2 resolution;
};

// used in skybox pipeline
struct SkyboxPushConstant
{
    ALIGN_AS(4) float exposure;
};

struct ShadowPushConstant
{
    ALIGN_AS(16) mat4 transform;
    ALIGN_AS(4) int cascadeIndex;
    ALIGN_AS(4) int materialIndex;
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
    ALIGN_AS(4) int shadows;
    ALIGN_AS(4) float iblFactor;
};