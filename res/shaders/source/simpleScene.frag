#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "../../../src/rendering/host_device.h"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangents;
layout(location = 4) in vec2 inTexCoords;

layout(location = 0) out vec4 outColor;

// materials array
layout(std140, set = 1, binding = eMaterials) buffer Materials {MaterialDescription m[];} materials;

layout(set = 1, binding = eTextures) uniform sampler2D samplers[];

layout( push_constant ) uniform _PushConstant { PushConstant pushConstant; };

void main() {
    // fetch material
    MaterialDescription material = materials.m[pushConstant.materialIndex];

    // apply normal mapping
    vec3 N = normalize(inNormal);
	vec3 T = normalize(inTangents.xyz);
    vec3 B = cross(N, T) * inTangents.w;
	mat3 TBN = mat3(T, B, N);
	vec3 normal = texture(samplers[material.normalTextureID], inTexCoords).rgb * 2.0 - vec3(1.0);
    normal = normalize(TBN * normal);

    vec4 albedo = texture(samplers[material.albedoTextureID], inTexCoords);
    // various debug outputs for the color
    outColor = albedo;
    //outColor = vec4(normal, 1) * 0.5 + 0.5;
}
