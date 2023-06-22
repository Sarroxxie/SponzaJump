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

layout(set = 0, binding = eCamera) uniform SceneTransform {
    mat4 proj;
    mat4 view;
} sceneTransform;

layout(location = 0) out vec3 outPosition;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    vec4 projected = inverse(sceneTransform.proj) * vec4(positions[gl_VertexIndex], 0.0, 1.0);
    outPosition = projected.xyz / projected.w;
}