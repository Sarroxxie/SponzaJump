#ifndef GRAPHICSPRAKTIKUM_RENDERUTILS_H
#define GRAPHICSPRAKTIKUM_RENDERUTILS_H

#include "RenderContext.h"

void calculateShadowCascades(PerspectiveSettings   perspectiveSettings,
                             glm::mat4             inverseViewProjection,
                             ShadowMappingSettings shadowSettings,
                             SceneTransform*       sceneTransforms,
                             float*                splitDepths);

#endif  // GRAPHICSPRAKTIKUM_RENDERUTILS_H
