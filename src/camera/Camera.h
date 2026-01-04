#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera(int width, int height);
    ~Camera() = default;

    void update();

    // 视角控制
    void rotate(float deltaX, float deltaY);
    void pan(float deltaX, float deltaY);
    void zoom(float delta);

    // 获取矩阵
    const glm::mat4& getViewMatrix() const { return m_viewMatrix; }
    const glm::mat4& getProjectionMatrix() const { return m_projectionMatrix; }
    const glm::mat4& getViewProjectionMatrix() const { return m_viewProjectionMatrix; }

    // 获取相机参数
    float getRotationX() const { return m_rotationX; }
    float getRotationY() const { return m_rotationY; }
    float getZoom() const { return m_zoom; }
    const glm::vec3& getPosition() const { return m_position; }

private:
    void recalculateMatrices();

    int m_width;
    int m_height;

    // 相机参数
    float m_rotationX;
    float m_rotationY;
    float m_zoom;
    glm::vec3 m_position;

    // 矩阵
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;
    glm::mat4 m_viewProjectionMatrix;

    // 灵敏度
    float m_rotationSensitivity;
    float m_panSensitivity;
    float m_zoomSensitivity;
};
