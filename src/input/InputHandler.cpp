#include "InputHandler.h"
#include "Camera.h"
#include "UI.h"
#include "Logger.h"
#include <imgui.h>

#include <iostream>

// 静态实例初始化
InputHandler* InputHandler::s_instance = nullptr;

void InputHandler::setUI(UI* ui) {
    m_ui = ui;
}

InputHandler::InputHandler(GLFWwindow* window, Camera* camera, UI* ui)
    : m_window(window), m_camera(camera), m_ui(ui),
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

bool InputHandler::isUIInteraction() const {
    // 添加对m_ui指针的检查日志
    if (!m_ui) {
        Logger::trace("UI Interaction check: m_ui is null, returning false");
        return false;
    }
    
    ImGuiIO& io = ImGui::GetIO();
    
    // 检查是否有任何ImGui项目处于活动状态
    if (io.WantCaptureMouse) {
        Logger::trace("UI Interaction detected: io.WantCaptureMouse is true");
        return true;
    }
    
    // 检查是否有窗口正在被拖拽 - 使用更广泛的检测
    bool isDragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
    bool isWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
    bool isAnyItemActive = io.MouseDown[0] && (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsAnyItemHovered());
    
    if (isDragging) {
        // 如果正在拖拽，检查是否在窗口区域
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || isWindowFocused || isAnyItemActive) {
            Logger::trace("UI Interaction detected: Mouse dragging over window (Dragging: {}, WindowHovered: {}, WindowFocused: {}, AnyItemActive: {})", 
                        isDragging, ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow), isWindowFocused, isAnyItemActive);
            return true;
        }
    }
    
    // 检查是否有任何窗口被悬停或聚焦
    bool isWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
    if (isWindowHovered || isWindowFocused || isAnyItemActive) {
        Logger::trace("UI Interaction detected: Window hovered or focused or item active (Hovered: {}, Focused: {}, AnyItemActive: {})", 
                    isWindowHovered, isWindowFocused, isAnyItemActive);
        return true;
    }
    
    // 添加调试日志显示当前状态
    Logger::trace("UI Interaction check: No UI interaction detected (WantCaptureMouse: {}, Dragging: {}, WindowHovered: {}, WindowFocused: {}, AnyItemActive: {})", 
                io.WantCaptureMouse, isDragging, isWindowHovered, isWindowFocused, isAnyItemActive);
    
    return false;
}

void InputHandler::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (s_instance) {
        // 检查是否在UI元素上，如果是则不处理相机控制
        bool isUIInteraction = s_instance->isUIInteraction();
        
        if (action == GLFW_PRESS) {
            Logger::trace("Mouse button {} pressed - UI Interaction: {}", 
                         (button == GLFW_MOUSE_BUTTON_LEFT ? "LEFT" : 
                          button == GLFW_MOUSE_BUTTON_RIGHT ? "RIGHT" : "MIDDLE"), 
                         isUIInteraction ? "YES" : "NO");
        } else if (action == GLFW_RELEASE) {
            Logger::trace("Mouse button {} released - UI Interaction: {}", 
                         (button == GLFW_MOUSE_BUTTON_LEFT ? "LEFT" : 
                          button == GLFW_MOUSE_BUTTON_RIGHT ? "RIGHT" : "MIDDLE"), 
                         isUIInteraction ? "YES" : "NO");
        }
        
        // 如果是UI交互，需要重置相机控制状态
        if (isUIInteraction) {
            // 在UI交互开始时，重置所有相机控制状态
            bool wasRotating = s_instance->m_isRotating;
            bool wasPanning = s_instance->m_isPanning;
            s_instance->m_isRotating = false;
            s_instance->m_isPanning = false;
            
            if (wasRotating || wasPanning) {
                Logger::trace("Reset camera control state due to UI interaction (wasRotating: {}, wasPanning: {})", 
                             wasRotating, wasPanning);
            }
        } else {
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                if (action == GLFW_PRESS) {
                    s_instance->m_isRotating = true;
                    double xpos, ypos;
                    glfwGetCursorPos(window, &xpos, &ypos);
                    s_instance->m_lastMousePos = glm::vec2((float)xpos, (float)ypos);
                    Logger::trace("Started camera rotation (mouse pos: {}, {})", (float)xpos, (float)ypos);
                } else if (action == GLFW_RELEASE) {
                    s_instance->m_isRotating = false;
                    Logger::trace("Stopped camera rotation");
                }
            } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
                if (action == GLFW_PRESS) {
                    s_instance->m_isPanning = true;
                    double xpos, ypos;
                    glfwGetCursorPos(window, &xpos, &ypos);
                    s_instance->m_lastMousePos = glm::vec2((float)xpos, (float)ypos);
                    Logger::trace("Started camera panning (mouse pos: {}, {})", (float)xpos, (float)ypos);
                } else if (action == GLFW_RELEASE) {
                    s_instance->m_isPanning = false;
                    Logger::trace("Stopped camera panning");
                }
            }
        }
    }
}

void InputHandler::mouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
    if (s_instance) {
        glm::vec2 currentPos((float)xpos, (float)ypos);
        glm::vec2 delta = currentPos - s_instance->m_lastMousePos;

        // 检查是否在UI元素上，如果是则不处理相机控制
        bool isUIInteraction = s_instance->isUIInteraction();

        Logger::trace("Mouse move: ({}, {}) delta: ({}, {}) - UI Interaction: {} - Rotating: {} - Panning: {}", 
                     (float)xpos, (float)ypos, delta.x, delta.y, 
                     isUIInteraction ? "YES" : "NO",
                     s_instance->m_isRotating ? "YES" : "NO",
                     s_instance->m_isPanning ? "YES" : "NO");

        // 如果是UI交互，重置相机控制状态
        if (isUIInteraction) {
            bool wasRotating = s_instance->m_isRotating;
            bool wasPanning = s_instance->m_isPanning;
            s_instance->m_isRotating = false;
            s_instance->m_isPanning = false;
            
            if (wasRotating || wasPanning) {
                Logger::trace("Reset camera control state due to UI interaction during mouse move (wasRotating: {}, wasPanning: {})", 
                             wasRotating, wasPanning);
            }
        } else {
            if (s_instance->m_isRotating) {
                s_instance->m_camera->rotate(delta.x, delta.y);
                Logger::trace("Camera rotation applied: ({}, {})", delta.x, delta.y);
            } else if (s_instance->m_isPanning) {
                s_instance->m_camera->pan(delta.x, delta.y);
                Logger::trace("Camera panning applied: ({}, {})", delta.x, delta.y);
            }
        }

        s_instance->m_lastMousePos = currentPos;
    }
}

void InputHandler::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (s_instance) {
        // 检查是否在UI元素上，如果是则不处理相机控制
        bool isUIInteraction = s_instance->isUIInteraction();

        Logger::trace("Mouse scroll: ({}, {}) - UI Interaction: {}", xoffset, yoffset, isUIInteraction ? "YES" : "NO");

        if (!isUIInteraction) {
            s_instance->m_camera->zoom((float)yoffset);
            Logger::trace("Camera zoom applied: {}", (float)yoffset);
        } else {
            Logger::trace("Camera zoom skipped due to UI interaction");
        }
    }
}

void InputHandler::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    // 添加更多键盘快捷键
    if (s_instance && action == GLFW_PRESS) {
        if (key == GLFW_KEY_R) {
            // 重置相机
            s_instance->m_camera->setView("Home");
        } else if (key == GLFW_KEY_F) {
            // Focus on object (for now, just center on origin)
            s_instance->m_camera->setCenterPoint(glm::vec3(0.0f, 0.0f, 0.0f));
        } else if (key == GLFW_KEY_H) {
            // Home view
            s_instance->m_camera->setView("Home");
        } else if (key == GLFW_KEY_T) {
            // Top view
            s_instance->m_camera->setView("Top");
        } else if (key == GLFW_KEY_B) {
            // Bottom view
            s_instance->m_camera->setView("Bottom");
        } else if (key == GLFW_KEY_L) {
            // Left view
            s_instance->m_camera->setView("Left");
        } else if (key == GLFW_KEY_R && mods == GLFW_MOD_CONTROL) {
            // Right view (using Ctrl+R to avoid conflict with reset)
            s_instance->m_camera->setView("Right");
        } else if (key == GLFW_KEY_P) {
            // Perspective view
            s_instance->m_camera->setView("Perspective");
        }
    }
}

void InputHandler::windowSizeCallback(GLFWwindow* window, int width, int height) {
    // 窗口大小变化处理
    // 这里可以添加窗口大小变化的逻辑
}
