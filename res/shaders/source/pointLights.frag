#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../../../src/rendering/host_device.h"
#include "BRDF.glsl"

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = eCamera) uniform _CameraUniform {CameraUniform cameraUniform; };

// primarily used for the camera position
layout(set = 0, binding = eLighting) uniform _LightingInformation {LightingInformation lightingInformation; };

// gBuffer
layout (set = 1, binding = eNormal) uniform sampler2D gBufferNormal;
layout (set = 1, binding = eAlbedo) uniform sampler2D gBufferAlbedo;
layout (set = 1, binding = ePBR) uniform sampler2D gBufferPBR;
layout (set = 1, binding = eDepth) uniform sampler2D gBufferDepth;

layout (push_constant) uniform _PointLightPushConstant { PointLightPushConstant pushConstant; };

void main() {
    // pixel coordinates (ranging from (0,0) to (width, height)) for sampling from gBuffer
    ivec2 intCoords = ivec2(gl_FragCoord.xy - 0.5);
    vec4 aoRoughnessMetallic = texelFetch(gBufferPBR, intCoords, 0);
    // aoRoughnessMetallic.a is 1.0 if there is geometry in the gBuffer, else the skybox will be rendered there anyways
    if(aoRoughnessMetallic.a < 1.0)
        discard;
    vec3 normal = normalize(texelFetch(gBufferNormal, intCoords, 0).rgb);
    vec3 albedo = texelFetch(gBufferAlbedo, intCoords, 0).rgb;
    float depth = texelFetch(gBufferDepth, intCoords, 0).r;

    // reconstruct world position from depth
    vec2 screenCoords = gl_FragCoord.xy / pushConstant.resolution * 2.0 - 1.0;
    vec4 tmp = cameraUniform.projInverse * vec4(screenCoords, depth, 1);
    tmp = cameraUniform.viewInverse * (tmp / tmp.w);
    vec3 position = tmp.xyz;

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, aoRoughnessMetallic.b);

    // lighting calculation
    vec3 V = normalize(lightingInformation.cameraPosition - position);
    vec3 L = normalize(pushConstant.position - position);

    // calculate attenuation function, so that attenuation is 0 outside the radius
    float lightDistance = length(pushConstant.position - position);
    float t = lightDistance / pushConstant.radius;
    // the higher the exponent here, the later the light influence fades to 0
    t = -pow(t, 6) + 1.0;
    float physicalAttenuation = 1 / (lightDistance * lightDistance);
    float attenuation = t * physicalAttenuation;

    vec3 radiance = attenuation * pushConstant.intensity;
    vec3 color = BRDF(L, V, normal, radiance, albedo, aoRoughnessMetallic.b, aoRoughnessMetallic.g);

    outColor = vec4(color, 1);
}