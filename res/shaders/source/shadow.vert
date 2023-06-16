#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../../../src/rendering/host_device.h"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangents;
layout(location = 3) in vec2 texCoords;

layout(set = 0, binding = eCamera) uniform SceneTransform {
    mat4 proj;
    mat4 view;
} sceneTransform;

layout( push_constant ) uniform ObjectTransform {
    mat4 obj;
} objectTransform;

void main() {
    vec4 pos = sceneTransform.proj * sceneTransform.view * objectTransform.obj * vec4(inPosition, 1);

    gl_Position = vec4(pos.xyz / pos.w, 1);
}


