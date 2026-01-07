#include "ShaderCompiler.h"
#include "Logger.h"
#include <fstream>
#include <stdexcept>

std::vector<uint32_t> ShaderCompiler::loadSPIRV(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + filename);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());

    // SPIR-V is uint32_t aligned
    if (fileSize % 4 != 0) {
        throw std::runtime_error("Invalid SPIR-V file size: " + filename);
    }

    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();

    Logger::debug("Loaded shader: {} ({} bytes)", filename, fileSize);

    return buffer;
}

VkShaderModule ShaderCompiler::createShaderModule(VkDevice device, const std::vector<uint32_t>& spirv) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirv.size() * sizeof(uint32_t);
    createInfo.pCode = spirv.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }

    return shaderModule;
}

VkShaderModule ShaderCompiler::loadAndCreateModule(VkDevice device, const std::string& filename) {
    auto spirv = loadSPIRV(filename);
    return createShaderModule(device, spirv);
}

