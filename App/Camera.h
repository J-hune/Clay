#ifndef CLAYAPP_CAMERA_H
#define CLAYAPP_CAMERA_H

#include "InputManager.h"
#include <QVector3D>
#include <QMatrix4x4>
#include <QtMath>

// Caméra avec modes FPS (hold bouton droit) et Orbit (hold bouton milieu)
class Camera {
public:
    void update(const float dt, const InputManager &input) {
        // Calcul direction base à partir yaw/pitch
        QVector3D front(
            cosf(qDegreesToRadians(m_yaw)) * cosf(qDegreesToRadians(m_pitch)),
            sinf(qDegreesToRadians(m_pitch)),
            sinf(qDegreesToRadians(m_yaw)) * cosf(qDegreesToRadians(m_pitch))
        );
        front.normalize();
        m_front = front;

        if (input.middleButtonDown()) {
            // Entrée en mode orbit: on capture le pivot une seule fois (point visé)
            if (!m_wasOrbiting) {
                m_orbitPivot = m_position + front * m_orbitDistance;
                m_wasOrbiting = true;
            }
            // La caméra orbite autour du pivot à distance m_orbitDistance
            m_position = m_orbitPivot - front * m_orbitDistance;
            return; // Pas de mouvement FPS simultané
        } else {
            m_wasOrbiting = false; // sortie du mode orbit
        }

        if (input.rightButtonDown()) {
            const QVector3D right = QVector3D::crossProduct(front, QVector3D(0, 1, 0)).normalized();
            const float speed = m_speed;
            // Avant / arrière: direction complète (inclut Y)
            if (input.moveForward()) m_position += front * (speed * dt);
            if (input.moveBackward()) m_position -= front * (speed * dt);
            if (input.moveLeft()) m_position -= right * (speed * dt);
            if (input.moveRight()) m_position += right * (speed * dt);
            if (input.moveUp()) m_position.setY(m_position.y() + speed * dt);
            if (input.moveDown()) m_position.setY(m_position.y() - speed * dt);
        }
        // Si bouton droit non tenu: pas de déplacement
    }

    QMatrix4x4 viewMatrix() const {
        QMatrix4x4 view;
        view.lookAt(m_position, m_position + m_front, QVector3D(0, 1, 0));
        return view;
    }

    const QVector3D &position() const { return m_position; }
    const QVector3D &frontVector() const { return m_front; }
    float yaw() const { return m_yaw; }
    float pitch() const { return m_pitch; }
    float speed() const { return m_speed; }
    float mouseSensitivity() const { return m_mouseSensitivity; }
    const QVector3D &orbitPivot() const { return m_orbitPivot; }
    float orbitDistance() const { return m_orbitDistance; }

    void setPosition(const QVector3D &p) { m_position = p; }
    void setYaw(const float y) { m_yaw = y; }
    void setPitch(const float p) { m_pitch = qBound(-89.f, p, 89.f); }
    void setSpeed(const float s) { m_speed = s < 0.f ? 0.f : s; }
    void setMouseSensitivity(const float s) { m_mouseSensitivity = s < 0.f ? 0.f : s; }
    void setOrbitPivot(const QVector3D &p) { m_orbitPivot = p; }
    void setOrbitDistance(const float d) { m_orbitDistance = d < 0.01f ? 0.01f : d; }

private:
    QVector3D m_position{0.f, 1.8f, 5.f};
    QVector3D m_front{0.f, 0.f, -1.f};
    float m_speed = 5.0f; // vitesse par défaut FPS
    QVector3D m_orbitPivot{0.f, 0.f, 0.f};
    float m_orbitDistance = 5.f; // distance orbit initiale
    bool m_wasOrbiting = false; // pour détecter l'entrée en mode orbit
    float m_yaw = -90.f;
    float m_pitch = 0.f;
    float m_mouseSensitivity = 0.15f;
};

#endif // CLAYAPP_CAMERA_H
