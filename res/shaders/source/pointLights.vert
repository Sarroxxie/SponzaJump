#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../../../src/rendering/host_device.h"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangents;
layout(location = 3) in vec2 texCoords;

layout(set = 0, binding = eCamera) uniform _CameraUniform {CameraUniform cameraUniform; };

layout (push_constant) uniform _PointLightPushConstant { PointLightPushConstant pushConstant; };

void main() {
    gl_Position = cameraUniform.proj * cameraUniform.view * pushConstant.transformation * vec4(inPosition, 1);
}