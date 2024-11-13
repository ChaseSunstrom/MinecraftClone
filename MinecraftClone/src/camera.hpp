#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "types.hpp"
#include "event.hpp"
#include "frustum.hpp"

namespace MC {

    enum class CameraMovement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN
    };

    class Camera {
    public:
        Camera(f32 aspect_ratio = 16 / 9, glm::vec3 position = { 0.0f, 0.0f, 3.0f }, glm::vec3 up = { 0.0f, 1.0f, 0.0f },
            f32 yaw = -90.0f, f32 pitch = 0.0f, f32 fov = 45.0f, f32 far = 100.0f, f32 near = 0.1f, f32 movement_speed = 1, f32 mouse_sensitivity = 0.3);

        // Get view and projection matrices
        glm::mat4 GetViewMatrix() const;
        glm::mat4 GetProjectionMatrix() const;

        // Camera movement
        void ProcessKeyboard(CameraMovement direction, f32 delta_time);
        void ProcessMouseMovement(f32 xoffset, f32 yoffset, bool constrain_pitch = true);
        void ProcessMouseScroll(f32 yoffset);

        // Accessors
        f32 GetFOV() const { return m_fov; }
        f32 GetMouseSensitivity() const { return m_mouse_sensitivity; }
        glm::vec3 GetPosition() const { return m_position; }
        glm::vec3 GetFront() const { return m_front; }
        glm::vec3 GetUp() const { return m_up; }
        glm::vec3 GetRight() const { return m_right; }
        glm::vec3 GetWorldUp() const { return m_world_up; }

        Frustum& GetFrustum() { return m_frustum; }

        void SetAspectRatio(f32 aspect_ratio);
        void SetFOV(f32 fov);

        void SetFar(f32 far);
        void IncreaseFar(f32 amount);
        void DecreaseFar(f32 amount);

        void OnWindowResize(EventPtr<WindowResizedEvent> event);

    private:
        // Updates the front vector based on the current yaw and pitch
        void UpdateCameraVectors();
 
    private:
        // Camera attributes
        glm::vec3 m_position;
        glm::vec3 m_front;
        glm::vec3 m_up;
        glm::vec3 m_right;
        glm::vec3 m_world_up;

        // Euler angles
        f32 m_yaw;
        f32 m_pitch;

        // Camera options
        f32 m_movement_speed;
        f32 m_mouse_sensitivity;
        f32 m_fov;
        f32 m_aspect_ratio;
        f32 m_far;
        f32 m_near;

        Frustum m_frustum;
    };
}

#endif
