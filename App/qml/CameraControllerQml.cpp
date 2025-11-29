#include "CameraControllerQml.h"

CameraControllerQml::CameraControllerQml(QObject *parent)
    : QObject(parent) {
}

void CameraControllerQml::setSharedController(CameraController *controller) {
    m_sharedController = controller;
    if (m_sharedController) {
        m_lastPosition = m_sharedController->camera().position();
        m_lastYaw = m_sharedController->camera().yaw();
        m_lastPitch = m_sharedController->camera().pitch();
        m_lastSpeed = m_sharedController->speed();
        m_lastOrbitDistance = m_sharedController->orbitDistance();
    }
}

QVector3D CameraControllerQml::position() const {
    return m_sharedController ? m_sharedController->camera().position() : QVector3D();
}

float CameraControllerQml::yaw() const {
    return m_sharedController ? m_sharedController->camera().yaw() : -90.0f;
}

float CameraControllerQml::pitch() const {
    return m_sharedController ? m_sharedController->camera().pitch() : 0.0f;
}

float CameraControllerQml::speed() const {
    return m_sharedController ? m_sharedController->speed() : 5.0f;
}

float CameraControllerQml::mouseSensitivity() const {
    return m_sharedController ? m_sharedController->mouseSensitivity() : 0.1f;
}

float CameraControllerQml::orbitDistance() const {
    return m_sharedController ? m_sharedController->orbitDistance() : 5.0f;
}

float CameraControllerQml::speedMin() const {
    return m_sharedController ? m_sharedController->cameraSpeedMin() : 0.1f;
}

float CameraControllerQml::speedMax() const {
    return m_sharedController ? m_sharedController->cameraSpeedMax() : 100.0f;
}

float CameraControllerQml::orbitDistanceMin() const {
    return m_sharedController ? m_sharedController->orbitDistanceMin() : 1.0f;
}

float CameraControllerQml::orbitDistanceMax() const {
    return m_sharedController ? m_sharedController->orbitDistanceMax() : 100.0f;
}

bool CameraControllerQml::isMovingCamera() const {
    return m_sharedController ? m_sharedController->isMovingCamera() : false;
}

void CameraControllerQml::setSpeed(float s) {
    if (!m_sharedController) return;
    m_sharedController->setSpeed(s);
    if (!qFuzzyCompare(m_lastSpeed, m_sharedController->speed())) {
        m_lastSpeed = m_sharedController->speed();
        emit speedChanged();
    }
}

void CameraControllerQml::setMouseSensitivity(float s) {
    if (!m_sharedController) return;
    float old = m_sharedController->mouseSensitivity();
    m_sharedController->setMouseSensitivity(s);
    if (!qFuzzyCompare(old, m_sharedController->mouseSensitivity())) {
        emit mouseSensitivityChanged();
    }
}

void CameraControllerQml::setOrbitDistance(float d) {
    if (!m_sharedController) return;
    m_sharedController->setOrbitDistance(d);
    if (!qFuzzyCompare(m_lastOrbitDistance, m_sharedController->orbitDistance())) {
        m_lastOrbitDistance = m_sharedController->orbitDistance();
        emit orbitDistanceChanged();
    }
}

void CameraControllerQml::handleKeyPress(QKeyEvent *event) {
    if (!m_sharedController) return;
    m_sharedController->handleKeyPress(event);
}

void CameraControllerQml::handleKeyRelease(QKeyEvent *event) {
    if (!m_sharedController) return;
    m_sharedController->handleKeyRelease(event);
}

void CameraControllerQml::handleMousePress(QMouseEvent *event, QQuickWindow *window) {
    if (!m_sharedController) return;
    bool wasMoving = m_sharedController->isMovingCamera();
    m_sharedController->handleMousePress(event, window);
    if (wasMoving != m_sharedController->isMovingCamera()) {
        emit isMovingCameraChanged();
    }
}

void CameraControllerQml::handleMouseMove(QMouseEvent *event, QQuickWindow *window, const QPoint &localPos, const QRect &rect) {
    if (!m_sharedController) return;
    m_sharedController->handleMouseMove(event, window, localPos, rect);
    checkAndEmitChanges();
}

void CameraControllerQml::handleMouseRelease(QMouseEvent *event, QQuickWindow *window) {
    if (!m_sharedController) return;
    bool wasMoving = m_sharedController->isMovingCamera();
    m_sharedController->handleMouseRelease(event, window);
    if (wasMoving != m_sharedController->isMovingCamera()) {
        emit isMovingCameraChanged();
    }
    checkAndEmitChanges();
}

void CameraControllerQml::handleWheel(QWheelEvent *event) {
    if (!m_sharedController) return;
    m_sharedController->handleWheel(event);
    checkAndEmitChanges();
}

void CameraControllerQml::checkAndEmitChanges() {
    if (!m_sharedController) return;

    // Vérifier la position
    QVector3D currentPos = m_sharedController->camera().position();
    if (m_lastPosition != currentPos) {
        m_lastPosition = currentPos;
        emit positionChanged();
    }

    // Vérifier yaw
    float currentYaw = m_sharedController->camera().yaw();
    if (!qFuzzyCompare(m_lastYaw, currentYaw)) {
        m_lastYaw = currentYaw;
        emit yawChanged();
    }

    // Vérifier pitch
    float currentPitch = m_sharedController->camera().pitch();
    if (!qFuzzyCompare(m_lastPitch, currentPitch)) {
        m_lastPitch = currentPitch;
        emit pitchChanged();
    }

    // Vérifier speed
    float currentSpeed = m_sharedController->speed();
    if (!qFuzzyCompare(m_lastSpeed, currentSpeed)) {
        m_lastSpeed = currentSpeed;
        emit speedChanged();
    }

    // Vérifier orbitDistance
    float currentOrbitDistance = m_sharedController->orbitDistance();
    if (!qFuzzyCompare(m_lastOrbitDistance, currentOrbitDistance)) {
        m_lastOrbitDistance = currentOrbitDistance;
        emit orbitDistanceChanged();
    }

    // Vérifier isMoving
    bool currentIsMoving = m_sharedController->isMovingCamera();
    if (m_lastIsMoving != currentIsMoving) {
        m_lastIsMoving = currentIsMoving;
        emit isMovingCameraChanged();
    }
}
