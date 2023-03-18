#include "camera_controller.h"
#include <iostream>

CameraController::CameraController(
    PerspectiveCamera& camera, const glm::vec3& target, int screenWidth, int screenHeight)
    : _camera(camera), _screenWidth(screenWidth), _screenHeight(screenHeight), _target(target) {}

void CameraController::update(const Input& input, float deltaTime) {
    auto& mouse = input.mouse;
    auto& pageX = input.mouse.move.xNow;
    auto& pageY = input.mouse.move.yNow;
    auto& deltaY = input.mouse.scroll.yOffset;
    bool mouseDown = (mouse.press.left || mouse.press.middle || mouse.press.right);
    bool mouseMove =
        (mouse.move.xNow - mouse.move.xOld != 0) || (mouse.move.yNow - mouse.move.yOld != 0);

    if (!mouseDown) {
        _state = STATE::NONE;
        return;
    } else {
        if (_state == STATE::NONE) {
            if (mouse.press.left) {
                _state = STATE::ROTATE;
            } else if (mouse.press.middle) {
                _state = STATE::ZOOM;
            } else if (mouse.press.right) {
                _state = STATE::PAN;
            }

            switch (_state) {
            case CameraController::STATE::ROTATE:
                _moveCurr = getMouseOnCircle(pageX, pageY);
                _movePrev = _moveCurr;
                break;
            case CameraController::STATE::ZOOM:
                _zoomStart = getMouseOnScreen(pageX, pageY);
                _zoomEnd = _zoomStart;
                break;
            case CameraController::STATE::PAN:
                _panStart = getMouseOnScreen(pageX, pageY);
                _panEnd = _panStart;
                break;
            default: break;
            }
        }
    }

    if (mouseMove) {
        switch (_state) {
        case CameraController::STATE::ROTATE:
            _movePrev = _moveCurr;
            _moveCurr = getMouseOnCircle(pageX, pageY);
            break;
        case CameraController::STATE::ZOOM: _zoomEnd = getMouseOnScreen(pageX, pageY); break;
        case CameraController::STATE::PAN: _panEnd = getMouseOnScreen(pageX, pageY); break;
        default: break;
        }
    }

    _eye = _camera.transform.position - _target;
    if (mouse.press.left) {
        rotateCamera1();
    }

    if (mouse.press.middle) {
        _zoomStart.y += input.mouse.scroll.yOffset * 0.025f;
        zoomCamera();
    }

    if (mouse.press.right) {
        panCamera();
    }

    _camera.transform.position = _target + _eye;
}

glm::vec2 CameraController::getMouseOnScreen(float pageX, float pageY) {
    return glm::vec2((pageX - _screenLeft) / _screenWidth, (pageY - _screenTop) / _screenHeight);
}

glm::vec2 CameraController::getMouseOnCircle(float pageX, float pageY) {
    return glm::vec2(
        (2.0f * (pageX - _screenLeft) - _screenWidth) / _screenWidth,
        (_screenHeight + 2.0f * (_screenTop - pageY)) / _screenWidth);
}

void CameraController::rotateCamera() {
    glm::vec3 moveDirection = glm::vec3(_moveCurr.x - _movePrev.x, _moveCurr.y - _movePrev.y, 0);
    float angle = glm::length(moveDirection);

    if (angle > 0) {
        _eye = _camera.transform.position - _target;
        glm::vec3 eyeDirection = glm::normalize(_eye);
        glm::vec3 objectUpDirection = glm::normalize(_camera.transform.getUp());
        glm::vec3 objectSidewaysDirection =
            glm::normalize(glm::cross(objectUpDirection, eyeDirection));

        objectUpDirection *= _moveCurr.y - _movePrev.y;
        objectSidewaysDirection *= _moveCurr.x - _movePrev.x;

        moveDirection = objectUpDirection + objectSidewaysDirection;
        glm::vec3 axis = glm::normalize(glm::cross(moveDirection, eyeDirection));
        angle *= _rotateSpeed;
        glm::quat quaternion = createQuatFromAngleAxis(angle, axis);
        _camera.transform.rotation = quaternion * _camera.transform.rotation;

        _eye = quaternion * _eye;
        _lastAxis = axis;
        _lastAngle = angle;
    } else if (!_staticMoving && (_lastAngle > 0)) {
        _lastAngle *= std::sqrt(1.0f - _dynamicDampingFactor);
        _eye = _camera.transform.position - _target;
        glm::quat quaternion = createQuatFromAngleAxis(_lastAngle, _lastAxis);
        _camera.transform.rotation = quaternion * _camera.transform.rotation;
        _eye = quaternion * _eye;
    }
    _movePrev = _moveCurr;
}

void CameraController::rotateCamera1() {
    constexpr float minPolar = 1e-2f;
    constexpr float maxPolar = glm::pi<float>() - 1e-2f;

    glm::quat q = glm::quat(_camera.transform.getUp(), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::quat invQ = glm::inverse(q);

    _eye = _camera.transform.position - _target;
    _eye = q * _eye;

    float radius = glm::length(_eye);
    glm::vec3 normEye = glm::normalize(_eye);
    float phi = std::acos(normEye.y);
    float theta = std::atan2(normEye.x, normEye.z);
    if (theta < 0) {
        theta += 2 * glm::pi<float>();
    }
    theta += (_moveCurr.x - _movePrev.x) * _rotateSpeed;
    phi += (_moveCurr.y - _movePrev.y) * _rotateSpeed;
    phi = std::min(maxPolar, std::max(minPolar, phi));
    _eye = radius
           * glm::vec3(
               std::sin(theta) * std::sin(phi), std::cos(phi), std::cos(theta) * std::sin(phi));
    _eye = invQ * _eye;

    // correct lookAt
    glm::vec3 in = glm::normalize(_eye);
    glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), in));
    glm::vec3 newUp = glm::normalize(glm::cross(in, right));
    _camera.transform.rotation = glm::mat3(right, newUp, in);
    _movePrev = _moveCurr;
}

void CameraController::zoomCamera() {
    float factor = 1.0f + (_zoomEnd.y - _zoomStart.y) * _zoomSpeed;
    if (factor != 1.0f && factor > 0.0f) {
        // Perspective Camera
        _eye *= factor;
    }

    if (_staticMoving) {
        _zoomStart = _zoomEnd;
    } else {
        _zoomStart.y += (_zoomEnd.y - _zoomStart.y) * _dynamicDampingFactor;
    }
}

void CameraController::panCamera() {
    glm::vec2 mouseChange = _panEnd - _panStart;
    if (glm::length(mouseChange) > 0) {
        mouseChange *= glm::length(_eye) * _panSpeed;
        glm::vec3 cameraUp = glm::normalize(_camera.transform.getUp());
        glm::vec3 pan =
            mouseChange.x * glm::normalize(glm::cross(_eye, cameraUp)) + mouseChange.y * cameraUp;

        _camera.transform.position += pan;
        _target += pan;
        if (_staticMoving) {
            _panStart = _panEnd;
        } else {
            _panStart += _dynamicDampingFactor * (_panEnd - _panStart);
        }
    }
}

void CameraController::checkDistance() {
    float eyeLength = glm::length(_eye);
    if (eyeLength > _maxDistance) {
        _camera.transform.position = _target + _maxDistance * _eye;
    }

    if (eyeLength < _minDistance) {
        _camera.transform.position = _target + _minDistance * _eye;
    }

    _zoomStart = _zoomEnd;
}