#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "../../../src/rendering/host_device.h"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangents;
layout(location = 4) in vec2 inTexCoords;

layout(location = 5) in vec4 inShadowCoords;

layout(location = 0) out vec4 outColor;

in vec4 gl_FragCoord ;

layout(set = 0, binding = eLighting) uniform _LightingInformation {LightingInformation lightingInformation; };

// materials array
layout(std140, set = 1, binding = eMaterials) buffer Materials {MaterialDescription m[];} materials;

layout(set = 1, binding = eTextures) uniform sampler2D samplers[];
layout(set = 1, binding = eSkybox) uniform samplerCube skybox;

layout(set = 2, binding = eShadowDepthBuffer) uniform sampler2D depthSampler;

layout( push_constant ) uniform _PushConstant { PushConstant pushConstant; };

// TODO: these light sources only serve as debug and will be replaced when deferred rendering is implemented
const int LIGHT_COUNT = 2;
const vec3 LIGHT_POS[LIGHT_COUNT] = {vec3(2.5, 3.5, 1.5),
                                    vec3(3.2, 3.0, -1.0)};
const vec3 LIGHT_COL[LIGHT_COUNT] = {vec3(0.9, 10.6, 2.9),
                                    vec3(0.0, 4.5, 12.7)};

const float PI = 3.14159265359;

// shader heavily based on https://learnopengl.com/PBR/Lighting
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------
vec3 BRDF(vec3 L, vec3 V, vec3 N, vec3 radiance, vec3 albedo, float metallic, float roughness) {
    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    vec3 H = normalize(V + L);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
    vec3 specular = numerator / denominator;

    // kS is equal to Fresnel
    vec3 kS = F;
    // for energy conservation, the diffuse and specular light can't
    // be above 1.0 (unless the surface emits light); to preserve this
    // relationship the diffuse component (kD) should equal 1.0 - kS.
    vec3 kD = vec3(1.0) - kS;
    // multiply kD by the inverse metalness such that only non-metals
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
    kD *= 1.0 - metallic;

    // scale light by NdotL
    float NdotL = max(dot(N, L), 0.0);

    // outgoing radiance
    // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

float getShadow(vec4 shadowCoords) {
    float shadow = 1.0;

    if ( shadowCoords.z > -1.0 && shadowCoords.z < 1.0 ) {
        float dist = texture( depthSampler, shadowCoords.st ).r;
        if ( shadowCoords.w > 0.0 && dist < shadowCoords.z )
        {
            shadow = 0.0;
        }
    }

    return shadow;
}

void main() {
    vec4 shadowCoordsHom = inShadowCoords / inShadowCoords.w;
    vec4 normalizedShadowCoords = vec4((shadowCoordsHom.xy + vec2(1)) / 2, shadowCoordsHom.ba);

    float shadow = getShadow(normalizedShadowCoords);

    // fetch material
    MaterialDescription material = materials.m[pushConstant.materialIndex];

    // apply normal mappingd
    vec3 N = normalize(inNormal);
	vec3 T = normalize(inTangents.xyz);
    vec3 B = cross(N, T) * inTangents.w;
	mat3 TBN = mat3(T, B, N);
	vec3 normal = texture(samplers[material.normalTextureID], inTexCoords).rgb * 2.0 - vec3(1.0);
    normal = normalize(TBN * normal);

    // fetch PBR textures
    vec4 albedoTexture = texture(samplers[material.albedoTextureID], inTexCoords);
    vec3 aoRoughnessMetallic = texture(samplers[material.aoRoughnessMetallicTextureID], inTexCoords).rgb;

    vec3 albedo = pow(albedoTexture.rgb, vec3(2.2, 2.2, 2.2)); // convert color into linear space
    float alpha = albedoTexture.a;
    float ao = aoRoughnessMetallic.r;
    float roughness = aoRoughnessMetallic.g;
    float metallic = aoRoughnessMetallic.b;

    vec3 Lo = vec3(0.0);

    vec3 V = normalize(lightingInformation.cameraPosition - inPosition);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // directional light source
    if (shadow > 0) {
        vec3 L = normalize(-lightingInformation.lightDirection);
        Lo += BRDF(L, V, normal, lightingInformation.lightIntensity, albedo, metallic, roughness) * shadow;
    }

    // iterate over omnidirectional light sources
    for (int i = 0; i < LIGHT_COUNT; i++) {
        vec3 L = normalize(LIGHT_POS[i] - inPosition);

        float distance = length(LIGHT_POS[i] - inPosition);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = LIGHT_COL[i] * attenuation;

        Lo += BRDF(L, V, normal, radiance, albedo, metallic, roughness);
    }

    // ambient lighting
    vec3 ambient = vec3(0.0002) * albedo * ao;

    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2));

    outColor = vec4(color, 1);

    //outColor = vec4(color.xyz * shadow, 1);

    // sample skybox
    //outColor = texture(skybox, normalize(inPosition - lightingInformation.cameraPosition));
}
