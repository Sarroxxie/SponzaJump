#include <stdexcept>
#include <filesystem>
#include "Shader.h"
#include "utils/FileUtils.h"

VkShaderModule createShaderModule(const VulkanBaseContext &context, const Shader &shader) {
    std::vector<char> shaderCode;

    std::string fullCompiledName = shader.spvDirectory + shader.getCompiledName();

    if (!std::filesystem::exists(fullCompiledName)) {
        compileShader(shader);
    }

    if (!readFile(fullCompiledName, shaderCode)) {
        throw std::runtime_error("failed to open vertex shader code!");
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(shaderCode.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(context.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

void compileShader(const Shader &shader) {
    std::string command = std::string(VULKAN_GLSLANG_VALIDATOR_PATH)
                          + " -g --target-env vulkan1.1 -o \""
                          + shader.spvDirectory + shader.getCompiledName() + "\" \""
                          + shader.sourceDirectory + shader.shaderSourceName + "\"";
    // this suppresses the console output from the command (command differs on windows and unix)
    /*#if defined(_WIN32) || defined(_WIN64)
        command += " > nul";
    #else
        command += " > /dev/null";
    #endif*/
    system(command.c_str());
}