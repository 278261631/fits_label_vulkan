#pragma once

#include "VulkanContext.h"
#include "Camera.h"

class Renderer;

class UI {
public:
    UI(VulkanContext* vulkanContext, Renderer* renderer, Camera* camera);
    ~UI();

    bool init();
    void update();
    void cleanup();
    VkDescriptorPool getDescriptorPool() const;

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
