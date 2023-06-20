#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "../../../src/rendering/host_device.h"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangents;
layout(location = 4) in vec2 inTexCoords;

layout(location = 5) in vec4 inShadowCoords;

layout(location = 0) out vec4 outColor;

in vec4 gl_FragCoord ;

// materials array
layout(std140, set = 1, binding = eMaterials) buffer Materials {MaterialDescription m[];} materials;

layout(set = 1, binding = eTextures) uniform sampler2D samplers[];

layout(set = 2, binding = eShadowDepthBuffer) uniform sampler2D depthSampler;

layout( push_constant ) uniform _PushConstant { PushConstant pushConstant; };

// this function is inspired by Sasha Willems shadow mapping example in https://github.com/SaschaWillems/Vulkan
float getShadow(vec4 shadowCoords) {
    float shadow = 1.0;

    if ( shadowCoords.z > -1.0 && shadowCoords.z < 1.0 ) {
        float dist = texture( depthSampler, shadowCoords.st ).r;
		if ( shadowCoords.w > 0.0 && dist < shadowCoords.z )
		{
			shadow = 0.1;
		}
    }

    return shadow;
}

void main() {
    vec4 shadowCoordsHom = inShadowCoords / inShadowCoords.w;
    vec4 normalizedShadowCoords = vec4((shadowCoordsHom.xy + vec2(1)) / 2, shadowCoordsHom.ba);

    float shadow = getShadow(normalizedShadowCoords);

    // fetch material
    MaterialDescription material = materials.m[pushConstant.materialIndex];

    // apply normal mappingd
    vec3 N = normalize(inNormal);
	vec3 T = normalize(inTangents.xyz);
    vec3 B = cross(N, T) * inTangents.w;
	mat3 TBN = mat3(T, B, N);
	vec3 normal = texture(samplers[material.normalTextureID], inTexCoords).rgb * 2.0 - vec3(1.0);
    normal = normalize(TBN * normal);

    vec4 albedo = texture(samplers[material.albedoTextureID], inTexCoords);
    // various debug outputs for the color
    outColor = vec4(albedo.xyz * shadow, albedo.a);
    //outColor = vec4(normal, 1) * 0.5 + 0.5;
}
