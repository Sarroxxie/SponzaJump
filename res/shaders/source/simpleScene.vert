#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../../../src/rendering/host_device.h"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangents;
layout(location = 3) in vec2 inTexCoords;

layout(set = 0, binding = eCamera) uniform SceneTransform {
    mat4 proj;
    mat4 view;
} sceneTransform;

layout( push_constant ) uniform _PushConstant { PushConstant pushConstant; };

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outTangent;
layout(location = 3) out vec3 outBiTangent;
layout(location = 4) out vec2 outTexCoords;

void main() {
    vec4 worldPosition = pushConstant.transformation * vec4(inPosition, 1);
    outPosition = worldPosition.xyz / worldPosition.w;
    
    gl_Position = sceneTransform.proj * sceneTransform.view * worldPosition;

    // prepare data for normal mapping
    mat3 normalTransformation = transpose(inverse(mat3(pushConstant.transformation)));
    outNormal = normalTransformation * normalize(inNormal);
    outTangent = normalTransformation * normalize(inTangents.xyz);
    outBiTangent = normalTransformation * normalize(cross(inNormal, inTangents.xyz) * inTangents.w);

    outTexCoords = inTexCoords;
}