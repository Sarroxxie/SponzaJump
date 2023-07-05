#include "CSMUtils.h"


// Done based on
// 1.
// https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-10-parallel-split-shadow-maps-programmable-gpus
/*
 // Build a matrix for cropping light's projection
 // Given vectors are in light's clip space Matrix
 Light::CalculateCropMatrix(Frustum splitFrustum)
 {
    Matrix lightViewProjMatrix = viewMatrix * projMatrix;
    // Find boundaries in light's clip space
    BoundingBox cropBB = CreateAABB(splitFrustum.AABB, lightViewProjMatrix);
    // Use default near-plane value
    cropBB.min.z = 0.0f;

    // Create the crop matrix
    float scaleX, scaleY, scaleZ;
    float offsetX, offsetY, offsetZ;

    scaleX = 2.0f / (cropBB.max.x - cropBB.min.x);
    scaleY = 2.0f / (cropBB.max.y - cropBB.min.y);

    offsetX = -0.5f * (cropBB.max.x + cropBB.min.x) * scaleX;
    offsetY = -0.5f * (cropBB.max.y + cropBB.min.y) * scaleY;


    scaleZ = 1.0f / (cropBB.max.z - cropBB.min.z);
    offsetZ = -cropBB.min.z * scaleZ;

    return Matrix(  scaleX,     0.0f,       0.0f,       0.0f,
                    0.0f,       scaleY,     0.0f,       0.0f,
                    0.0f,       0.0f,       scaleZ,     0.0f,
                    offsetX,    offsetY,    offsetZ,    1.0f);
}

 *
 */

// 2. Sasha Willems cascaded shadow mapping example in https://github.com/SaschaWillems/Vulkan
void calculateShadowCascades(PerspectiveSettings   perspectiveSettings,
                             glm::mat4             inverseViewProjection,
                             ShadowMappingSettings shadowSettings,
                             glm::vec3 viewDir,
                             glm::mat4*            VPMats,
                             SplitDummyStruct*     splitDepths) {

    int   numberCascades = shadowSettings.numberCascades;
    float cascadeSplits[numberCascades];

    calculateCascadeSplitDepths(perspectiveSettings, numberCascades, cascadeSplits,
                                shadowSettings.cascadeSplitsBlendFactor);

    if(shadowSettings.newCascadeCalculation) {
        calculateVPMatsNew(perspectiveSettings, inverseViewProjection,
                           shadowSettings, cascadeSplits, viewDir, VPMats, splitDepths);
    } else {
        calculateVPMatsSashaWillems(perspectiveSettings, inverseViewProjection,
                                    shadowSettings, cascadeSplits, VPMats, splitDepths);
    }
}
void calculateCascadeSplitDepths(PerspectiveSettings perspectiveSettings,
                                 int                 numberCascades,
                                 float*              cascadeSplits,
                                 float               splitsBlendFactor) {

    float near      = perspectiveSettings.nearPlane;
    float far       = perspectiveSettings.farPlane;
    float clipRange = far - near;

    for(int i = 0; i < numberCascades; i++) {
        float i_over_m = static_cast<float>(i + 1) / static_cast<float>(numberCascades);

        float C_log     = near * powf(far / near, i_over_m);
        float C_uniform = near + (far - near) * i_over_m;

        float unnormalizedDist =
            splitsBlendFactor * C_log + (1 - splitsBlendFactor) * C_uniform;

        cascadeSplits[i] = unnormalizedDist / clipRange;
    }
}
void calculateVPMatsSashaWillems(PerspectiveSettings   perspectiveSettings,
                                 glm::mat4             inverseViewProjection,
                                 ShadowMappingSettings shadowSettings,
                                 const float*          cascadeSplits,
                                 glm::mat4*            VPMats,
                                 SplitDummyStruct*     splitDepths) {
    float near      = perspectiveSettings.nearPlane;
    float far       = perspectiveSettings.farPlane;
    float clipRange = far - near;

    float lastSplitDist = 0.0;
    for(uint32_t i = 0; i < shadowSettings.numberCascades; i++) {
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

        glm::mat4 view = glm::lookAt(frustumCenter - lightDir * -minExtents.z,
                                     frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 ortho =
            glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y,
                       0.0f, maxExtents.z - minExtents.z);

        ortho[1][1] *= -1;

        VPMats[i] = ortho * view;

        lastSplitDist = cascadeSplits[i];
    }
}
void calculateVPMatsNew(PerspectiveSettings   perspectiveSettings,
                        glm::mat4             inverseViewProjection,
                        ShadowMappingSettings shadowSettings,
                        const float*          cascadeSplits,
                        glm::vec3 viewDir,
                        glm::mat4*            VPMats,
                        SplitDummyStruct*     splitDepths) {
    int numberCascades = shadowSettings.numberCascades;

    float near      = perspectiveSettings.nearPlane;
    float far       = perspectiveSettings.farPlane;
    float clipRange = far - near;

    glm::vec3 frustumCorners[8] = {
        glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f),
        glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(-1.0f, -1.0f, 0.0f),
        glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f),
        glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(-1.0f, -1.0f, 1.0f),
    };

    // 1. Calculate Frustum Corner in World Space
    for(int i = 0; i < 8; i++) {
        glm::vec4 frustumCornerHom =
            inverseViewProjection * glm::vec4(frustumCorners[i], 1);
        frustumCorners[i] = glm::vec3(frustumCornerHom / frustumCornerHom.w);
    }

    shadowSettings.lightCamera.normalizeViewDir();
    glm::vec3 lightDir = shadowSettings.lightCamera.getViewDir();

    glm::vec3 upVec = glm::vec3(0, 1, 0);  // TODO can set this to cross product
                                           // of camera dir and light dir ?

    if (shadowSettings.crossProductUp)
        upVec = glm::cross(viewDir, lightDir);

    glm::vec3 frustumCenters[numberCascades];

    glm::vec3 minExtents[numberCascades];
    glm::vec3 maxExtents[numberCascades];

    float maxDistAlongLightDir = -INFINITY;
    float globalZMin           = INFINITY;

    float lastSplitDepth = 0;
    for(int i = 0; i < numberCascades; i++) {
        float currentSplitDepth = cascadeSplits[i];

        // 2. Split Frustum to cascade Frustum
        glm::vec3 cascadeLocalFrustumCorners[8];

        for(int j = 0; j < 4; j++) {
            glm::vec3 cornerDiff = frustumCorners[j + 4] - frustumCorners[j];
            cascadeLocalFrustumCorners[j] =
                frustumCorners[j] + (lastSplitDepth * cornerDiff);
            cascadeLocalFrustumCorners[j + 4] =
                frustumCorners[j] + (currentSplitDepth * cornerDiff);
        }

        splitDepths[i].splitVal = (near + currentSplitDepth * clipRange) * -1.0f;
        lastSplitDepth = currentSplitDepth;

        // 3. Calculate Frustum Center
        frustumCenters[i] = glm::vec3(0);
        for(auto cascadeLocalFrustumCorner : cascadeLocalFrustumCorners) {
            frustumCenters[i] += cascadeLocalFrustumCorner;
        }
        frustumCenters[i] /= 8;


        // 4. Project Frustum Corners into Light Camera Space

        glm::mat4 cascadeView =
            glm::lookAt(frustumCenters[i] - (lightDir), frustumCenters[i], upVec);

        float startVal = INFINITY;

        minExtents[i] = glm::vec3(startVal);
        maxExtents[i] = glm::vec3(-startVal);


        for(auto& cascadeLocalFrustumCorner : cascadeLocalFrustumCorners) {
            glm::vec4 cascadeCornerHom =
                cascadeView * glm::vec4(cascadeLocalFrustumCorner, 1);

            glm::vec3 lightSpaceCorner = cascadeCornerHom / cascadeCornerHom.w;

            // 5. Calculate per dimension extents in light view space
            minExtents[i].x = glm::min(minExtents[i].x, lightSpaceCorner.x);
            minExtents[i].y = glm::min(minExtents[i].y, lightSpaceCorner.y);
            minExtents[i].z = glm::min(minExtents[i].z, lightSpaceCorner.z);

            // TODO this might be wrong, because camera might look into negative
            // z direction ?
            // -> Would mean I need to flip role of min and max extents
            // 1. minExtents would use > and glm::max to compare, still store
            // untransformed distance
            // 2. maxExtents would use < and glm::min to compare since values
            // are growing negative
            //  -> actually probably switch max and min meanings, since the
            //  vec3s are initialized with pos and neg infinity
            // 3. use abs val of maxExtents to then calculate far plane.
            // 4.


            maxExtents[i].x = glm::max(maxExtents[i].x, lightSpaceCorner.x);
            maxExtents[i].y = glm::max(maxExtents[i].y, lightSpaceCorner.y);

            // since the view direction is in negative z direction in cameraspace
            // largest z value means closest to camera (or behind it, in which
            // we can still use to translate camera along view dir later)
            if(lightSpaceCorner.z > maxExtents[i].z) {
                maxExtents[i].z = lightSpaceCorner.z;

                // use first frustum center as constant reference point,
                // calculate relative to other frustum centers later
                glm::vec3 diffVec = frustumCenters[0] - cascadeLocalFrustumCorner;

                // distance to frustum corner along light direction
                float dist = glm::dot(diffVec, lightDir);  // store the max of this

                maxDistAlongLightDir = glm::max(maxDistAlongLightDir, dist);
            }
        }
        // 6. Store z-min globally
        globalZMin = glm::min(globalZMin, minExtents[i].z);
    }

    glm::vec3 refPoint = frustumCenters[0] - (lightDir * maxDistAlongLightDir);

    // 7. Exit first loop, another loop to calculate final VP mats
    for(int i = 0; i < numberCascades; i++) {
        // 8. Calculate view mat and orthographic projection using min and max coords
        float nearPlaneOffset = 0.0;

        glm::vec3 cascadeDiff = frustumCenters[i] - refPoint;
        float     dist        = glm::dot(cascadeDiff, lightDir);

        glm::vec3 eye = frustumCenters[i] - (lightDir * dist + nearPlaneOffset);

        // using slightly different light transformation matrix doesn't matter
        // here, because we only change the camera distance and the projection is orthographic
        glm::mat4 cascadeView =
            glm::lookAt(eye,
                        frustumCenters[i], upVec);

        float farPlane = glm::abs(minExtents[i].z) - 1 + dist + nearPlaneOffset;

        glm::mat4 projection =
            glm::ortho(minExtents[i].x, maxExtents[i].x, minExtents[i].y,
                       maxExtents[i].y, nearPlaneOffset, farPlane);

        projection[1][1] *= -1;

        VPMats[i] = projection * cascadeView;
    }
}
