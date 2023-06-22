#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../../../src/rendering/host_device.h"

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = eLighting) uniform _LightingInformation {LightingInformation lightingInformation; };

layout(set = 1, binding = eSkybox) uniform samplerCube skybox;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 color = texture(skybox, inPosition - lightingInformation.cameraPosition);
    color = texture(skybox, vec3(inPosition.xy, 0.5));
    outColor = vec4(inPosition.xy * 0.5 + 0.5, 0, 1);
    outColor = color;
}