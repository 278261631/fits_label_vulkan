#pragma once

#include "VulkanContext.h"
#include "Renderer.h"
#include "Camera.h"

class UI {
public:
    UI(VulkanContext* vulkanContext, Renderer* renderer, Camera* camera);
    ~UI();

    bool init();
    void update();
    void cleanup();

private:
    void initImGui();
    void drawCoordinateSystem();
    void drawGrid();
    void drawControlPanel();

    VulkanContext* m_vulkanContext;
    Renderer* m_renderer;
    Camera* m_camera;
    bool m_showControlPanel;
};
