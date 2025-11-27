#include "CameraControllerQml.h"

CameraControllerQml::CameraControllerQml(QObject *parent)
    : QObject(parent) {
    m_lastPosition = m_controller.camera().position();
    m_lastYaw = m_controller.camera().yaw();
    m_lastPitch = m_controller.camera().pitch();
    m_lastSpeed = m_controller.speed();
    m_lastOrbitDistance = m_controller.orbitDistance();
}

void CameraControllerQml::setSpeed(float s) {
    m_controller.setSpeed(s);
    if (!qFuzzyCompare(m_lastSpeed, m_controller.speed())) {
        m_lastSpeed = m_controller.speed();
        emit speedChanged();
    }
}

void CameraControllerQml::setMouseSensitivity(float s) {
    float old = m_controller.mouseSensitivity();
    m_controller.setMouseSensitivity(s);
    if (!qFuzzyCompare(old, m_controller.mouseSensitivity())) {
        emit mouseSensitivityChanged();
    }
}

void CameraControllerQml::setOrbitDistance(float d) {
    m_controller.setOrbitDistance(d);
    if (!qFuzzyCompare(m_lastOrbitDistance, m_controller.orbitDistance())) {
        m_lastOrbitDistance = m_controller.orbitDistance();
        emit orbitDistanceChanged();
    }
}

void CameraControllerQml::handleKeyPress(QKeyEvent *event) {
    m_controller.handleKeyPress(event);
}

void CameraControllerQml::handleKeyRelease(QKeyEvent *event) {
    m_controller.handleKeyRelease(event);
}

void CameraControllerQml::handleMousePress(QMouseEvent *event, QQuickWindow *window) {
    bool wasMoving = m_controller.isMovingCamera();
    m_controller.handleMousePress(event, window);
    if (wasMoving != m_controller.isMovingCamera()) {
        emit isMovingCameraChanged();
    }
}

void CameraControllerQml::handleMouseMove(QMouseEvent *event, QQuickWindow *window, const QPoint &localPos, const QRect &rect) {
    m_controller.handleMouseMove(event, window, localPos, rect);
    checkAndEmitChanges();
}

void CameraControllerQml::handleMouseRelease(QMouseEvent *event, QQuickWindow *window) {
    bool wasMoving = m_controller.isMovingCamera();
    m_controller.handleMouseRelease(event, window);
    if (wasMoving != m_controller.isMovingCamera()) {
        emit isMovingCameraChanged();
    }
    checkAndEmitChanges();
}

void CameraControllerQml::handleWheel(QWheelEvent *event) {
    m_controller.handleWheel(event);
    checkAndEmitChanges();
}

void CameraControllerQml::update(float dt) {
    m_controller.update(dt);
    checkAndEmitChanges();
}

void CameraControllerQml::checkAndEmitChanges() {
    bool changed = false;

    // Vérifier la position
    QVector3D currentPos = m_controller.camera().position();
    if (m_lastPosition != currentPos) {
        m_lastPosition = currentPos;
        emit positionChanged();
        changed = true;
    }

    // Vérifier yaw
    float currentYaw = m_controller.camera().yaw();
    if (!qFuzzyCompare(m_lastYaw, currentYaw)) {
        m_lastYaw = currentYaw;
        emit yawChanged();
        changed = true;
    }

    // Vérifier pitch
    float currentPitch = m_controller.camera().pitch();
    if (!qFuzzyCompare(m_lastPitch, currentPitch)) {
        m_lastPitch = currentPitch;
        emit pitchChanged();
        changed = true;
    }

    // Vérifier speed
    float currentSpeed = m_controller.speed();
    if (!qFuzzyCompare(m_lastSpeed, currentSpeed)) {
        m_lastSpeed = currentSpeed;
        emit speedChanged();
        changed = true;
    }

    // Vérifier orbitDistance
    float currentOrbitDistance = m_controller.orbitDistance();
    if (!qFuzzyCompare(m_lastOrbitDistance, currentOrbitDistance)) {
        m_lastOrbitDistance = currentOrbitDistance;
        emit orbitDistanceChanged();
        changed = true;
    }

    // Vérifier isMoving
    bool currentIsMoving = m_controller.isMovingCamera();
    if (m_lastIsMoving != currentIsMoving) {
        m_lastIsMoving = currentIsMoving;
        emit isMovingCameraChanged();
        changed = true;
    }
}
