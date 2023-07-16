#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../../../src/rendering/host_device.h"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangents;
layout(location = 3) in vec2 inTexCoords;

layout(set = 0, binding = eCamera) uniform _CameraUniform {CameraUniform cameraUniform; };

layout( push_constant ) uniform _PushConstant { PushConstant pushConstant; };

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec4 outTangents;
layout(location = 2) out vec2 outTexCoords;

void main() {
    vec4 worldPosition = pushConstant.transformation * vec4(inPosition, 1);
    
    gl_Position = cameraUniform.proj * cameraUniform.view * worldPosition;

    // prepare data for normal mapping
    mat3 normalTransformation = mat3(pushConstant.normalsTransformation);
    outNormal = normalize(normalTransformation * inNormal);
    outTangents = normalize(vec4(normalTransformation * inTangents.xyz, inTangents.w));

    outTexCoords = inTexCoords;
}
