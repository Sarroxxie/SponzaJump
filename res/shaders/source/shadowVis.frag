#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../../../src/rendering/host_device.h"

layout(location = 0) in vec2 fragTexCoords;

layout(set = 0, binding = eShadowDepthBuffer) uniform sampler2D depthSampler;

layout(location = 0) out vec4 outColor;

void main() {
    float val = texture(depthSampler, fragTexCoords).x;
    outColor = vec4(vec3(1 - val), 1);
    // outColor = vec4(fragTexCoords, 0, 1);
}