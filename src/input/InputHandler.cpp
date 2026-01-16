#include "InputHandler.h"
#include "Camera.h"
#include "UI.h"
#include "Logger.h"
#include "PluginContext.h"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <cstdio>

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
        // Check if interacting with UI elements
        bool isUIInteraction = s_instance->isUIInteraction();

        // Camera controls disabled for redesign
        // TODO: Implement new camera control system
        
        // Keep point selection active
        if (!isUIInteraction && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            s_instance->pickPoint(xpos, ypos);
        }
    }
}

void InputHandler::mouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
    if (s_instance) {
        glm::vec2 currentPos((float)xpos, (float)ypos);
        s_instance->m_lastMousePos = currentPos;
        
        // Camera controls disabled for redesign
        // TODO: Implement new camera control system
    }
}

void InputHandler::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (s_instance) {
        // Camera zoom disabled for redesign
        // TODO: Implement new camera zoom system
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
    // Window resize handling
}

void InputHandler::pickPoint(double mouseX, double mouseY) {
    if (!m_pluginContext || !m_pluginContext->hasPointCloudData()) {
        return;
    }

    // Get window size
    int windowWidth, windowHeight;
    glfwGetWindowSize(m_window, &windowWidth, &windowHeight);

    // Get camera matrices
    glm::mat4 viewMatrix = m_camera->getViewMatrix();
    glm::mat4 projMatrix = m_camera->getProjectionMatrix();
    glm::mat4 vpMatrix = projMatrix * viewMatrix;

    const auto& points = m_pluginContext->getPointCloudData();

    float minDist = (std::numeric_limits<float>::max)();
    int closestIndex = -1;

    // Find closest point to mouse position in screen space
    for (size_t i = 0; i < points.size(); i++) {
        const auto& pt = points[i];
        glm::vec4 worldPos(pt.x, pt.y, pt.z, 1.0f);

        // Transform to clip space
        glm::vec4 clipPos = vpMatrix * worldPos;

        // Skip points behind camera
        if (clipPos.w <= 0) continue;

        // Perspective divide to NDC
        glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;

        // Convert to screen coordinates
        // Vulkan NDC: Y points down, so we use (ndc.y + 1.0f) directly
        float screenX = (ndc.x + 1.0f) * 0.5f * windowWidth;
        float screenY = (ndc.y + 1.0f) * 0.5f * windowHeight;

        // Calculate distance to mouse
        float dx = screenX - (float)mouseX;
        float dy = screenY - (float)mouseY;
        float dist = dx * dx + dy * dy;

        if (dist < minDist) {
            minDist = dist;
            closestIndex = (int)i;
        }
    }

    // Check if click is close enough to a point (within 20 pixels)
    float threshold = 20.0f * 20.0f;  // squared distance
    if (closestIndex >= 0 && minDist < threshold) {
        const auto& pt = points[closestIndex];

        // Print coordinates to console
        Logger::info("[Point Selected] Index: {} | X: {:.2f}, Y: {:.2f}, Z: {:.2f} | Color: ({:.2f}, {:.2f}, {:.2f})",
                    closestIndex, pt.x, pt.y, pt.z, pt.r, pt.g, pt.b);

        // Also print to stdout for visibility
        printf("[Point] X: %.2f, Y: %.2f, Z: %.2f\n", pt.x, pt.y, pt.z);

        // Set selected point index
        m_pluginContext->setSelectedPointIndex(closestIndex);
    }
}
