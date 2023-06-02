#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../../../src/rendering/host_device.h"

layout(location = 0) in vec2 fragPos;
layout(location = 1) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

// TODO: needs descriptor set
//layout(set = 0, binding = 1) buffer Materials {MaterialDescription m[];} ; // materials array

void main() {
    outColor = vec4(fragColor, 1);
}