#ifndef GRAPHICSPRAKTIKUM_CAMERA_H
#define GRAPHICSPRAKTIKUM_CAMERA_H


#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"

class Camera {

private:

    glm::vec3 m_WorldPos;
    glm::vec3 m_ViewDir;
    glm::vec3 m_UpVec;

public:
    explicit Camera(glm::vec3 worldPos = glm::vec3(0, 0, 5), glm::vec3 viewDir = glm::vec3(0, 0, -1), glm::vec3 upVector = glm::vec3(0, 1, 0));

    void setViewDir(glm::vec3 newViewDir);
    void setPosition(glm::vec3 newPosition);
    void setLookAt(glm::vec3 newCenter);

    glm::vec3 getWorldPos();
    glm::vec3 getViewDir();

    glm::vec3& getWorldPosRef();
    glm::vec3& getViewDirRef();

    glm::mat4 getCameraMatrix();

};


#endif //GRAPHICSPRAKTIKUM_CAMERA_H
