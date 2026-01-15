#pragma once

#include <memory>
#include "VulkanContext.h"
#include "Camera.h"

class UI;
class CoordinateSystemRenderer;
class DemoObjectRenderer;
class GridRenderer;
class PointCloudRenderer;
class PluginContext;

class Renderer {
public:
    Renderer(VulkanContext* vulkanContext, Camera* camera, UI* ui);
    ~Renderer();

    bool init();
    void render();
    void cleanup();

    // Setters
    void setUI(UI* ui) { m_ui = ui; }
    void setPluginContext(PluginContext* ctx) { m_pluginContext = ctx; }

    // Getters
    VkDescriptorPool getDescriptorPool() const { return m_descriptorPool; }
    GridRenderer* getGridRenderer() const { return m_gridRenderer.get(); }
    PointCloudRenderer* getPointCloudRenderer() const { return m_pointCloudRenderer.get(); }

private:
    void drawFrame();
    void drawCoordinateSystem();
    void drawGrid();

    VulkanContext* m_vulkanContext;
    Camera* m_camera;
    UI* m_ui;
    PluginContext* m_pluginContext;
    VkDescriptorPool m_descriptorPool;

    std::unique_ptr<CoordinateSystemRenderer> m_coordinateRenderer;
    std::unique_ptr<DemoObjectRenderer> m_demoObjectRenderer;
    std::unique_ptr<GridRenderer> m_gridRenderer;
    std::unique_ptr<PointCloudRenderer> m_pointCloudRenderer;
};
