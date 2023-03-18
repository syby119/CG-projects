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
    PerspectiveCamera& _camera;
    bool _enabled = true;

    int _screenLeft = 0;
    int _screenTop = 0;
    int _screenWidth = 0;
    int _screenHeight = 0;

    float _rotateSpeed = 1.0f;
    float _zoomSpeed = 1.2f;
    float _panSpeed = 0.3f;

    bool _staticMoving = false;
    float _dynamicDampingFactor = 0.2f;

    float _minDistance = 0.0f;
    float _maxDistance = std::numeric_limits<float>::infinity();

    glm::vec3 _target = glm::vec3(0.0f);
    glm::vec3 lastPosition = glm::vec3(0.0f);
    float _lastZoom = 1.0f;
    float _lastAngle = 0.0f;

    STATE _state = STATE::NONE;

    glm::vec3 _eye = glm::vec3(0.0f);
    glm::vec2 _movePrev = glm::vec2(0.0f);
    glm::vec2 _moveCurr = glm::vec2(0.0f);
    glm::vec3 _lastAxis = glm::vec3(0.0f);
    glm::vec2 _zoomStart = glm::vec2(1.0f);
    glm::vec2 _zoomEnd = glm::vec2(1.0f);
    glm::vec2 _panStart = glm::vec2(0.0f);
    glm::vec2 _panEnd = glm::vec2(0.0f);

    static constexpr float EPS = 0.001f;

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