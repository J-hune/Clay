#ifndef CLAYAPP_INPUTMANAGER_H
#define CLAYAPP_INPUTMANAGER_H

#include <QPoint>
#include <QKeyEvent>
#include <QQuickWindow>

// Gestion des entrées clavier / souris pour contrôles FPS + orbit + orientation caméra
class InputManager {
public:
    void keyPress(const QKeyEvent *e) {
        switch (e->key()) {
            case Qt::Key_Z: m_moveForward = true; break;
            case Qt::Key_S: m_moveBackward = true; break;
            case Qt::Key_Q: m_moveLeft = true; break;
            case Qt::Key_D: m_moveRight = true; break;
            case Qt::Key_E: m_moveUp = true; break;
            case Qt::Key_A: m_moveDown = true; break;
            default: break;
        }
    }

    void keyRelease(const QKeyEvent *e) {
        switch (e->key()) {
            case Qt::Key_Z: m_moveForward = false; break;
            case Qt::Key_S: m_moveBackward = false; break;
            case Qt::Key_Q: m_moveLeft = false; break;
            case Qt::Key_D: m_moveRight = false; break;
            case Qt::Key_E: m_moveUp = false; break;
            case Qt::Key_A: m_moveDown = false; break;
            default: break;
        }
    }

    void mousePress(const QMouseEvent *e, QQuickWindow *window) {
        if (e->button() == Qt::RightButton) {
            m_rightButtonDown = true;
            m_lastMousePos = e->position().toPoint();

            // On garde en mémoire la position de la souris et on la cache
            m_lastGlobalPos = window->mapFromGlobal(QCursor::pos());
            window->setCursor(QCursor(Qt::BlankCursor));
        } else if (e->button() == Qt::MiddleButton) {
            m_middleButtonDown = true;
            m_lastMousePos = e->position().toPoint();
        }
    }

    void mouseMove(const QMouseEvent *e, const QQuickWindow *w, const QPoint &localPos, const QRect &rect) {
        if (!m_rightButtonDown && !m_middleButtonDown) return;
        const QPoint p = e->position().toPoint();
        const QPoint delta = p - m_lastMousePos;
        m_lastMousePos = p;
        m_mouseDelta = delta;

        // Logique de "wrap" de la souris quand on atteint les bords de la fenêtre
        if (m_middleButtonDown) {
            if (localPos.y() < rect.top()) {
                QCursor::setPos(w->mapToGlobal(QPoint(localPos.x(), rect.bottom() - 1)));
                m_lastMousePos.setY(rect.height());
            }
            else if (localPos.y() >= rect.bottom()) {
                QCursor::setPos(w->mapToGlobal(QPoint(localPos.x(), rect.top() + 1)));
                m_lastMousePos.setY(0);
            }

            if (localPos.x() < rect.left()) {
                QCursor::setPos(w->mapToGlobal(QPoint(rect.right() - 1, localPos.y())));
                m_lastMousePos.setX(rect.width());
            }
            else if (localPos.x() >= rect.right()) {
                QCursor::setPos(w->mapToGlobal(QPoint(rect.left() + 1, localPos.y())));
                m_lastMousePos.setX(0);
            }
        }
    }

    void mouseRelease(const QMouseEvent *e, QQuickWindow *window) {
        if (e->button() == Qt::RightButton) {
            m_rightButtonDown = false;

            // On réaffiche le curseur à la position sauvegardée
            window->setCursor(QCursor(Qt::ArrowCursor));
            QCursor::setPos(window->mapToGlobal(m_lastGlobalPos));

        } else if (e->button() == Qt::MiddleButton) {
            m_middleButtonDown = false;
        }
    }

    // Accesseurs mouvements
    bool moveForward() const { return m_moveForward; }
    bool moveBackward() const { return m_moveBackward; }
    bool moveLeft() const { return m_moveLeft; }
    bool moveRight() const { return m_moveRight; }
    bool moveUp() const { return m_moveUp; }
    bool moveDown() const { return m_moveDown; }
    bool rightButtonDown() const { return m_rightButtonDown; }
    bool middleButtonDown() const { return m_middleButtonDown; }

    // Delta souris depuis le dernier événement (utilisé par MyGLItem)
    QPoint mouseDelta() const { return m_mouseDelta; }
    void clearMouseDelta() { m_mouseDelta = QPoint(0,0); }

    void copyFrom(const InputManager &o) {
        m_moveForward = o.m_moveForward;
        m_moveBackward = o.m_moveBackward;
        m_moveLeft = o.m_moveLeft;
        m_moveRight = o.m_moveRight;
        m_moveUp = o.m_moveUp;
        m_moveDown = o.m_moveDown;
        m_rightButtonDown = o.m_rightButtonDown;
        m_middleButtonDown = o.m_middleButtonDown;
        m_lastMousePos = o.m_lastMousePos;
        m_mouseDelta = o.m_mouseDelta;
    }

private:
    // États de déplacement
    bool m_moveForward = false;
    bool m_moveBackward = false;
    bool m_moveLeft = false;
    bool m_moveRight = false;
    bool m_moveUp = false;
    bool m_moveDown = false;
    bool m_rightButtonDown = false;
    bool m_middleButtonDown = false;
    QPoint m_lastMousePos;
    QPoint m_mouseDelta{0,0};
    QPoint m_lastGlobalPos;
};

#endif // CLAYAPP_INPUTMANAGER_H
