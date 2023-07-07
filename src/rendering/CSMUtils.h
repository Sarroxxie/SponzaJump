#ifndef GRAPHICSPRAKTIKUM_CSMUTILS_H
#define GRAPHICSPRAKTIKUM_CSMUTILS_H

#include "RenderContext.h"

// Utilities for Cascaded Shadow Mapping
void calculateShadowCascades(PerspectiveSettings   perspectiveSettings,
                             glm::mat4             inverseViewProjection,
                             ShadowMappingSettings shadowSettings,
                             glm::vec3             viewDir,
                             glm::mat4*            VPMats,
                             SplitDummyStruct*     splitDepths);

void calculateCascadeSplitDepths(PerspectiveSettings perspectiveSettings,
                                 int                 numberCascades,
                                 float               cascadeSplits[],
                                 float               splitsBlendFactor = 0.5);

void calculateVPMatsSashaWillems(PerspectiveSettings   perspectiveSettings,
                                 glm::mat4             inverseViewProjection,
                                 ShadowMappingSettings shadowSettings,
                                 const float           cascadeSplits[],
                                 glm::mat4*            VPMats,
                                 SplitDummyStruct*     splitDepths);

void calculateVPMatsNew(PerspectiveSettings   perspectiveSettings,
                        glm::mat4             inverseViewProjection,
                        ShadowMappingSettings shadowSettings,
                        const float           cascadeSplits[],
                        glm::vec3             viewDir,
                        glm::mat4*            VPMats,
                        SplitDummyStruct*     splitDepths);

#endif  // GRAPHICSPRAKTIKUM_CSMUTILS_H
