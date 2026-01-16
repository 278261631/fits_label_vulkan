#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>

class Camera {
public:
    Camera(int width, int height);
    ~Camera() = default;

    void update();

    // View control
    void rotate(float deltaX, float deltaY);
    void pan(float deltaX, float deltaY);
    void zoom(float delta);
    void setCenterPoint(const glm::vec3& center);
    void moveCenterPoint(const glm::vec3& offset);
    void setView(const std::string& view);

    // Get matrices
    const glm::mat4& getViewMatrix() const { return m_viewMatrix; }
    const glm::mat4& getProjectionMatrix() const { return m_projectionMatrix; }
    const glm::mat4& getViewProjectionMatrix() const { return m_viewProjectionMatrix; }

    // Get camera parameters
    float getRotationX() const { return m_rotationX; }
    float getRotationY() const { return m_rotationY; }
    float getZoom() const { return m_zoom; }
    const glm::vec3& getPosition() const { return m_position; }
    const glm::vec3& getCenterPoint() const { return m_centerPoint; }

private:
    void recalculateMatrices();

    int m_width;
    int m_height;

    // Camera parameters
    float m_rotationX;
    float m_rotationY;
    float m_zoom;
    float m_distance;  // Distance from center
    glm::vec3 m_position;
    glm::vec3 m_centerPoint;
    glm::quat m_orientation;  // Quaternion for rotation

    // Matrices
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;
    glm::mat4 m_viewProjectionMatrix;

    // Sensitivity
    float m_rotationSensitivity;
    float m_panSensitivity;
    float m_zoomSensitivity;
};
