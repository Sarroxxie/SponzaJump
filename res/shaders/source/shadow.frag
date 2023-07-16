#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier: enable

#include "../../../src/rendering/host_device.h"

layout (location = 0) in vec2 inTexCoords;

layout (std140, set = 1, binding = eMaterials) buffer Materials {MaterialDescription m[];} materials;

layout(set = 1, binding = eTextures) uniform sampler2D samplers[];

layout (push_constant) uniform shadowPushConstant {
    mat4 transform;
    int cascadeIndex;
    int materialIndex;
} pushConstant;

void main() {
    MaterialDescription material = materials.m[pushConstant.materialIndex];

    vec3 albedo = material.albedo;
    if(material.albedoTextureID != -1) {
        vec4 albedoTexture = texture(samplers[material.albedoTextureID], inTexCoords);
        // allow alpha masking
        if (albedoTexture.a < 1.0) discard;
    }
}