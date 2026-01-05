#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <glm/glm.hpp>

class VulkanContext;
class Camera;

class CoordinateSystem {
public:
    CoordinateSystem(VulkanContext* vulkanContext, Camera* camera);
    ~CoordinateSystem();

    bool init();
    void draw(VkCommandBuffer commandBuffer);
    void cleanup();

private:
    void createVertexBuffer();
    void createPipeline();
    void createDescriptorSet();

    VulkanContext* m_vulkanContext;
    Camera* m_camera;
    
    // 顶点数据
    std::vector<glm::vec3> m_vertices;
    std::vector<uint32_t> m_indices;
    
    // Vulkan资源
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferMemory;
    
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
};