#include "Camera.h"
#include "Logger.h"

Camera::Camera(int width, int height)
    : m_width(width), m_height(height),
      m_rotationX(0.0f), m_rotationY(0.0f), m_zoom(1.0f),
      m_position(0.0f, 0.0f, 5.0f),
      m_centerPoint(0.0f, 0.0f, 0.0f),  // 默认观察中心点为原点
      m_rotationSensitivity(0.01f),
    m_panSensitivity(0.5f),
    m_zoomSensitivity(1.0f) {

    recalculateMatrices();
}

void Camera::update() {
    recalculateMatrices();
}

void Camera::rotate(float deltaX, float deltaY) {
    // Calculate current distance from camera to center
    glm::vec3 relativePos = m_position - m_centerPoint;
    float distance = glm::length(relativePos);

    // Update rotation angles
    m_rotationY += deltaX * m_rotationSensitivity;
    m_rotationX -= deltaY * m_rotationSensitivity;

    // Limit X rotation to upper hemisphere only (avoid crossing XZ plane)
    // This prevents the gimbal lock / flip issue
    const float PI = 3.14159265f;
    if (m_rotationX > PI * 0.49f) m_rotationX = PI * 0.49f;  // ~88 degrees from top
    if (m_rotationX < 0.02f) m_rotationX = 0.02f;            // ~1 degree from top

    // Calculate camera position based on spherical coordinates
    float x = sin(m_rotationY) * sin(m_rotationX) * distance;
    float y = cos(m_rotationX) * distance;
    float z = cos(m_rotationY) * sin(m_rotationX) * distance;

    // Update camera position relative to center point
    m_position = m_centerPoint + glm::vec3(x, y, z);
}

void Camera::pan(float deltaX, float deltaY) {
    // Calculate pan speed based on current distance to center (zoom level)
    glm::vec3 relativePos = m_position - m_centerPoint;
    float distance = glm::length(relativePos);
    float adjustedSensitivity = m_panSensitivity * distance * 0.002f;

    // Pan moves the center point, camera follows
    // Note: deltaY is negated because screen Y is down, world Y is up
    m_centerPoint.x += deltaX * adjustedSensitivity;
    m_centerPoint.y -= deltaY * adjustedSensitivity;

    // Update camera position to maintain relative position
    m_position.x += deltaX * adjustedSensitivity;
    m_position.y -= deltaY * adjustedSensitivity;
}

void Camera::zoom(float delta) {
    // 计算相机到中心点的方向
    glm::vec3 relativePos = m_position - m_centerPoint;
    glm::vec3 direction = glm::normalize(relativePos);
    
    // 沿方向向量移动相机（注意方向：delta为正应该放大，即相机靠近中心点）
    float zoomAmount = delta * m_zoomSensitivity * 0.5f;
    m_position -= direction * zoomAmount;
    
    // 计算当前距离，无限制
    float distance = glm::length(m_position - m_centerPoint);
    
    // 更新zoom变量，用于正交投影矩阵
    m_zoom = 1.0f / distance;
}

void Camera::setCenterPoint(const glm::vec3& center) {
    // 当设置新的中心点时，保持相机相对于中心点的位置
    glm::vec3 relativePos = m_position - m_centerPoint;  // 保存当前相对位置
    m_centerPoint = center;
    m_position = m_centerPoint + relativePos;  // 更新相机位置
}

void Camera::moveCenterPoint(const glm::vec3& offset) {
    // 移动中心点，并相应地移动相机位置
    m_centerPoint += offset;
    m_position += offset;
}

void Camera::setView(const std::string& view) {
    // 重置旋转角度
    m_rotationX = 0.0f;
    m_rotationY = 0.0f;
    
    // 设置相机距离（缩放）
    float distance = 5.0f; // 默认距离
    
    // 根据视图类型设置相机位置
    if (view == "Front") {
        // 前视图：从Z正方向看
        m_position = m_centerPoint + glm::vec3(0.0f, 0.0f, distance);
        m_rotationX = 0.0f;
        m_rotationY = 0.0f;
    } else if (view == "Back") {
        // 后视图：从Z负方向看
        m_position = m_centerPoint + glm::vec3(0.0f, 0.0f, -distance);
        m_rotationX = 0.0f;
        m_rotationY = 3.14159f; // 180度
    } else if (view == "Left") {
        // 左视图：从X负方向看
        m_position = m_centerPoint + glm::vec3(-distance, 0.0f, 0.0f);
        m_rotationX = 0.0f;
        m_rotationY = -3.14159f / 2.0f; // -90度
    } else if (view == "Right") {
        // 右视图：从X正方向看
        m_position = m_centerPoint + glm::vec3(distance, 0.0f, 0.0f);
        m_rotationX = 0.0f;
        m_rotationY = 3.14159f / 2.0f; // 90度
    } else if (view == "Top") {
        // 顶视图：从Y正方向看
        m_position = m_centerPoint + glm::vec3(0.0f, distance, 0.0f);
        m_rotationX = -3.14159f / 2.0f; // -90度
        m_rotationY = 0.0f;
    } else if (view == "Bottom") {
        // 底视图：从Y负方向看
        m_position = m_centerPoint + glm::vec3(0.0f, -distance, 0.0f);
        m_rotationX = 3.14159f / 2.0f; // 90度
        m_rotationY = 0.0f;
    } else if (view == "Perspective" || view == "Home") {
        // 透视视图（默认视图）
        m_rotationX = 0.2f; // 小角度俯视
        m_rotationY = 0.5f; // 小角度侧视
        // 计算基于旋转角度的位置
        float x = sin(m_rotationY) * sin(m_rotationX) * distance;
        float y = cos(m_rotationX) * distance;
        float z = cos(m_rotationY) * sin(m_rotationX) * distance;
        m_position = m_centerPoint + glm::vec3(x, y, z);
    }
    
    // 更新缩放值
    m_zoom = 1.0f / distance;
}

void Camera::recalculateMatrices() {
    // Orthographic projection matrix
    float aspectRatio = static_cast<float>(m_width) / static_cast<float>(m_height);
    float size = 100.0f * m_zoom;
    m_projectionMatrix = glm::ortho(-size * aspectRatio, size * aspectRatio, -size, size, -1000.0f, 1000.0f);

    // Calculate up vector based on camera orientation
    // Use the rotation angles to compute a consistent up vector
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    // View matrix
    m_viewMatrix = glm::lookAt(m_position, m_centerPoint, up);

    // View-projection matrix
    m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
}
