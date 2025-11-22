#ifndef CLAYAPP_CAMERA_H
#define CLAYAPP_CAMERA_H

#include <QVector3D>
#include <QMatrix4x4>
#include <QtMath>

#include "InputManager.h"

// Caméra FPS simple
class Camera {
public:
    void update(const float dt, const InputManager &input, const float yaw, const float pitch) {
        // On calcule les directions
        QVector3D front(
            cosf(qDegreesToRadians(yaw)) * cosf(qDegreesToRadians(pitch)),
            sinf(qDegreesToRadians(pitch)),
            sinf(qDegreesToRadians(yaw)) * cosf(qDegreesToRadians(pitch))
        );
        front.normalize();
        const QVector3D right = QVector3D::crossProduct(front, QVector3D(0, 1, 0)).normalized();
        const QVector3D forwardXZ = (front - QVector3D(0, front.y(), 0)).normalized();

        const float speed = m_speed; // vitesse configurable
        if (input.moveForward()) m_position += forwardXZ * (speed * dt);
        if (input.moveBackward()) m_position -= forwardXZ * (speed * dt);
        if (input.moveLeft()) m_position -= right * (speed * dt);
        if (input.moveRight()) m_position += right * (speed * dt);

        m_front = front;
    }

    QMatrix4x4 viewMatrix() const {
        QMatrix4x4 view;
        view.lookAt(m_position, m_position + m_front, QVector3D(0, 1, 0));
        return view;
    }

    const QVector3D &position() const { return m_position; }
    void setPosition(const QVector3D &p) { m_position = p; }

    const QVector3D &frontVector() const { return m_front; }

    float speed() const { return m_speed; }
    void setSpeed(const float s) { m_speed = s < 0.f ? 0.f : s; }

private:
    QVector3D m_position{0.f, 1.8f, 5.f};
    QVector3D m_front{0.f, 0.f, -1.f};
    float m_speed = 5.0f; // vitesse par défaut
};

#endif // CLAYAPP_CAMERA_H
