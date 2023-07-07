#ifndef GRAPHICSPRAKTIKUM_CSMUTILS_H
#define GRAPHICSPRAKTIKUM_CSMUTILS_H

#include "RenderContext.h"

// Utilities for Cascaded Shadow Mapping
void calculateShadowCascades(PerspectiveSettings   perspectiveSettings,
                             glm::mat4             inverseViewProjection,
                             ShadowMappingSettings shadowSettings,
                             glm::vec3             viewDir,
                             std::vector<glm::mat4>& VPMats,
                             std::vector<SplitDummyStruct>& splitDepths);

void calculateCascadeSplitDepths(PerspectiveSettings perspectiveSettings,
                                 int                 numberCascades,
                                 std::vector<float>& cascadeSplits,
                                 float               splitsBlendFactor = 0.5);

void calculateVPMatsSashaWillems(PerspectiveSettings   perspectiveSettings,
                                 glm::mat4             inverseViewProjection,
                                 ShadowMappingSettings shadowSettings,
                                 std::vector<float>&   cascadeSplits,
                                 std::vector<glm::mat4>& VPMats,
                                 std::vector<SplitDummyStruct>& splitDepths);

void calculateVPMatsNew(PerspectiveSettings   perspectiveSettings,
                        glm::mat4             inverseViewProjection,
                        ShadowMappingSettings shadowSettings,
                        std::vector<float>&   cascadeSplits,
                        glm::vec3             viewDir,
                        std::vector<glm::mat4>&        VPMats,
                        std::vector<SplitDummyStruct>& splitDepths);

#endif  // GRAPHICSPRAKTIKUM_CSMUTILS_H
