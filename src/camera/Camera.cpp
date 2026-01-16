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
    // Create rotation quaternions for horizontal (around world Y) and vertical (around world X)
    float angleY = -deltaX * m_rotationSensitivity;
    float angleX = -deltaY * m_rotationSensitivity;

    // Rotation around world Y axis (horizontal rotation)
    glm::quat rotY = glm::angleAxis(angleY, glm::vec3(0.0f, 1.0f, 0.0f));

    // Rotation around world X axis (vertical rotation)
    // Using world X axis instead of local right vector to avoid gimbal lock-like behavior
    glm::vec3 worldRight = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::quat rotX = glm::angleAxis(angleX, worldRight);

    // Store old orientation for logging
    glm::quat oldOrientation = m_orientation;
    glm::vec3 oldPosition = m_position;

    // Apply rotations: horizontal rotation in world space, then vertical rotation
    // Order: rotY * m_orientation * rotX to rotate around world axes
    glm::quat newOrientation = rotY * m_orientation * rotX;
    newOrientation = glm::normalize(newOrientation);

    // Calculate new camera position
    glm::vec3 forward = newOrientation * glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 newPosition = m_centerPoint + forward * m_distance;

    // Check if the new position would cause the camera to be too close to vertical
    // Calculate the angle between forward direction and world Y axis
    glm::vec3 newForward = glm::normalize(m_centerPoint - newPosition);
    float verticalDot = glm::abs(glm::dot(newForward, glm::vec3(0.0f, 1.0f, 0.0f)));
    
    // Limit vertical rotation to prevent gimbal lock (max 89 degrees from horizontal)
    const float maxVerticalDot = 0.9998f;  // cos(1 degree) - very close to vertical but not quite
    
    if (verticalDot > maxVerticalDot) {
        // Reject this rotation - too close to vertical
        Logger::debug("Rotation rejected: too close to vertical (dot={:.4f})", verticalDot);
        return;
    }

    // Accept the rotation
    m_orientation = newOrientation;
    m_position = newPosition;

    // Calculate camera's up vector from orientation
    glm::vec3 localUp = m_orientation * glm::vec3(0.0f, 1.0f, 0.0f);
    
    // Log rotation details when there's significant movement
    if (glm::abs(deltaX) > 0.1f || glm::abs(deltaY) > 0.1f) {
        Logger::info("=== Camera Rotation ===");
        Logger::info("Delta: ({:.3f}, {:.3f})", deltaX, deltaY);
        Logger::info("Angles: Y={:.3f}, X={:.3f}", angleY, angleX);
        Logger::info("Position: ({:.3f}, {:.3f}, {:.3f})", m_position.x, m_position.y, m_position.z);
        Logger::info("Forward: ({:.3f}, {:.3f}, {:.3f})", forward.x, forward.y, forward.z);
        Logger::info("Local Up: ({:.3f}, {:.3f}, {:.3f})", localUp.x, localUp.y, localUp.z);
        Logger::info("Vertical Dot: {:.4f}", verticalDot);
        Logger::info("Orientation: ({:.3f}, {:.3f}, {:.3f}, {:.3f})", 
                    m_orientation.w, m_orientation.x, m_orientation.y, m_orientation.z);
        
        // Check if we're crossing the XZ plane (Y position changes sign)
        if ((oldPosition.y > 0 && m_position.y < 0) || (oldPosition.y < 0 && m_position.y > 0)) {
            Logger::warn("!!! CROSSING XZ PLANE !!! Y: {:.3f} -> {:.3f}", oldPosition.y, m_position.y);
        }
        
        // Check if local up vector is flipping
        if (localUp.y < 0) {
            Logger::warn("!!! LOCAL UP VECTOR IS INVERTED !!! Up.y = {:.3f}", localUp.y);
        }
    }
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

    // Use world Y axis as up vector to prevent flipping when crossing XZ plane
    // This ensures the camera's "up" direction always points towards world +Y
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    // Calculate local up from orientation for comparison
    glm::vec3 localUp = m_orientation * glm::vec3(0.0f, 1.0f, 0.0f);
    
    // Calculate forward direction
    glm::vec3 forward = glm::normalize(m_centerPoint - m_position);
    
    // Check if forward and up are nearly parallel (singularity)
    float forwardUpDot = glm::abs(glm::dot(forward, up));
    
    // Log when local up and world up are significantly different
    float upDot = glm::dot(localUp, up);
    if (upDot < 0.5f || forwardUpDot > 0.99f) {
        Logger::warn("recalculateMatrices: Potential singularity detected!");
        Logger::warn("  Position: ({:.3f}, {:.3f}, {:.3f})", m_position.x, m_position.y, m_position.z);
        Logger::warn("  Forward: ({:.3f}, {:.3f}, {:.3f})", forward.x, forward.y, forward.z);
        Logger::warn("  LocalUp dot WorldUp: {:.3f}", upDot);
        Logger::warn("  Forward dot Up: {:.3f}", forwardUpDot);
    }

    // View matrix
    glm::mat4 oldViewMatrix = m_viewMatrix;
    m_viewMatrix = glm::lookAt(m_position, m_centerPoint, up);
    
    // Extract and log the view matrix's Z axis (depth direction)
    glm::vec3 viewZ = glm::vec3(m_viewMatrix[0][2], m_viewMatrix[1][2], m_viewMatrix[2][2]);
    static glm::vec3 lastViewZ = viewZ;
    static bool firstRun = true;
    
    if (!firstRun) {
        // Check if view Z direction suddenly flipped
        float zDot = glm::dot(viewZ, lastViewZ);
        if (zDot < -0.5f) {  // Significant flip
            Logger::error("!!! VIEW MATRIX Z-AXIS FLIPPED !!! Dot={:.3f}", zDot);
            Logger::error("  Old ViewZ: ({:.3f}, {:.3f}, {:.3f})", lastViewZ.x, lastViewZ.y, lastViewZ.z);
            Logger::error("  New ViewZ: ({:.3f}, {:.3f}, {:.3f})", viewZ.x, viewZ.y, viewZ.z);
            Logger::error("  Camera Y: {:.3f}", m_position.y);
        }
    }
    firstRun = false;
    lastViewZ = viewZ;

    // View-projection matrix
    m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
}
