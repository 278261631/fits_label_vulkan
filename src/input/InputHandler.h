#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Camera;
class UI;

class InputHandler {
public:
    InputHandler(GLFWwindow* window, Camera* camera, UI* ui = nullptr);
    ~InputHandler() = default;

    void init();
    void pollEvents();
    void setUI(UI* ui);

private:
    // 鼠标事件回调
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    // 键盘事件回调
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    // 窗口大小变化回调
    static void windowSizeCallback(GLFWwindow* window, int width, int height);

    // UI交互检测辅助方法
    bool isUIInteraction() const;

    GLFWwindow* m_window;
    Camera* m_camera;
    UI* m_ui;

    // 鼠标状态
    bool m_isRotating;
    bool m_isPanning;
    glm::vec2 m_lastMousePos;

    // 静态实例指针，用于回调函数访问
    static InputHandler* s_instance;
};
