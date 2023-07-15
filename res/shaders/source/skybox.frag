#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../../../src/rendering/host_device.h"

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = eLighting) uniform _LightingInformation {LightingInformation lightingInformation; };

layout(set = 1, binding = eSkybox) uniform samplerCube skybox;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 color = texture(skybox, inPosition - lightingInformation.cameraPosition).rgb;

    // tone mapping
    color = color / (color + vec3(1.0));
    outColor = vec4(color, 1);
}