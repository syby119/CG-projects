#pragma once

#include <algorithm>
#include <glm/glm.hpp>

#include "../base/camera.h"
#include "../base/input.h"

class CameraController {
private:
    enum class STATE {
        NONE = -1,
        ROTATE = 0,
        ZOOM = 1,
        PAN = 2,
    };

public:
    CameraController(
        PerspectiveCamera& camera, const glm::vec3& target, int screenWidth, int screenHeight);

    void update(const Input& input, float deltaTime);

    ~CameraController() = default;

private:
    PerspectiveCamera& m_camera;
    bool m_enabled = true;

    int m_screenLeft = 0;
    int m_screenTop = 0;
    int m_screenWidth = 0;
    int m_screenHeight = 0;

    float m_rotateSpeed = 1.0f;
    float m_zoomSpeed = 1.2f;
    float m_panSpeed = 0.3f;

    bool m_staticMoving = false;
    float m_dynamicDampingFactor = 0.2f;

    float m_minDistance = 0.0f;
    float m_maxDistance = std::numeric_limits<float>::infinity();

    glm::vec3 m_target = glm::vec3(0.0f);
    glm::vec3 m_lastPosition = glm::vec3(0.0f);
    float m_lastZoom = 1.0f;
    float m_lastAngle = 0.0f;

    STATE m_state = STATE::NONE;

    glm::vec3 m_eye = glm::vec3(0.0f);
    glm::vec2 m_movePrev = glm::vec2(0.0f);
    glm::vec2 m_moveCurr = glm::vec2(0.0f);
    glm::vec3 m_lastAxis = glm::vec3(0.0f);
    glm::vec2 m_zoomStart = glm::vec2(1.0f);
    glm::vec2 m_zoomEnd = glm::vec2(1.0f);
    glm::vec2 m_panStart = glm::vec2(0.0f);
    glm::vec2 m_panEnd = glm::vec2(0.0f);

    static constexpr float m_ePS = 0.001f;

private:
    glm::vec2 getMouseOnScreen(float pageX, float pageY);
    glm::vec2 getMouseOnCircle(float pageX, float pageY);
    void rotateCamera();
    void rotateCamera1();
    void zoomCamera();
    void panCamera();
    void checkDistance();
    // void reset();
    glm::quat createQuatFromAngleAxis(float angle, const glm::vec3& axis) {
        float sinHalfAngle = std::sin(angle * 0.5f);
        return glm::quat(
            std::cos(angle * 0.5f), sinHalfAngle * axis.x, sinHalfAngle * axis.y,
            sinHalfAngle * axis.z);
    }
};