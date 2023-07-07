#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../../../src/rendering/host_device.h"

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inTangents;
layout (location = 4) in vec2 inTexCoords;

layout (location = 5) in vec3 inViewPos;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outAlbedo;
layout(location = 3) out vec4 outAoRoughnessMetallic;

void main() {
    outPosition = vec4(0,0,0,0);
    outNormal = vec4(0,0,0,0);
    outAlbedo = vec4(0,0,0,0);
    outAoRoughnessMetallic = vec4(0,0,0,0);
}