#include <iostream>
#include "window.h"
#include "vulkan/VulkanSetup.h"
#include "vulkan/VulkanRenderer.h"

#include "utils/FileUtils.h"
#include "scene/RenderableObject.h"
#include "rendering/RenderSetup.h"

#include "tiny_gltf.h"
#include "stb_image.h"

#include "scene/ModelLoader.h"

#define DEFAULT_APPLICATION_WIDTH 800
#define DEFAULT_APPLICATION_HEIGHT 600
#define DEFAULT_APPLICATION_NAME "GraphicsPraktikum"

/**
* TODO: keep track of uploaded textures somewhere in a map (uri + TextureStruct)
*       -> probably create a class to handle all model loading
*       -> will need to create a proper desriptor for objects and textures so the data can get stored
*       -> use tinygltf only for initial loading, scrap the "tinygltf::model" after loading
*       -> maybe name class "sceneResourceManager"
*/
bool gpuImageLoading(tinygltf::Image*     image,
              const int            image_idx,
              std::string*         err,
              std::string*         warn,
              int                  req_width,
              int                  req_height,
              const unsigned char* bytes,
              int                  size,
              void* user_data) {
    // TODO:
    // 1. check if the uri is already registered (image->uri is already set at this point)

    // 2. use stbi_load_from_memory() -> use the memory content from "bytes"

    // 3. upload the image to the GPU and register it (with URI as identifier) -> same structure as noted in the todo above

    // 4. use stbi_image_free() to free image from CPU RAM

    //tinygltf::LoadImageData(image, image_idx, err, warn, req_width, req_height, bytes, size, user_data);
    return true;
}

// TODO: pass flags like "NO_WARNINGS" and "NO_ERRORS" to disable console output
bool loadModel(tinygltf::Model &model, const char *filename) {
    tinygltf::TinyGLTF loader;
    std::string        errors;
    std::string        warnings;
    bool               success;

    // set custom image loading function
    loader.SetImageLoader(gpuImageLoading, nullptr);
    success = loader.LoadASCIIFromFile(&model, &errors, &warnings, filename);

    if(!warnings.empty()) {
        printf("Warn: %s\n", warnings.c_str());
    }

    if(!errors.empty()) {
        printf("Err: %s\n", errors.c_str());
    }

    if(!success) {
        printf("Failed to parse glTF\n");
    }
    return success;
}

unsigned char bytesToUnsignedChar2(unsigned char* address) {
    unsigned char out;
    memcpy(&out, address, sizeof(unsigned char));
    return out;
}


int main() {
    // tinygltf test code
    const char*  modelPath = "res/assets/models/debug_cube/debug_cube.gltf";
    tinygltf::Model    model;
    loadModel(model, modelPath);
    
    /* for(tinygltf::Image image : model.images) {
        // manually load images this way
        int    w, h, c;
        std::string path      = "res/assets/debug_model/" + image.uri;
        stbi_uc* stbiImage = stbi_load(path.c_str(),
                                       &w, &h, &c, 4);
        std::cout << "image name: " << image.name << "\n";
        std::cout << "image component: " << image.component << "\n";
        image.width = w;
        image.height = h;
        image.component = c;
        std::cout << "image width: " << image.width << "\n";

        stbi_image_free(stbiImage);
        std::cout << "\n";
    }*/

    ModelLoader loader;
    loader.loadModel(modelPath);
    std::cout << loader.meshes.size() << "\n";
    std::cout << loader.meshes[0].verticesCount << "\n";
    std::cout << loader.meshes[0].indicesCount << "\n";

    return 0;

    Window window = Window(DEFAULT_APPLICATION_WIDTH,
                           DEFAULT_APPLICATION_HEIGHT, DEFAULT_APPLICATION_NAME);

    ApplicationVulkanContext appContext;
    appContext.window = &window;
    initializeGraphicsApplication(appContext);

    RenderContext renderContext;
    initializeSimpleSceneRenderContext(appContext, renderContext);

    Scene scene(appContext.baseContext, renderContext);

    /*
    scene.addObject(createObject(appContext.baseContext,
                                 appContext.commandContext,
                                 COLORED_PYRAMID,
                                 {glm::vec3(1, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0.5)}));

    scene.addObject(createObject(appContext.baseContext,
                                 appContext.commandContext,
                                 COLORED_CUBE_DEF,
                                 {glm::vec3(-1, 0, 0), glm::vec3(0, glm::radians(45.0f), 0), glm::vec3(0.5)}));
    */

    int halfCountPerDimension = 2;
    int spacing = 5;
    for (int i = -halfCountPerDimension; i <= halfCountPerDimension; i++) {
        for (int j = -halfCountPerDimension; j <= halfCountPerDimension; j++) {
            for (int k = -halfCountPerDimension; k <= halfCountPerDimension; k++) {
                scene.addObject(createObject(appContext.baseContext,
                                             appContext.commandContext,
                                             COLORED_CUBE_DEF,
                                             {glm::vec3(j * spacing, k * spacing, i * spacing),
                                              glm::vec3(0, glm::radians(45.0f), 0), glm::vec3(0.5)}));
            }
        }
    }

    VulkanRenderer renderer(appContext, renderContext);

    // passes reference to the renderer to the key callback function
    glfwSetWindowUserPointer(window.getWindowHandle(), (void *) &renderer);

    if (renderContext.usesImgui) {
        ImGui_ImplGlfw_InitForVulkan(window.getWindowHandle(), true);
    }

    while (!glfwWindowShouldClose(window.getWindowHandle())) {
        glfwPollEvents();

        if (renderContext.usesImgui) {
            // @IMGUI
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
            ImGui::Begin("Statistics", 0,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove
                             | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMouseInputs
                             | ImGuiWindowFlags_NoTitleBar);
            ImGui::Text("%.3f ms", 1000.0f / ImGui::GetIO().Framerate);
            ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
            ImGui::SetWindowSize(ImVec2(0,0), ImGuiCond_Once);
            ImGui::End();
        }

        renderer.render(scene);
    }
    vkDeviceWaitIdle(appContext.baseContext.device);

    scene.cleanup(appContext.baseContext);
    renderer.cleanVulkanRessources();
    cleanupRenderContext(appContext.baseContext, renderContext);
    cleanupVulkanApplication(appContext);

    return 0;
}
