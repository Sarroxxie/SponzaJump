//
//

#include "Camera.h"
#include "glm/ext/matrix_transform.hpp"

Camera::Camera(glm::vec3 worldPos, glm::vec3 viewDir, glm::vec3 upVector)
    : m_WorldPos(worldPos), m_ViewDir(viewDir), m_UpVec(upVector)
{

}

glm::mat4 Camera::getCameraMatrix() {
    return glm::lookAt(m_WorldPos, m_WorldPos + glm::normalize(m_ViewDir), m_UpVec);
}

void Camera::setViewDir(glm::vec3 newViewDir) {
    m_ViewDir = newViewDir;
}

void Camera::setPosition(glm::vec3 newPosition) {
    m_WorldPos = newPosition;
}

void Camera::setLookAt(glm::vec3 newCenter) {
    m_ViewDir = glm::normalize(newCenter - m_WorldPos);
}
