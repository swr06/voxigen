#ifndef _voxigen_simpleFpsCamera_h_
#define _voxigen_simpleFpsCamera_h_

#include "voxigen/voxigen_export.h"
#include <memory>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace voxigen
{

class VOXIGEN_EXPORT SimpleFpsCamera
{
public:
    SimpleFpsCamera(glm::vec3 position=glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 direction=glm::vec3(0.0f, 1.0f, 0.0f));
    ~SimpleFpsCamera();

    void setFov(float fov);
    void setClipping(float near, float far);
    void setView(size_t width, size_t height);

    void setYaw(float angle);
    void setPitch(float angle);

    const glm::vec3 &getPosition() { return m_position; }
    void setPosition(const glm::vec3 &position) { m_position=position; }
    void move(const glm::vec3 &velocity);

    bool isDirty() { return (m_projectionDirty||m_viewDirty); }
    glm::mat4x4 &getProjectionViewMat() { updateMatrix(); return m_projectionViewMatrix; }

    glm::vec3 getDirection();
    void setDirection(const glm::vec3 &direction);

private:
    void updateMatrix();

    float m_fov;
    float m_near;
    float m_far;

    glm::vec3 m_position;
    float m_yaw;
    float m_pitch;

    glm::vec3 m_worldUp;

    size_t m_width;
    size_t m_height;

    bool m_projectionDirty;
    glm::mat4 m_projectionMatrix;
    bool m_viewDirty;
    glm::mat4 m_viewMatrix;

    glm::mat4 m_projectionViewMatrix;
};

}//namespace voxigen

#endif //_voxigen_simpleFpsCamera_h_