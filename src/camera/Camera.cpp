#include "Camera.h"

Camera::Camera(int width, int height)
    : m_width(width), m_height(height),
      m_rotationX(0.0f), m_rotationY(0.0f), m_zoom(1.0f),
      m_position(0.0f, 0.0f, 5.0f),
      m_rotationSensitivity(0.01f),
      m_panSensitivity(0.5f),
      m_zoomSensitivity(0.1f) {

    recalculateMatrices();
}

void Camera::update() {
    recalculateMatrices();
}

void Camera::rotate(float deltaX, float deltaY) {
    // 计算当前相机到原点的距离
    float distance = glm::length(m_position);
    
    // 更新旋转角度
    m_rotationY += deltaX * m_rotationSensitivity;
    m_rotationX -= deltaY * m_rotationSensitivity;

    // 限制X轴旋转范围
    if (m_rotationX > 3.14f) m_rotationX = 3.14f;
    if (m_rotationX < -3.14f) m_rotationX = -3.14f;
    
    // 根据旋转角度和距离重新计算相机位置
    float x = sin(m_rotationY) * sin(m_rotationX) * distance;
    float y = cos(m_rotationX) * distance;
    float z = cos(m_rotationY) * sin(m_rotationX) * distance;
    
    m_position = glm::vec3(x, y, z);
}

void Camera::pan(float deltaX, float deltaY) {
    m_position.x += deltaX * m_panSensitivity;
    m_position.y += deltaY * m_panSensitivity;
}

void Camera::zoom(float delta) {
    // 计算相机到原点的方向
    glm::vec3 direction = glm::normalize(m_position);
    
    // 沿方向向量移动相机
    float zoomAmount = delta * m_zoomSensitivity * 0.5f;
    m_position += direction * zoomAmount;
    
    // 限制相机与原点的距离
    float distance = glm::length(m_position);
    if (distance < 1.0f) {
        m_position = direction * 1.0f;
    } else if (distance > 10.0f) {
        m_position = direction * 10.0f;
    }
}

void Camera::recalculateMatrices() {
    // 正交投影矩阵
    float aspectRatio = static_cast<float>(m_width) / static_cast<float>(m_height);
    float size = 100.0f * m_zoom;
    m_projectionMatrix = glm::ortho(-size * aspectRatio, size * aspectRatio, -size, size, -1000.0f, 1000.0f);

    // 视图矩阵：使用正确的坐标系设置，z轴向上
    // 1. 平移：将相机移动到观察位置
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), -m_position);
    
    // 2. 计算旋转矩阵，使相机始终看向原点
    glm::mat4 lookAt = glm::lookAt(m_position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    // 3. 使用lookAt矩阵作为视图矩阵
    m_viewMatrix = lookAt;

    // 计算视图投影矩阵
    m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
}
