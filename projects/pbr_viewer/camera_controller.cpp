#include "camera_controller.h"
#include <iostream>

CameraController::CameraController(
    PerspectiveCamera& camera, const glm::vec3& target, int screenWidth, int screenHeight)
    : m_camera(camera), m_screenWidth(screenWidth), m_screenHeight(screenHeight), m_target(target) {
}

void CameraController::update(const Input& input, float deltaTime) {
    auto& mouse = input.mouse;
    auto& pageX = input.mouse.move.xNow;
    auto& pageY = input.mouse.move.yNow;
    auto& deltaY = input.mouse.scroll.yOffset;
    bool mouseDown = (mouse.press.left || mouse.press.middle || mouse.press.right);
    bool mouseMove =
        (mouse.move.xNow - mouse.move.xOld != 0) || (mouse.move.yNow - mouse.move.yOld != 0);

    if (!mouseDown) {
        m_state = STATE::NONE;
        return;
    } else {
        if (m_state == STATE::NONE) {
            if (mouse.press.left) {
                m_state = STATE::ROTATE;
            } else if (mouse.press.middle) {
                m_state = STATE::ZOOM;
            } else if (mouse.press.right) {
                m_state = STATE::PAN;
            }

            switch (m_state) {
            case CameraController::STATE::ROTATE:
                m_moveCurr = getMouseOnCircle(pageX, pageY);
                m_movePrev = m_moveCurr;
                break;
            case CameraController::STATE::ZOOM:
                m_zoomStart = getMouseOnScreen(pageX, pageY);
                m_zoomEnd = m_zoomStart;
                break;
            case CameraController::STATE::PAN:
                m_panStart = getMouseOnScreen(pageX, pageY);
                m_panEnd = m_panStart;
                break;
            default: break;
            }
        }
    }

    if (mouseMove) {
        switch (m_state) {
        case CameraController::STATE::ROTATE:
            m_movePrev = m_moveCurr;
            m_moveCurr = getMouseOnCircle(pageX, pageY);
            break;
        case CameraController::STATE::ZOOM: m_zoomEnd = getMouseOnScreen(pageX, pageY); break;
        case CameraController::STATE::PAN: m_panEnd = getMouseOnScreen(pageX, pageY); break;
        default: break;
        }
    }

    m_eye = m_camera.transform.position - m_target;
    if (mouse.press.left) {
        rotateCamera1();
    }

    if (mouse.press.middle) {
        m_zoomStart.y += input.mouse.scroll.yOffset * 0.025f;
        zoomCamera();
    }

    if (mouse.press.right) {
        panCamera();
    }

    m_camera.transform.position = m_target + m_eye;
}

glm::vec2 CameraController::getMouseOnScreen(float pageX, float pageY) {
    return glm::vec2(
        (pageX - m_screenLeft) / m_screenWidth, (pageY - m_screenTop) / m_screenHeight);
}

glm::vec2 CameraController::getMouseOnCircle(float pageX, float pageY) {
    return glm::vec2(
        (2.0f * (pageX - m_screenLeft) - m_screenWidth) / m_screenWidth,
        (m_screenHeight + 2.0f * (m_screenTop - pageY)) / m_screenWidth);
}

void CameraController::rotateCamera() {
    glm::vec3 moveDirection =
        glm::vec3(m_moveCurr.x - m_movePrev.x, m_moveCurr.y - m_movePrev.y, 0);
    float angle = glm::length(moveDirection);

    if (angle > 0) {
        m_eye = m_camera.transform.position - m_target;
        glm::vec3 eyeDirection = glm::normalize(m_eye);
        glm::vec3 objectUpDirection = glm::normalize(m_camera.transform.getUp());
        glm::vec3 objectSidewaysDirection =
            glm::normalize(glm::cross(objectUpDirection, eyeDirection));

        objectUpDirection *= m_moveCurr.y - m_movePrev.y;
        objectSidewaysDirection *= m_moveCurr.x - m_movePrev.x;

        moveDirection = objectUpDirection + objectSidewaysDirection;
        glm::vec3 axis = glm::normalize(glm::cross(moveDirection, eyeDirection));
        angle *= m_rotateSpeed;
        glm::quat quaternion = createQuatFromAngleAxis(angle, axis);
        m_camera.transform.rotation = quaternion * m_camera.transform.rotation;

        m_eye = quaternion * m_eye;
        m_lastAxis = axis;
        m_lastAngle = angle;
    } else if (!m_staticMoving && (m_lastAngle > 0)) {
        m_lastAngle *= std::sqrt(1.0f - m_dynamicDampingFactor);
        m_eye = m_camera.transform.position - m_target;
        glm::quat quaternion = createQuatFromAngleAxis(m_lastAngle, m_lastAxis);
        m_camera.transform.rotation = quaternion * m_camera.transform.rotation;
        m_eye = quaternion * m_eye;
    }
    m_movePrev = m_moveCurr;
}

void CameraController::rotateCamera1() {
    constexpr float minPolar = 1e-2f;
    constexpr float maxPolar = glm::pi<float>() - 1e-2f;

    glm::quat q = glm::quat(m_camera.transform.getUp(), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::quat invQ = glm::inverse(q);

    m_eye = m_camera.transform.position - m_target;
    m_eye = q * m_eye;

    float radius = glm::length(m_eye);
    glm::vec3 normEye = glm::normalize(m_eye);
    float phi = std::acos(normEye.y);
    float theta = std::atan2(normEye.x, normEye.z);
    if (theta < 0) {
        theta += 2 * glm::pi<float>();
    }
    theta += (m_moveCurr.x - m_movePrev.x) * m_rotateSpeed;
    phi += (m_moveCurr.y - m_movePrev.y) * m_rotateSpeed;
    phi = std::min(maxPolar, std::max(minPolar, phi));
    m_eye = radius
            * glm::vec3(
                std::sin(theta) * std::sin(phi), std::cos(phi), std::cos(theta) * std::sin(phi));
    m_eye = invQ * m_eye;

    // correct lookAt
    glm::vec3 in = glm::normalize(m_eye);
    glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), in));
    glm::vec3 newUp = glm::normalize(glm::cross(in, right));
    m_camera.transform.rotation = glm::mat3(right, newUp, in);
    m_movePrev = m_moveCurr;
}

void CameraController::zoomCamera() {
    float factor = 1.0f + (m_zoomEnd.y - m_zoomStart.y) * m_zoomSpeed;
    if (factor != 1.0f && factor > 0.0f) {
        // Perspective Camera
        m_eye *= factor;
    }

    if (m_staticMoving) {
        m_zoomStart = m_zoomEnd;
    } else {
        m_zoomStart.y += (m_zoomEnd.y - m_zoomStart.y) * m_dynamicDampingFactor;
    }
}

void CameraController::panCamera() {
    glm::vec2 mouseChange = m_panEnd - m_panStart;
    if (glm::length(mouseChange) > 0) {
        mouseChange *= glm::length(m_eye) * m_panSpeed;
        glm::vec3 cameraUp = glm::normalize(m_camera.transform.getUp());
        glm::vec3 pan =
            mouseChange.x * glm::normalize(glm::cross(m_eye, cameraUp)) + mouseChange.y * cameraUp;

        m_camera.transform.position += pan;
        m_target += pan;
        if (m_staticMoving) {
            m_panStart = m_panEnd;
        } else {
            m_panStart += m_dynamicDampingFactor * (m_panEnd - m_panStart);
        }
    }
}

void CameraController::checkDistance() {
    float eyeLength = glm::length(m_eye);
    if (eyeLength > m_maxDistance) {
        m_camera.transform.position = m_target + m_maxDistance * m_eye;
    }

    if (eyeLength < m_minDistance) {
        m_camera.transform.position = m_target + m_minDistance * m_eye;
    }

    m_zoomStart = m_zoomEnd;
}