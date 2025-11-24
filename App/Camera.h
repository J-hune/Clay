#ifndef CLAYAPP_CAMERA_H
#define CLAYAPP_CAMERA_H

#include <QVector3D>
#include <QMatrix4x4>
#include <QtMath>

// Caméra purement mathématique : stocke position/orientation, sans gérer directement les entrées utilisateur.
class Camera {
public:
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

    void setPosition(const QVector3D &p) {
        if (m_position != p) {
            m_position = p;
            m_dirty = true;
        }
    }

    void setYaw(const float y) {
        if (!qFuzzyCompare(m_yaw, y)) {
            m_yaw = y;
            m_dirty = true;
        }
    }

    void setPitch(const float p) {
        const float clamped = qBound(-89.f, p, 89.f);
        if (!qFuzzyCompare(m_pitch, clamped)) {
            m_pitch = clamped;
            m_dirty = true;
        }
    }

    void setSpeed(const float s) {
        const float clamped = s < 0.f ? 0.f : s;
        if (!qFuzzyCompare(m_speed, clamped)) {
            m_speed = clamped;
            m_dirty = true;
        }
    }

    void setMouseSensitivity(const float s) {
        const float clamped = s < 0.f ? 0.f : s;
        if (!qFuzzyCompare(m_mouseSensitivity, clamped)) {
            m_mouseSensitivity = clamped;
            m_dirty = true;
        }
    }

    void setOrbitPivot(const QVector3D &p) {
        if (m_orbitPivot != p) {
            m_orbitPivot = p;
            m_dirty = true;
        }
    }

    void setOrbitDistance(const float d) {
        const float clamped = d < 0.01f ? 0.01f : d;
        if (!qFuzzyCompare(m_orbitDistance, clamped)) {
            m_orbitDistance = clamped;
            m_dirty = true;
        }
    }

    // Met à jour le vecteur front à partir de yaw/pitch ; à appeler après modification de yaw/pitch
    void recomputeFront() {
        QVector3D front(
            cosf(qDegreesToRadians(m_yaw)) * cosf(qDegreesToRadians(m_pitch)),
            sinf(qDegreesToRadians(m_pitch)),
            sinf(qDegreesToRadians(m_yaw)) * cosf(qDegreesToRadians(m_pitch))
        );
        front.normalize();
        if (m_front != front) { m_front = front; m_dirty = true; }
    }

    // Dirty flag: indique si la caméra a changé depuis la dernière fois
    bool isDirty() const { return m_dirty; }
    void clearDirty() { m_dirty = false; }

private:
    QVector3D m_position{0.f, 1.8f, 5.f};
    QVector3D m_front{0.f, 0.f, -1.f};
    float m_speed = 5.0f; // vitesse par défaut FPS
    QVector3D m_orbitPivot{0.f, 1.8f, 0.f};
    float m_orbitDistance = 5.f; // distance orbit initiale
    float m_yaw = -90.f;
    float m_pitch = 0.f;
    float m_mouseSensitivity = 0.15f;
    bool m_dirty = true; // on considère qu'elle a changé au départ
};

#endif // CLAYAPP_CAMERA_H
