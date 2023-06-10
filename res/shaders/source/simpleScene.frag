#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "../../../src/rendering/host_device.h"

layout(location = 0) in vec2 fragPos;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 fragTexCoords;

layout(location = 0) out vec4 outColor;

// materials array
layout(std140, set = 1, binding = eMaterials) buffer Materials {MaterialDescription m[];} materials;

layout(set = 1, binding = eTextures) uniform sampler2D samplers[];

layout( push_constant ) uniform _PushConstant { PushConstant pushConstant; };

void main() {
    MaterialDescription mat = materials.m[pushConstant.materialIndex];
    vec4 textureColor = texture(samplers[mat.albedoTextureID], fragTexCoords);
    outColor = vec4(mat.albedo, 1);
    outColor = textureColor;
    //outColor = vec4(fragColor, 1);
}