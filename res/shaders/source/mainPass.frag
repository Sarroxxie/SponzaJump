#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../../../src/rendering/host_device.h"

layout(location = 0) in vec2 fragPos;
layout(location = 1) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

in vec4 gl_FragCoord ;

// materials array
layout(std140, set = 1, binding = eMaterials) buffer Materials {MaterialDescription m[];} materials;

layout(set = 2, binding = eShadowDepthBuffer) uniform sampler2D depthSampler;

void main() {
    MaterialDescription mat = materials.m[0];
    outColor = vec4(mat.albedo, 1);
    outColor = vec4(fragColor, 1);
    vec2 normFragCoord = vec2(gl_FragCoord.x / 1920, gl_FragCoord.y / 1080);
    outColor = vec4(normFragCoord.x, normFragCoord.y, 0, 1);
    float val = texture(depthSampler, normFragCoord).x / 10;
    outColor = vec4(val, val, val, 1);
}