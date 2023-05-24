#ifndef GRAPHICSPRAKTIKUM_SCENE_H
#define GRAPHICSPRAKTIKUM_SCENE_H

#include <vulkan/vulkan_core.h>
#include <vector>
#include "vulkan/ApplicationContext.h"
#include "SceneSetup.h"
#include "RenderableObject.h"
#include "Camera.h"
#include "rendering/RenderContext.h"
#include "Model.h"
#include "ModelLoader.h"


class Scene {
private:
    std::vector<RenderableObject> objects;

    std::vector<Mesh>          meshes;
    std::vector<MeshPart>      meshParts;
    std::vector<Texture>       textures;
    std::vector<Material>      materials;
    std::vector<Model>         models;
    std::vector<ModelInstance> instances;

    Camera m_Camera;

    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    void* uniformBufferMapped;

    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;

public:
    Scene(VulkanBaseContext vulkanBaseContext, RenderContext &renderContext, Camera camera = Camera());

    void addObject(ModelLoader loader);

    std::vector<Mesh> &getMeshes();
    std::vector<MeshPart> &getMeshParts();
    std::vector<Texture> &getTextures();
    std::vector<Material> &getMaterials();
    std::vector<Model> &getModels();
    std::vector<ModelInstance> &getInstances();

    std::vector<RenderableObject> &getObjects();
    bool hasObject();

    Camera &getCameraRef();

    void cleanup(VulkanBaseContext &baseContext);

    void *getUniformBufferMapping();

    VkDescriptorSet *getDescriptorSet();

    void registerSceneImgui();

    float currentAngleY = 0;
    float currentAngleX = 0;

    float cameraAngleY = 0;
    float cameraDist = 45;

private:
    void createUniformBuffers(VulkanBaseContext vulkanBaseContext);

    void createDescriptorPool(VulkanBaseContext &baseContext);

    void createDescriptorSets(VulkanBaseContext &baseContext, RenderContext &renderContext);

};

#endif //GRAPHICSPRAKTIKUM_SCENE_H
