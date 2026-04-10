#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace gps {

    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp)
    {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;

        this->cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        this->cameraUpDirection = cameraUp;
        this->cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
    }

    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraPosition + cameraFrontDirection, cameraUpDirection);
    }

    void Camera::move(MOVE_DIRECTION direction, float speed)
    {
        if (direction == MOVE_FORWARD)
            cameraPosition += speed * cameraFrontDirection;

        if (direction == MOVE_BACKWARD)
            cameraPosition -= speed * cameraFrontDirection;

        if (direction == MOVE_LEFT)
            cameraPosition -= glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection)) * speed;

        if (direction == MOVE_RIGHT)
            cameraPosition += glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection)) * speed;

        cameraTarget = cameraPosition + cameraFrontDirection;
    }

    void Camera::rotate(float pitchOffset, float yawOffset)
    {
        yaw += yawOffset;
        pitch += pitchOffset;

        if (pitch > 89.0f)  pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        cameraFrontDirection = glm::normalize(front);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
        cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));

        cameraTarget = cameraPosition + cameraFrontDirection;
    }

}