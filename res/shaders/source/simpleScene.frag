#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../../../src/rendering/host_device.h"

layout(location = 0) in vec2 fragPos;
layout(location = 1) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

// materials array
layout(std140, set = 1, binding = eMaterials) buffer Materials {MaterialDescription m[];} materials;

void main() {
    MaterialDescription mat = materials.m[0];
    //outColor = vec4(fragColor, 1);
    outColor = vec4(mat.albedo, 1);
}