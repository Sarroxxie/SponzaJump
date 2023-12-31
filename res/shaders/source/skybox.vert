#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../../../src/rendering/host_device.h"

vec2 positions[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0,  1.0)
);

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangents;
layout(location = 3) in vec2 texCoords;

layout(set = 0, binding = eCamera) uniform _CameraUniform {CameraUniform cameraUniform; };

layout(location = 0) out vec3 outPosition;

void main() {
    // set depth to maximum amount so the skybox will always be behind all other objects
    gl_Position = vec4(positions[gl_VertexIndex], 1.0, 1.0);
    // revert projection and view transformation to get world space coordinates of the quad
    vec4 projected = cameraUniform.viewInverse * cameraUniform.projInverse * vec4(positions[gl_VertexIndex], 0.0, 1.0);
    outPosition = projected.xyz / projected.w;
}