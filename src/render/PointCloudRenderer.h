#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <glm/glm.hpp>

class VulkanContext;
class Camera;
class PluginContext;

struct PointVertex {
    glm::vec3 position;
    glm::vec3 color;
    float size;
};

class PointCloudRenderer {
public:
    PointCloudRenderer(VulkanContext* vulkanContext, Camera* camera);
    ~PointCloudRenderer();

    bool init();
    void updatePoints(PluginContext* pluginContext);
    void draw(VkCommandBuffer commandBuffer);
    void cleanup();

private:
    void createVertexBuffer();
    void createShaderModules();
    void createPipelineLayout();
    void createGraphicsPipeline();
    
    VkShaderModule createShaderModule(const std::vector<char>& code);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    
    VulkanContext* m_vulkanContext;
    Camera* m_camera;
    
    std::vector<PointVertex> m_vertices;
    
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
    
    VkShaderModule m_vertexShaderModule;
    VkShaderModule m_fragmentShaderModule;
    
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    
    bool m_initialized;
    bool m_hasData;
};

