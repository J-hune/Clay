#ifndef CLAYAPP_INPUTMANAGER_H
#define CLAYAPP_INPUTMANAGER_H

#include <QPoint>
#include <QKeyEvent>
#include <QtGlobal>

// On gère les entrées clavier / souris pour un contrôle FPS
class InputManager {
public:
    void keyPress(const QKeyEvent *e) {
        switch (e->key()) {
            case Qt::Key_Z: m_moveForward = true; break;
            case Qt::Key_S: m_moveBackward = true; break;
            case Qt::Key_Q: m_moveLeft = true; break;
            case Qt::Key_D: m_moveRight = true; break;
            default: break;
        }
    }

    void keyRelease(const QKeyEvent *e) {
        switch (e->key()) {
            case Qt::Key_Z: m_moveForward = false; break;
            case Qt::Key_S: m_moveBackward = false; break;
            case Qt::Key_Q: m_moveLeft = false; break;
            case Qt::Key_D: m_moveRight = false; break;
            default: break;
        }
    }

    void mousePress(const QMouseEvent *e) {
        if (e->button() == Qt::RightButton) {
            m_rightButtonDown = true;
            m_lastMousePos = e->position().toPoint();
        }
    }

    void mouseMove(const QMouseEvent *e, float &yaw, float &pitch, const float sensitivity) {
        if (!m_rightButtonDown) return;
        const QPoint p = e->position().toPoint();
        const QPoint delta = p - m_lastMousePos;
        m_lastMousePos = p;
        yaw += static_cast<float>(delta.x()) * sensitivity;
        pitch += static_cast<float>(-delta.y()) * sensitivity;
        pitch = qBound(-89.f, pitch, 89.f);
    }

    void mouseRelease(const QMouseEvent *e) {
        if (e->button() == Qt::RightButton) {
            m_rightButtonDown = false;
        }
    }

    // On expose les états de déplacement
    bool moveForward() const { return m_moveForward; }
    bool moveBackward() const { return m_moveBackward; }
    bool moveLeft() const { return m_moveLeft; }
    bool moveRight() const { return m_moveRight; }
    bool rightButtonDown() const { return m_rightButtonDown; }

    // On synchronise avec un autre InputManager
    void copyFrom(const InputManager &o) {
        m_moveForward = o.m_moveForward;
        m_moveBackward = o.m_moveBackward;
        m_moveLeft = o.m_moveLeft;
        m_moveRight = o.m_moveRight;
        m_rightButtonDown = o.m_rightButtonDown;
        m_lastMousePos = o.m_lastMousePos;
    }

private:
    bool m_moveForward = false;
    bool m_moveBackward = false;
    bool m_moveLeft = false;
    bool m_moveRight = false;
    bool m_rightButtonDown = false;
    QPoint m_lastMousePos;
};

#endif // CLAYAPP_INPUTMANAGER_H
