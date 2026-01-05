#pragma once

class VulkanContext;
class Renderer;
class Camera;

class PluginContext {
public:
    PluginContext(VulkanContext* vulkanContext, Renderer* renderer, Camera* camera);
    
    VulkanContext* getVulkanContext() const { return m_vulkanContext; }
    Renderer* getRenderer() const { return m_renderer; }
    Camera* getCamera() const { return m_camera; }
    
private:
    VulkanContext* m_vulkanContext;
    Renderer* m_renderer;
    Camera* m_camera;
};
