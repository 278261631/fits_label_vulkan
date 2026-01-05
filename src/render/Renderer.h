#pragma once

#include "VulkanContext.h"
#include "Camera.h"

class UI;

class Renderer {
public:
    Renderer(VulkanContext* vulkanContext, Camera* camera, UI* ui);
    ~Renderer();

    bool init();
    void render();
    void cleanup();
    
    // Setters
    void setUI(UI* ui) { m_ui = ui; }
    
    // Getters
    VkDescriptorPool getDescriptorPool() const { return m_descriptorPool; }

private:
    void drawFrame();
    void drawCoordinateSystem();
    void drawGrid();

    VulkanContext* m_vulkanContext;
    Camera* m_camera;
    UI* m_ui;
    VkDescriptorPool m_descriptorPool;
};
