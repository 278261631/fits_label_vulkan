#pragma once

#include <string>

class VulkanContext;
class Renderer;
class InputHandler;
class UI;
class Camera;

class Application {
public:
    Application(const std::string& title = "Vulkan Application", int width = 800, int height = 600);
    ~Application();

    void run();
    void shutdown();

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    const std::string& getTitle() const { return m_title; }

    VulkanContext* getVulkanContext() const { return m_vulkanContext; }
    Renderer* getRenderer() const { return m_renderer; }
    InputHandler* getInputHandler() const { return m_inputHandler; }
    UI* getUI() const { return m_ui; }
    Camera* getCamera() const { return m_camera; }

private:
    bool init();
    void mainLoop();

    std::string m_title;
    int m_width;
    int m_height;
    bool m_running;

    VulkanContext* m_vulkanContext;
    Renderer* m_renderer;
    InputHandler* m_inputHandler;
    UI* m_ui;
    Camera* m_camera;
};
