#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Camera;
class UI;
class PluginContext;

class InputHandler {
public:
    InputHandler(GLFWwindow* window, Camera* camera, UI* ui = nullptr);
    ~InputHandler() = default;

    void init();
    void pollEvents();
    void setUI(UI* ui);
    void setPluginContext(PluginContext* context) { m_pluginContext = context; }

private:
    // Mouse event callbacks
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    // Keyboard event callback
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    // Window resize callback
    static void windowSizeCallback(GLFWwindow* window, int width, int height);

    // UI interaction detection
    bool isUIInteraction() const;

    // Point picking - find closest point to mouse click
    void pickPoint(double mouseX, double mouseY);

    GLFWwindow* m_window;
    Camera* m_camera;
    UI* m_ui;
    PluginContext* m_pluginContext = nullptr;

    // Mouse state
    bool m_isRotating;
    bool m_isPanning;
    glm::vec2 m_lastMousePos;

    // Static instance pointer for callbacks
    static InputHandler* s_instance;
};
