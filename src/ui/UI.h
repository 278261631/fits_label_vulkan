#pragma once

#include "VulkanContext.h"
#include "Camera.h"

class Renderer;
class GridRenderer;

class UI {
public:
    UI(VulkanContext* vulkanContext, Renderer* renderer, Camera* camera);
    ~UI();

    bool init();
    void update();
    void cleanup();
    VkDescriptorPool getDescriptorPool() const;

    void setGridRenderer(GridRenderer* gridRenderer) { m_gridRenderer = gridRenderer; }

private:
    void initImGui();
    void drawCoordinateSystem();
    void drawGrid();
    void drawControlPanel();

    VulkanContext* m_vulkanContext;
    Renderer* m_renderer;
    Camera* m_camera;
    GridRenderer* m_gridRenderer = nullptr;
    bool m_showControlPanel;
};
