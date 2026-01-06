#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <glm/glm.hpp>

class VulkanContext;
class Camera;

// 顶点数据结构
struct CoordinateVertex {
    glm::vec3 position;
    glm::vec3 color;
};

class CoordinateSystemRenderer {
public:
    CoordinateSystemRenderer(VulkanContext* vulkanContext, Camera* camera);
    ~CoordinateSystemRenderer();

    bool init();
    void draw(VkCommandBuffer commandBuffer);
    void cleanup();

private:
    // 初始化方法
    void generateVertexData();
    void createVertexBuffer();
    void createIndexBuffer();
    void createShaderModules();
    void createPipelineLayout();
    void createGraphicsPipeline();
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets();
    
    // 辅助方法
    VkShaderModule createShaderModule(const std::vector<char>& code);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    
    // 成员变量
    VulkanContext* m_vulkanContext;
    Camera* m_camera;
    
    // 顶点数据
    std::vector<CoordinateVertex> m_vertices;
    std::vector<uint32_t> m_indices;
    
    // Vulkan资源
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferMemory;
    
    // 着色器
    VkShaderModule m_vertexShaderModule;
    VkShaderModule m_fragmentShaderModule;
    
    // 管线
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    
    // 描述符
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorPool m_descriptorPool;
    VkDescriptorSet m_descriptorSet;
    
    // 统一缓冲区（用于MVP矩阵）
    VkBuffer m_uniformBuffer;
    VkDeviceMemory m_uniformBufferMemory;
    
    // 常量
    const float m_axisLength = 100.0f;
    const int m_tickCount = 10;
};