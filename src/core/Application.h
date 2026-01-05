#pragma once

#include <string>
#include <chrono>
#include <memory>

class VulkanContext;
class Renderer;
class InputHandler;
class UI;
class Camera;
class Config;
class PluginManager;
class PluginContext;

class Application {
public:
    Application(const std::string& title = "Vulkan Application", int width = 800, int height = 600);
    ~Application();

    void run();
    void shutdown();

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    const std::string& getTitle() const { return m_title; }

    VulkanContext* getVulkanContext() const { return m_vulkanContext.get(); }
    Renderer* getRenderer() const { return m_renderer.get(); }
    InputHandler* getInputHandler() const { return m_inputHandler.get(); }
    UI* getUI() const { return m_ui.get(); }
    Camera* getCamera() const { return m_camera.get(); }

private:
    bool init();
    void mainLoop();

    std::string m_title;
    int m_width;
    int m_height;
    bool m_running;

    std::unique_ptr<VulkanContext> m_vulkanContext;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<InputHandler> m_inputHandler;
    std::unique_ptr<UI> m_ui;
    std::unique_ptr<Camera> m_camera;
    
    std::unique_ptr<PluginContext> m_pluginContext;
    std::unique_ptr<PluginManager> m_pluginManager;
};
