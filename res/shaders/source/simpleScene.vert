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

layout( push_constant ) uniform _PushConstant { PushConstant pushConstant; };

layout(location = 0) out vec2 fragPos;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec2 fragTexCoords;

void main() {
    gl_Position = sceneTransform.proj * sceneTransform.view * pushConstant.transformation * vec4(inPosition, 1);
    fragColor = (inNormal + 1) * 0.5;
    fragTexCoords = texCoords;
    //fragColor = normal;
}