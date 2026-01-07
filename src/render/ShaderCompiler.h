#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

class ShaderCompiler {
public:
    enum class ShaderType {
        Vertex,
        Fragment,
        Geometry,
        Compute
    };

    // Load SPIR-V from file
    static std::vector<uint32_t> loadSPIRV(const std::string& filename);

    // Create Vulkan shader module from SPIR-V
    static VkShaderModule createShaderModule(VkDevice device, const std::vector<uint32_t>& spirv);

    // Convenience function: load SPIR-V and create shader module
    static VkShaderModule loadAndCreateModule(VkDevice device, const std::string& filename);
};

