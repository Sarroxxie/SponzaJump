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

layout(set = 0, binding = eLight) uniform LightTransform {
    mat4 proj;
    mat4 view;
} lightTransform;

layout( push_constant ) uniform _PushConstant { PushConstant pushConstant; };

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec4 outTangents;
layout(location = 4) out vec2 outTexCoords;

layout(location = 5) out vec4 outShadowCoords;

void main() {
    vec4 worldPosition = pushConstant.transformation * vec4(inPosition, 1);
    outPosition = worldPosition.xyz / worldPosition.w;
    
    gl_Position = sceneTransform.proj * sceneTransform.view * worldPosition;
    outShadowCoords = lightTransform.proj * lightTransform.view * worldPosition;

    // prepare data for normal mapping
    mat3 normalTransformation = transpose(inverse(mat3(pushConstant.transformation)));
    outNormal = normalize(normalTransformation * inNormal);
    outTangents = vec4(normalize(normalTransformation * inTangents.xyz), inTangents.w);

    outTexCoords = inTexCoords;
}
