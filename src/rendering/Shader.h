#ifndef GRAPHICSPRAKTIKUM_SHADER_H
#define GRAPHICSPRAKTIKUM_SHADER_H

#include <string>
#include "vulkan/ApplicationContext.h"

enum ShaderStage {
    VERTEX_SHADER,
    FRAGMENT_SHADER
};

typedef struct shader_s
{
    ShaderStage shaderStage;
    std::string shaderSourceName;

    std::string sourceDirectory; // Directories need trailing slash
    std::string spvDirectory;

    [[nodiscard]] std::string getCompiledName() const {
        return shaderSourceName + ".spv";
    }
} Shader;

VkShaderModule createShaderModule(const VulkanBaseContext &context, const Shader &shader);

void compileShader(const Shader &shader);

#endif //GRAPHICSPRAKTIKUM_SHADER_H
