#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../../../src/rendering/host_device.h"

layout(location = 0) in vec2 fragTexCoords;

layout(set = 0, binding = eShadowDepthBuffer) uniform sampler2D depthSampler;

layout(location = 0) out vec4 outColor;

void main() {
    float val = texture(depthSampler, fragTexCoords).x;
    vec4 texVal = texture(depthSampler, fragTexCoords);
    val = 1 - val;
    outColor = vec4(val, val, val, 1);
    // outColor = vec4(fragTexCoords, 0, 1);
}