#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <glm/glm.hpp>

class VulkanContext;
class Camera;

// Grid vertex structure
struct GridVertex {
    glm::vec3 position;
    glm::vec3 color;
};

// Label info for 3D text rendering
struct GridLabel {
    glm::vec3 worldPos;
    std::string text;
    glm::vec3 color;

    GridLabel(const glm::vec3& pos, const std::string& txt, const glm::vec3& col)
        : worldPos(pos), text(txt), color(col) {}
};

class GridRenderer {
public:
    GridRenderer(VulkanContext* vulkanContext, Camera* camera);
    ~GridRenderer();

    bool init();
    void draw(VkCommandBuffer commandBuffer);
    void drawLabels();  // Call after ImGui::NewFrame, before ImGui::Render
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
    void generateLabels();
    void createVertexBuffer();
    void createShaderModules();
    void createPipelineLayout();
    void createGraphicsPipeline();
    void rebuildIfNeeded();
    glm::vec2 worldToScreen(const glm::vec3& worldPos);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VulkanContext* m_vulkanContext;
    Camera* m_camera;

    // Vertex data
    std::vector<GridVertex> m_vertices;

    // Labels for axis values
    std::vector<GridLabel> m_labels;

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

