#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../../../src/rendering/host_device.h"
#include "BRDF.glsl"

const vec3 cascadeVisColors[MAX_CASCADES] = vec3[](
    vec3(1, 0, 0),
    vec3(1, 1, 0),
    vec3(0, 1, 0),
    vec3(0, 0, 1)
);

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = eCamera) uniform _CameraUniform {CameraUniform cameraUniform; };

// primarily used for the camera position
layout(set = 0, binding = eLighting) uniform _LightingInformation {LightingInformation lightingInformation; };

// shadow mapping
layout (set = 1, binding = eShadowDepthBuffer) uniform sampler2D depthSamplers[MAX_CASCADES];

layout (set = 1, binding = eCascadeSplits) uniform _CascadeSplits {
    SplitDummyStruct split[MAX_CASCADES];
} cascadeSplits;

layout (set = 1, binding = eLightVPs) uniform _LightVPs {
    mat4 mats[MAX_CASCADES];
} LightVPs;

// gBuffer
layout (set = 2, binding = eNormal) uniform sampler2D gBufferNormal;
layout (set = 2, binding = eAlbedo) uniform sampler2D gBufferAlbedo;
layout (set = 2, binding = ePBR) uniform sampler2D gBufferPBR;
layout (set = 2, binding = eDepth) uniform sampler2D gBufferDepth;

// Image Based Lighting
layout(set = 3, binding = eIrradiance) uniform samplerCube irradianceMap;
layout(set = 3, binding = eRadiance) uniform samplerCube radianceMap;
layout(set = 3, binding = eLUT) uniform sampler2D brdfLUT;

layout (push_constant) uniform _PushConstant { PushConstant pushConstant; };


// following two functions largely taken from sasha willems shadow mapping example in https://github.com/SaschaWillems/Vulkan
float getShadow(vec4 shadowCoords, vec2 offset, uint cascadeIndex) {
    float shadow = 1.0;

    if (shadowCoords.z > -1.0 && shadowCoords.z < 1.0) {
        float dist = texture(depthSamplers[cascadeIndex], shadowCoords.st + offset).r;
        if (shadowCoords.w > 0.0 && dist < shadowCoords.z)
        {
            shadow = 0.0;
        }
    }

    return shadow;
}

float filterPCF(vec4 sc, uint cascadeIndex)
{
    ivec2 texDim = textureSize(depthSamplers[cascadeIndex], 0);
    float scale = 1;
    float dx = scale * 1.0 / float(texDim.x);
    float dy = scale * 1.0 / float(texDim.y);

    float shadowFactor = 0.0;
    int count = 0;
    int range = 1;

    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++)
        {
            shadowFactor += getShadow(sc, vec2(dx * x, dy * y), cascadeIndex);
            count++;
        }

    }
    return shadowFactor / count;
}

bool getControlFlagValue(uint flags, uint controlBit) {
    return (flags & controlBit) == controlBit;
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   

void main() {
    // pixel coordinates (ranging from (0,0) to (width, height)) for sampling from gBuffer
    ivec2 intCoords = ivec2(gl_FragCoord.xy - 0.5);
    vec3 normal = normalize(texelFetch(gBufferNormal, intCoords, 0).rgb);
    vec3 albedo = texelFetch(gBufferAlbedo, intCoords, 0).rgb;
    vec3 aoRoughnessMetallic = texelFetch(gBufferPBR, intCoords, 0).rgb;
    float depth = texelFetch(gBufferDepth, intCoords, 0).r;

    // reconstruct world position from depth
    vec2 screenCoords = gl_FragCoord.xy / pushConstant.resolution * 2.0 - 1.0;
    vec4 tmp = cameraUniform.projInverse * vec4(screenCoords, depth, 1);
    tmp = cameraUniform.viewInverse * (tmp / tmp.w);
    vec3 position = tmp.xyz;

    // depth in view space used for cascade evaluation
    float viewSpaceDepth = (cameraUniform.view * vec4(position, 1)).z;

    // shadow stuff
    uint cascadeIndex = 0;
    for(uint i = 0; i < pushConstant.cascadeCount - 1; ++i) {
        if(viewSpaceDepth < cascadeSplits.split[i].splitVal) {
            cascadeIndex = i + 1;
        }
    }
    vec4 shadowCoord = (LightVPs.mats[cascadeIndex]) * vec4(position, 1.0);

    shadowCoord = shadowCoord / shadowCoord.w;
    shadowCoord = vec4((shadowCoord.xyz + vec3(1)) / 2, shadowCoord.a);

    float shadow = 0;
    bool doPCF = getControlFlagValue(pushConstant.controlFlags, PCF_CONTROL_BIT);

    if (doPCF) {
        shadow = filterPCF(shadowCoord, cascadeIndex);
    } else {
        shadow = getShadow(shadowCoord, vec2(0.0), cascadeIndex);
    }

    // lighting calculation for directional light source
    vec3 color = vec3(0,0,0);
    vec3 V = normalize(lightingInformation.cameraPosition - position);
    /*if (shadow > 0) {
        vec3 L = normalize(-lightingInformation.lightDirection);
        color += BRDF(L, V, normal, lightingInformation.lightIntensity * 5, albedo, aoRoughnessMetallic.b, aoRoughnessMetallic.g) * shadow;
    }*/
    // ambient lighting (we now use IBL as the ambient term)


    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 R = reflect(-V, normal);
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, aoRoughnessMetallic.b);
    vec3 F = fresnelSchlickRoughness(max(dot(normal, V), 0.0), F0, aoRoughnessMetallic.g);
    
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - aoRoughnessMetallic.b;	  
    
    vec3 irradiance = texture(irradianceMap, normal).rgb;
    vec3 diffuse = irradiance * albedo;
    
    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(radianceMap, R,  aoRoughnessMetallic.g * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(normal, V), 0.0), aoRoughnessMetallic.g)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * aoRoughnessMetallic.r;

    // tune down IBL influence when shadowed
    if(shadow == 0) {
        ambient *= 0.015;
    }
    // old, not IBL ambient
    //vec3 ambient = vec3(0.01) * albedo * aoRoughnessMetallic.r;
    color += ambient;

    outColor = vec4(color, 1);
    bool cascadeVis = getControlFlagValue(pushConstant.controlFlags, CASCADE_VIS_CONTROL_BIT);
    if (cascadeVis) {
        vec3 addColor = cascadeVisColors[cascadeIndex];

        float factor = 0.2;
        outColor = vec4(outColor.xyz + factor * addColor, 1);
    }
}