#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier: enable

#include "../../../src/rendering/host_device.h"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec4 inTangents;
layout (location = 2) in vec2 inTexCoords;

// gBuffer
layout(location = eNormal) out vec4 outNormal;
layout(location = eAlbedo) out vec4 outAlbedo;
layout(location = ePBR) out vec4 outAoRoughnessMetallic;

// materials array
layout (std140, set = 1, binding = eMaterials) buffer Materials {MaterialDescription m[];} materials;
// textures array
layout(set = 1, binding = eTextures) uniform sampler2D samplers[];
layout( push_constant ) uniform _PushConstant { PushConstant pushConstant; };

void main() {
    // fetch material
    MaterialDescription material = materials.m[pushConstant.materialIndex];

    vec3 albedo = material.albedo;
    if(material.albedoTextureID != -1) {
        vec4 albedoTexture = texture(samplers[material.albedoTextureID], inTexCoords);
        // allow alpha masking
        if (albedoTexture.a == 0)
            discard;
        albedo = albedoTexture.rgb;
    }

    vec3 normal = inNormal;
    if (material.normalTextureID != -1) {
        // apply normal mapping
        vec3 N = normalize(inNormal);
        vec3 T = normalize(inTangents.xyz);
        vec3 B = cross(N, T) * inTangents.w;
        mat3 TBN = mat3(T, B, N);
        normal = texture(samplers[material.normalTextureID], inTexCoords).rgb * 2.0 - vec3(1.0);
        normal = normalize(TBN * normal);
    }

    float ao = 1.0;
    float roughness = material.aoRoughnessMetallic.g;
    float metallic = material.aoRoughnessMetallic.b;
    if (material.aoRoughnessMetallicTextureID != -1) {
        vec3 aoRoughnessMetallic = texture(samplers[material.aoRoughnessMetallicTextureID], inTexCoords).rgb;
        // For some reason, the blender glTF exporter sets AO to 0 if no texture is provided. However we want a default value of 1.0
        ao = aoRoughnessMetallic.r > 0.0 ? aoRoughnessMetallic.r : 1.0;
        roughness *= aoRoughnessMetallic.g;
        metallic *= aoRoughnessMetallic.b;
    }

    outNormal = vec4(normal,0);
    outAlbedo = vec4(albedo,0);
    // the alpha channel of this value can be used to determine whether there is geometry at a certain pixel
    outAoRoughnessMetallic = vec4(ao,roughness,metallic,1);
}