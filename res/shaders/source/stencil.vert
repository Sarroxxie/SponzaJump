#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../../../src/rendering/host_device.h"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangents;
layout(location = 3) in vec2 texCoords;

layout (push_constant) uniform _StencilPushConstant { StencilPushConstant pushConstant; };

void main() {
    gl_Position = pushConstant.projView * pushConstant.transformation * vec4(inPosition, 1);
}