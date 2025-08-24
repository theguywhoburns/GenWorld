#include <GenWorld/Core/Camera.h>

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : front(glm::vec3(0.0f, 0.0f, -1.0f)), movementSpeed(SPEED),
      mouseSensitivity(SENSITIVITY), zoom(ZOOMMAX) {
  this->position = position;
  this->worldUp = up;
  this->yaw = yaw;
  this->pitch = pitch;
  updateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ, float upX, float upY,
               float upZ, float yaw, float pitch)
    : front(glm::vec3(0.0f, 0.0f, -1.0f)), movementSpeed(SPEED),
      mouseSensitivity(SENSITIVITY), zoom(ZOOMMAX) {
  this->position = glm::vec3(posX, posY, posZ);
  this->worldUp = glm::vec3(upX, upY, upZ);
  this->yaw = yaw;
  this->pitch = pitch;
  updateCameraVectors();
}

void Camera::processKeyboard(Camera_Movement direction, float deltaTime) {
  float velocity = movementSpeed * deltaTime;
  if (direction == FORWARD) {
    position += front * velocity;
  }
  if (direction == BACKWARD) {
    position -= front * velocity;
  }
  if (direction == LEFT) {
    position -= right * velocity;
  }
  if (direction == RIGHT) {
    position += right * velocity;
  }
}

void Camera::processMouseMovement(float xoffset, float yoffset,
                                  GLboolean constrainPitch) {
  xoffset *= mouseSensitivity;
  yoffset *= mouseSensitivity;

  yaw += xoffset;
  pitch += yoffset;

  if (constrainPitch) {
    if (pitch > 89.0f) {
      pitch = 89.0f;
    }
    if (pitch < -89.0f) {
      pitch = -89.0f;
    }
  }

  updateCameraVectors();
}

void Camera::processMouseScroll(float yoffset) {
  if (zoom >= ZOOMMIN && zoom <= ZOOMMAX) {
    zoom -= yoffset;
  }
  if (zoom <= ZOOMMIN) {
    zoom = ZOOMMIN;
  }
  if (zoom >= ZOOMMAX) {
    zoom = ZOOMMAX;
  }
}

glm::mat4 Camera::GetViewMatrix() {
  return glm::lookAt(position, position + front, up);
}

void Camera::updateCameraVectors() {
  // calculate the new Front vector
  glm::vec3 front;
  front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
  front.y = sin(glm::radians(pitch));
  front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
  this->front = glm::normalize(front);
  // also re-calculate the Right and Up vector
  right = glm::normalize(glm::cross(
      front, worldUp)); // normalize the vectors, because their length gets
                        // closer to 0 the more you look up or down which
                        // results in slower movement.
  up = glm::normalize(glm::cross(right, front));
}
