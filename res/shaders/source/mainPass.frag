#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "../../../src/rendering/host_device.h"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangents;
layout(location = 4) in vec2 inTexCoords;

layout(location = 0) out vec4 outColor;

in vec4 gl_FragCoord ;

// materials array
layout(std140, set = 1, binding = eMaterials) buffer Materials {MaterialDescription m[];} materials;

layout(set = 1, binding = eTextures) uniform sampler2D samplers[];

layout(set = 2, binding = eShadowDepthBuffer) uniform sampler2D depthSampler;

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

    // fetch PBR textures
    vec4 albedo = texture(samplers[material.albedoTextureID], inTexCoords);
    vec3 aoRoughnessMetallic = texture(samplers[material.aoRoughnessMetallicTextureID], inTexCoords).rgb;
    float ao = aoRoughnessMetallic.r;
    float roughness = aoRoughnessMetallic.g;
    float metallic = aoRoughnessMetallic.b;

    // various debug outputs for the color
    outColor = vec4(albedo.xyz, 1);
    //outColor = vec4(normal, 1) * 0.5 + 0.5;
    //outColor = vec4(ao, ao, ao, 1);
    //outColor = vec4(roughness, roughness, roughness, 1);
    //outColor = vec4(metallic, metallic, metallic, 1);

    vec2 normFragCoord = vec2(gl_FragCoord.x / 1920, gl_FragCoord.y / 1080);
    float val = texture(depthSampler, normFragCoord).x / 10;
    //outColor = vec4(val, val, val, 1);
}
