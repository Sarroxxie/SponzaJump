#include "RenderUtils.h"


// Done based on
// 1.
// https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-10-parallel-split-shadow-maps-programmable-gpus
// 2. Sasha Willems cascaded shadow mapping example in https://github.com/SaschaWillems/Vulkan
void calculateShadowCascades(PerspectiveSettings   perspectiveSettings,
                             glm::mat4             inverseViewProjection,
                             ShadowMappingSettings shadowSettings,
                             glm::mat4*            VPMats,
                             glm::mat4*            invVPMats,
                             SplitDummyStruct*                splitDepths) {

    int   numberCascades = shadowSettings.numberCascades;
    float cascadeSplits[numberCascades];

    float near      = perspectiveSettings.nearPlane;
    float far       = perspectiveSettings.farPlane;
    float clipRange = far - near;

    for(int i = 0; i < numberCascades; i++) {
        float i_over_m = static_cast<float>(i + 1) / static_cast<float>(numberCascades);

        float C_log     = near * powf(far / near, i_over_m);
        float C_uniform = near + (far - near) * i_over_m;

        float unnormalizedDist = shadowSettings.cascadeSplitsBlendFactor * C_log
                                 + (1 - shadowSettings.cascadeSplitsBlendFactor) * C_uniform;

        cascadeSplits[i] = unnormalizedDist / clipRange;
    }


    float lastSplitDist = 0.0;
    for(uint32_t i = 0; i < numberCascades; i++) {
        float splitDist = cascadeSplits[i];

        glm::vec3 frustumCorners[8] = {
            glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(-1.0f, -1.0f, 0.0f),
            glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(-1.0f, -1.0f, 1.0f),
        };

        // From here code copied from Sasha Willems and adjusted

        // Project frustum corners into world space
        for(uint32_t i = 0; i < 8; i++) {
            glm::vec4 invCorner =
                inverseViewProjection * glm::vec4(frustumCorners[i], 1.0f);
            frustumCorners[i] = invCorner / invCorner.w;
        }

        for(uint32_t i = 0; i < 4; i++) {
            glm::vec3 dist        = frustumCorners[i + 4] - frustumCorners[i];
            frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
            frustumCorners[i]     = frustumCorners[i] + (dist * lastSplitDist);
        }

        // Get frustum center
        glm::vec3 frustumCenter = glm::vec3(0.0f);
        for(uint32_t i = 0; i < 8; i++) {
            frustumCenter += frustumCorners[i];
        }
        frustumCenter /= 8.0f;

        float radius = 0.0f;
        for(uint32_t i = 0; i < 8; i++) {
            float distance = glm::length(frustumCorners[i] - frustumCenter);
            radius         = glm::max(radius, distance);
        }
        radius = std::ceil(radius * 16.0f) / 16.0f;

        glm::vec3 maxExtents = glm::vec3(radius);
        glm::vec3 minExtents = -maxExtents;

        shadowSettings.lightCamera.normalizeViewDir();
        glm::vec3 lightDir = shadowSettings.lightCamera.getViewDir();

        // Store split distance and matrix in cascade
        splitDepths[i].splitVal = (near + splitDist * clipRange) * -1.0f;

        glm::mat4 view = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter,
                        glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 ortho = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

        ortho[1][1] *= -1;

        VPMats[i] = ortho * view;
        invVPMats[i] = VPMats[i];

        lastSplitDist = cascadeSplits[i];
    }
}
