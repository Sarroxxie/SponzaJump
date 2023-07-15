#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../../../src/rendering/host_device.h"

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = eLighting) uniform _LightingInformation {LightingInformation lightingInformation; };

layout(set = 1, binding = eSkybox) uniform samplerCube skybox;
layout(set = 1, binding = eIrradiance) uniform samplerCube irradianceMap;
layout(set = 1, binding = eRadiance) uniform samplerCube radianceMap;

layout( push_constant ) uniform _SkyboxPushConstant { SkyboxPushConstant pushConstant; };

layout(location = 0) out vec4 outColor;

void main() {
    vec3 color = texture(skybox, inPosition - lightingInformation.cameraPosition).rgb;
    color = textureLod(radianceMap, inPosition - lightingInformation.cameraPosition, 4).rgb;

    // exposure tone mapping
    color = vec3(1.0) - exp(-color * pushConstant.exposure);

    outColor = vec4(color, 1);
}