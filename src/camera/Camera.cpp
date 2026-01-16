#include "Camera.h"
#include "Logger.h"
#include <glm/gtx/quaternion.hpp>

Camera::Camera(int width, int height)
    : m_width(width), m_height(height),
      m_rotationX(0.0f), m_rotationY(0.0f), m_zoom(0.2f),
      m_distance(5.0f),
      m_position(0.0f, 0.0f, 5.0f),
      m_centerPoint(0.0f, 0.0f, 0.0f),
      m_orientation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
      m_rotationSensitivity(0.005f),
      m_panSensitivity(0.5f),
      m_zoomSensitivity(1.0f) {

    recalculateMatrices();
}

void Camera::update() {
    recalculateMatrices();
}

void Camera::rotate(float deltaX, float deltaY) {
    // Create rotation quaternions for horizontal (around world Y) and vertical (around local X)
    float angleY = -deltaX * m_rotationSensitivity;
    float angleX = -deltaY * m_rotationSensitivity;

    // Rotation around world Y axis (horizontal rotation)
    glm::quat rotY = glm::angleAxis(angleY, glm::vec3(0.0f, 1.0f, 0.0f));

    // Rotation around local X axis (vertical rotation)
    // Get the local right vector from current orientation
    glm::vec3 right = m_orientation * glm::vec3(1.0f, 0.0f, 0.0f);
    glm::quat rotX = glm::angleAxis(angleX, right);

    // Apply rotations: first horizontal, then vertical
    m_orientation = rotX * rotY * m_orientation;
    m_orientation = glm::normalize(m_orientation);

    // Update camera position based on orientation and distance
    glm::vec3 forward = m_orientation * glm::vec3(0.0f, 0.0f, 1.0f);
    m_position = m_centerPoint + forward * m_distance;
}

void Camera::pan(float deltaX, float deltaY) {
    float adjustedSensitivity = m_panSensitivity * m_distance * 0.002f;

    // Get camera's right and up vectors from orientation
    glm::vec3 right = m_orientation * glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 up = m_orientation * glm::vec3(0.0f, 1.0f, 0.0f);

    // Pan in camera's local space
    glm::vec3 offset = right * (deltaX * adjustedSensitivity) - up * (deltaY * adjustedSensitivity);

    m_centerPoint += offset;
    m_position += offset;
}

void Camera::zoom(float delta) {
    float zoomAmount = delta * m_zoomSensitivity * 0.5f;
    m_distance -= zoomAmount;

    // Clamp distance
    if (m_distance < 0.1f) m_distance = 0.1f;
    if (m_distance > 10000.0f) m_distance = 10000.0f;

    // Update camera position
    glm::vec3 forward = m_orientation * glm::vec3(0.0f, 0.0f, 1.0f);
    m_position = m_centerPoint + forward * m_distance;

    // Update zoom for orthographic projection
    m_zoom = 1.0f / m_distance;
}

void Camera::setCenterPoint(const glm::vec3& center) {
    glm::vec3 offset = center - m_centerPoint;
    m_centerPoint = center;
    m_position += offset;
}

void Camera::moveCenterPoint(const glm::vec3& offset) {
    m_centerPoint += offset;
    m_position += offset;
}

void Camera::setView(const std::string& view) {
    m_distance = 5.0f;

    if (view == "Front") {
        m_orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    } else if (view == "Back") {
        m_orientation = glm::angleAxis(glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
    } else if (view == "Left") {
        m_orientation = glm::angleAxis(glm::half_pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
    } else if (view == "Right") {
        m_orientation = glm::angleAxis(-glm::half_pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
    } else if (view == "Top") {
        m_orientation = glm::angleAxis(-glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
    } else if (view == "Bottom") {
        m_orientation = glm::angleAxis(glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
    } else if (view == "Perspective" || view == "Home") {
        // Default perspective view
        glm::quat rotY = glm::angleAxis(0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::quat rotX = glm::angleAxis(-0.3f, glm::vec3(1.0f, 0.0f, 0.0f));
        m_orientation = rotX * rotY;
    }

    m_orientation = glm::normalize(m_orientation);

    // Update position based on orientation
    glm::vec3 forward = m_orientation * glm::vec3(0.0f, 0.0f, 1.0f);
    m_position = m_centerPoint + forward * m_distance;

    m_zoom = 1.0f / m_distance;
}

void Camera::recalculateMatrices() {
    // Orthographic projection matrix
    float aspectRatio = static_cast<float>(m_width) / static_cast<float>(m_height);
    float size = 100.0f * m_zoom;
    m_projectionMatrix = glm::ortho(-size * aspectRatio, size * aspectRatio, -size, size, -1000.0f, 1000.0f);

    // Calculate up vector from quaternion orientation
    glm::vec3 up = m_orientation * glm::vec3(0.0f, 1.0f, 0.0f);

    // View matrix
    m_viewMatrix = glm::lookAt(m_position, m_centerPoint, up);

    // View-projection matrix
    m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
}
