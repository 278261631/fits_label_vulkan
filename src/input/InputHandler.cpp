#include "InputHandler.h"
#include "Camera.h"
#include <iostream>

// 静态实例初始化
InputHandler* InputHandler::s_instance = nullptr;

InputHandler::InputHandler(GLFWwindow* window, Camera* camera)
    : m_window(window), m_camera(camera),
      m_isRotating(false), m_isPanning(false),
      m_lastMousePos(0.0f, 0.0f) {
    s_instance = this;
}

void InputHandler::init() {
    // 设置GLFW回调函数
    glfwSetWindowUserPointer(m_window, this);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetCursorPosCallback(m_window, mouseMoveCallback);
    glfwSetScrollCallback(m_window, scrollCallback);
    glfwSetKeyCallback(m_window, keyCallback);
    glfwSetWindowSizeCallback(m_window, windowSizeCallback);
}

void InputHandler::pollEvents() {
    glfwPollEvents();
}

void InputHandler::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (s_instance) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                s_instance->m_isRotating = true;
                double xpos, ypos;
                glfwGetCursorPos(window, &xpos, &ypos);
                s_instance->m_lastMousePos = glm::vec2((float)xpos, (float)ypos);
            } else if (action == GLFW_RELEASE) {
                s_instance->m_isRotating = false;
            }
        } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            if (action == GLFW_PRESS) {
                s_instance->m_isPanning = true;
                double xpos, ypos;
                glfwGetCursorPos(window, &xpos, &ypos);
                s_instance->m_lastMousePos = glm::vec2((float)xpos, (float)ypos);
            } else if (action == GLFW_RELEASE) {
                s_instance->m_isPanning = false;
            }
        }
    }
}

void InputHandler::mouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
    if (s_instance) {
        glm::vec2 currentPos((float)xpos, (float)ypos);
        glm::vec2 delta = currentPos - s_instance->m_lastMousePos;

        if (s_instance->m_isRotating) {
            s_instance->m_camera->rotate(delta.x, delta.y);
        } else if (s_instance->m_isPanning) {
            s_instance->m_camera->pan(delta.x, delta.y);
        }

        s_instance->m_lastMousePos = currentPos;
    }
}

void InputHandler::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (s_instance) {
        s_instance->m_camera->zoom((float)yoffset);
    }
}

void InputHandler::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    // 添加更多键盘快捷键
    if (s_instance) {
        if (key == GLFW_KEY_R && action == GLFW_PRESS) {
            // 重置相机
            // 这里可以添加相机重置逻辑
        }
    }
}

void InputHandler::windowSizeCallback(GLFWwindow* window, int width, int height) {
    // 窗口大小变化处理
    // 这里可以添加窗口大小变化的逻辑
}
