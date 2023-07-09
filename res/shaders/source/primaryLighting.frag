#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../../../src/rendering/host_device.h"

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec4 outColor;


layout(set = 0, binding = eLighting) uniform _LightingInformation {LightingInformation lightingInformation; };

// gBuffer
layout (set = 2, binding = ePosition) uniform sampler2D gBufferPosition;
layout (set = 2, binding = eNormal) uniform sampler2D gBufferNormal;
layout (set = 2, binding = eAlbedo) uniform sampler2D gBufferAlbedo;
layout (set = 2, binding = ePBR) uniform sampler2D gBufferPBR;
layout (set = 2, binding = eDepth) uniform sampler2D gBufferDepth;

void main() {
    // pixel coordinates (ranging from (0,0) to (width, height)) for sampling from gBuffer
    ivec2 intCoords = ivec2(gl_FragCoord.xy - 0.5);
    outColor = vec4(1,0,0,1);
}