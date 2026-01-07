#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <glm/glm.hpp>

class VulkanContext;
class Camera;

// Grid vertex structure
struct GridVertex {
    glm::vec3 position;
    glm::vec3 color;
};

class GridRenderer {
public:
    GridRenderer(VulkanContext* vulkanContext, Camera* camera);
    ~GridRenderer();

    bool init();
    void draw(VkCommandBuffer commandBuffer);
    void cleanup();

    // Settings
    void setGridSize(float size) { m_gridSize = size; m_needsRebuild = true; }
    void setGridSpacing(float spacing) { m_gridSpacing = spacing; m_needsRebuild = true; }
    void setShowLabels(bool show) { m_showLabels = show; }
    
    float getGridSize() const { return m_gridSize; }
    float getGridSpacing() const { return m_gridSpacing; }
    bool getShowLabels() const { return m_showLabels; }

private:
    void generateVertexData();
    void createVertexBuffer();
    void createShaderModules();
    void createPipelineLayout();
    void createGraphicsPipeline();
    void rebuildIfNeeded();

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    
    VulkanContext* m_vulkanContext;
    Camera* m_camera;
    
    // Vertex data
    std::vector<GridVertex> m_vertices;
    
    // Vulkan resources
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
    
    // Shaders
    VkShaderModule m_vertexShaderModule;
    VkShaderModule m_fragmentShaderModule;
    
    // Pipeline
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    
    // Settings
    float m_gridSize = 10.0f;
    float m_gridSpacing = 1.0f;
    bool m_showLabels = true;
    bool m_needsRebuild = false;
};

