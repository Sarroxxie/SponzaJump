#version 460
#extension GL_GOOGLE_include_directive: enable

#include "../../../src/rendering/host_device.h"

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inTangents;
layout (location = 3) in vec2 texCoords;

layout (set = 0, binding = eCamera) uniform SceneTransform {
    SceneTransformStruct data[MAX_CASCADES];
} sceneTransform;

layout (push_constant) uniform ObjectTransform {
    mat4 transform;
    int cascadeIndex;
} objectTransform;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() {
    vec4 pos =  sceneTransform.data[objectTransform.cascadeIndex].projection
                * sceneTransform.data[objectTransform.cascadeIndex].view
                * objectTransform.transform * vec4(inPosition, 1);

    gl_Position = pos;
}
