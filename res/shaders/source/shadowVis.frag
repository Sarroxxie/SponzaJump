#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../../../src/rendering/host_device.h"

layout(location = 0) in vec2 fragTexCoords;

layout (set = 0, binding = eShadowDepthBuffer) uniform sampler2D depthSamplers[MAX_CASCADES];

layout (push_constant) uniform _PushConstant { ShadowControlPushConstant pushConstant; };

layout(location = 0) out vec4 outColor;

void main() {
    float val = texture(depthSamplers[pushConstant.cascadeIndex], fragTexCoords).x;
    vec4 texVal = texture(depthSamplers[pushConstant.cascadeIndex], fragTexCoords);
    val = 1 - val;
    val = pow(val, 2);
    outColor = vec4(val, val, val, 1);
}