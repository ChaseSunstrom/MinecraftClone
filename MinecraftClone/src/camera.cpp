#include "camera.hpp"

namespace MC {
    Camera::Camera(f32 aspect_ratio, glm::vec3 position, glm::vec3 up, f32 yaw, f32 pitch, f32 fov, f32 movement_speed, f32 mouse_sensitivity)
        : m_aspect_ratio(aspect_ratio), m_position(position), m_world_up(up), m_yaw(yaw), m_pitch(pitch), m_fov(fov),
        m_movement_speed(movement_speed), m_mouse_sensitivity(mouse_sensitivity) {
        UpdateCameraVectors();
    }

    glm::mat4 Camera::GetViewMatrix() const {
        return glm::lookAt(m_position, m_position + m_front, m_up);
    }

    glm::mat4 Camera::GetProjectionMatrix() const {
        return glm::perspective(glm::radians(m_fov), m_aspect_ratio, 0.1f, 100.0f);
    }

    void Camera::OnWindowResize(EventPtr<WindowResizedEvent> event) {
        m_aspect_ratio = event->width / event->height;
    }

    void Camera::ProcessKeyboard(CameraMovement direction, f32 delta_time) {
        f32 velocity = m_movement_speed * delta_time;
        switch (direction) {
        case CameraMovement::FORWARD:
            m_position += m_front * velocity;
            break;
        case CameraMovement::BACKWARD:
            m_position -= m_front * velocity;
            break;
        case CameraMovement::LEFT:
            m_position -= m_right * velocity;
            break;
        case CameraMovement::RIGHT:
            m_position += m_right * velocity;
            break;
        case CameraMovement::UP:
            m_position += m_up * velocity;
            break;
        case CameraMovement::DOWN:
            m_position -= m_up * velocity;
            break;
        }
    }

    void Camera::ProcessMouseMovement(f32 xoffset, f32 yoffset, bool constrain_pitch) {
        xoffset *= m_mouse_sensitivity;
        yoffset *= m_mouse_sensitivity;

        m_yaw += xoffset;
        m_pitch += yoffset;

        if (constrain_pitch) {
            m_pitch = glm::clamp(m_pitch, -89.0f, 89.0f);
        }

        UpdateCameraVectors();
    }

    void Camera::SetAspectRatio(f32 aspect_ratio) {
        m_aspect_ratio = aspect_ratio;
    }

    void Camera::SetFOV(f32 fov) {
        m_fov = glm::clamp(fov, 1.0f, 90.0f);
    }

    void Camera::ProcessMouseScroll(f32 yoffset) {
        m_fov -= yoffset;
        m_fov = glm::clamp(m_fov, 1.0f, 45.0f);
    }

    void Camera::UpdateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        front.y = sin(glm::radians(m_pitch));
        front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        m_front = glm::normalize(front);

        m_right = glm::normalize(glm::cross(m_front, m_world_up));
        m_up = glm::normalize(glm::cross(m_right, m_front));
    }
}