#ifndef CLAYAPP_CAMERACONTROLLER_H
#define CLAYAPP_CAMERACONTROLLER_H

#include <QPoint>
#include <QQuickWindow>
#include <QKeyEvent>
#include <QtMath>
#include "Camera.h"

class CameraController {
public:
    enum class Mode { Orbit, Fps };
    enum class MoveDirection { Forward, Backward, Left, Right, Up, Down };

    CameraController() = default;

    // Accès caméra
    Camera& camera() { return m_camera; }
    const Camera& camera() const { return m_camera; }

    // Paramètres caméra
    float speed() const { return m_camera.speed(); }
    void setSpeed(const float s) { m_camera.setSpeed(s); }
    float mouseSensitivity() const { return m_camera.mouseSensitivity(); }
    void setMouseSensitivity(const float s) { m_camera.setMouseSensitivity(s); }
    float orbitDistance() const { return m_camera.orbitDistance(); }
    void setOrbitDistance(const float d) { m_camera.setOrbitDistance(d); }
    float orbitDistanceMin() const { return m_orbitDistanceMin; }
    float orbitDistanceMax() const { return m_orbitDistanceMax; }
    float cameraSpeedMin() const { return m_cameraSpeedMin; }
    float cameraSpeedMax() const { return m_cameraSpeedMax; }

    Mode mode() const { return m_mode; }

    // Entrées clavier
    void handleKeyPress(const QKeyEvent *e) {
        switch (e->key()) {
            case Qt::Key_Z: m_moveDirections[MoveDirection::Forward] = true; break;
            case Qt::Key_S: m_moveDirections[MoveDirection::Backward] = true; break;
            case Qt::Key_Q: m_moveDirections[MoveDirection::Left] = true; break;
            case Qt::Key_D: m_moveDirections[MoveDirection::Right] = true; break;
            case Qt::Key_E: m_moveDirections[MoveDirection::Up] = true; break;
            case Qt::Key_A: m_moveDirections[MoveDirection::Down] = true; break;
            default: break;
        }
    }

    void handleKeyRelease(const QKeyEvent *e) {
        switch (e->key()) {
            case Qt::Key_Z: m_moveDirections[MoveDirection::Forward] = false; break;
            case Qt::Key_S: m_moveDirections[MoveDirection::Backward] = false; break;
            case Qt::Key_Q: m_moveDirections[MoveDirection::Left] = false; break;
            case Qt::Key_D: m_moveDirections[MoveDirection::Right] = false; break;
            case Qt::Key_E: m_moveDirections[MoveDirection::Up] = false; break;
            case Qt::Key_A: m_moveDirections[MoveDirection::Down] = false; break;
            default: break;
        }
    }

    // Entrées souris
    void handleMousePress(const QMouseEvent *e, QQuickWindow *window) {
        if (e->button() == Qt::RightButton) {
            m_rightButtonDown = true;
            m_mode = Mode::Fps;
            m_lastMousePos = e->position().toPoint();

            // Sauvegarde la position du curseur et cache-le
            m_lastGlobalPos = window->mapFromGlobal(QCursor::pos());
            window->setCursor(QCursor(Qt::BlankCursor));
        } else if (e->button() == Qt::MiddleButton) {
            m_middleButtonDown = true;
            m_mode = Mode::Orbit;
            m_lastMousePos = e->position().toPoint();
        }
    }

    void handleMouseMove(const QMouseEvent *e, const QQuickWindow *w, const QPoint &localPos, const QRect &rect) {
        if (!m_rightButtonDown && !m_middleButtonDown) return;

        const QPoint p = e->position().toPoint();
        const QPoint delta = p - m_lastMousePos;
        m_lastMousePos = p;
        m_mouseDelta = delta;

        // Mise à jour de yaw et pitch selon la souris
        if (m_rightButtonDown || m_middleButtonDown) {
            handleCursorWrap(localPos, rect, w);
            const float sens = m_camera.mouseSensitivity();
            m_camera.setYaw(m_camera.yaw() + static_cast<float>(m_mouseDelta.x()) * sens);
            m_camera.setPitch(m_camera.pitch() - static_cast<float>(m_mouseDelta.y()) * sens);
        }

        // Gestion de "wrap" de la souris
        if (m_middleButtonDown) {
            handleCursorWrap(localPos, rect, w);
        }
    }

    void handleMouseRelease(const QMouseEvent *e, QQuickWindow *window) {
        if (e->button() == Qt::RightButton) {
            m_rightButtonDown = false;

            // Restaure le curseur et repositionne-le
            window->setCursor(QCursor(Qt::ArrowCursor));
            QCursor::setPos(window->mapToGlobal(m_lastGlobalPos));

            // Recalcule un pivot aligné avec la vue actuelle
            setOrbitPivotFromCameraView();
            m_mode = Mode::Orbit; // Retour au mode Orbit
        } else if (e->button() == Qt::MiddleButton) {
            m_middleButtonDown = false;
            m_mode = Mode::Orbit;
        }
    }

    void handleWheel(QWheelEvent *event) {
        const QPoint numDegrees = event->angleDelta() / 8; // 1 "degree" = 1/8 de tour
        if (!numDegrees.isNull()) {
            if (m_mode == Mode::Orbit && !m_rightButtonDown) {
                adjustOrbitDistance(numDegrees);
            } else if (m_mode == Mode::Fps || m_rightButtonDown) {
                adjustFpsSpeed(numDegrees);
            }
        }
        event->accept();
    }

    // Mise à jour de la caméra (en fonction du temps)
    void update(const float dt) {
        m_camera.recomputeFront();
        const QVector3D front = m_camera.frontVector();

        // Mode Orbit (drag actif)
        if (m_middleButtonDown) {
            updateOrbitMode(front);
        } else {
            // Mode FPS ou Orbit idle
            updateFpsMode(front, dt);
            // Si on n'est pas en FPS (bouton droit relâché) et pas en drag orbit, on maintient la distance d'orbite
            if (!m_rightButtonDown) {
                m_camera.setPosition(m_camera.orbitPivot() - front * m_camera.orbitDistance());
            }
        }
    }

    // Copie de l'état de l'entrée/clavier/souris
    void copyInputFrom(const CameraController &o) {
        m_moveDirections = o.m_moveDirections;
        m_rightButtonDown = o.m_rightButtonDown;
        m_middleButtonDown = o.m_middleButtonDown;
        m_lastMousePos = o.m_lastMousePos;
        m_mouseDelta = o.m_mouseDelta;
        m_lastGlobalPos = o.m_lastGlobalPos;
        m_mode = o.m_mode;
    }

    // Indique s'il y a une interaction utilisateur active (touches/mouse)
    bool hasActiveInput() const {
        if (m_rightButtonDown || m_middleButtonDown) return true;
        for (auto it = m_moveDirections.constBegin(); it != m_moveDirections.constEnd(); ++it) {
            if (it.value()) return true;
        }
        return false;
    }

private:
    // Direction du mouvement
    QMap<MoveDirection, bool> m_moveDirections;

    // État souris
    bool m_rightButtonDown = false;
    bool m_middleButtonDown = false;
    QPoint m_lastMousePos;
    QPoint m_mouseDelta{0, 0};
    QPoint m_lastGlobalPos;

    // Mode courant
    Mode m_mode = Mode::Orbit;

    // Caméra
    Camera m_camera;

    // Etat de l'orbite
    bool m_wasOrbiting = false;

    const float m_cameraSpeedMin = 0.05f;
    const float m_cameraSpeedMax = 200.f;
    const float m_orbitDistanceMin = 0.1f;
    const float m_orbitDistanceMax = 500.f;

    // Méthodes supplémentaires
    void handleCursorWrap(const QPoint &localPos, const QRect &rect, const QQuickWindow *w) {
        if (localPos.y() < rect.top()) {
            QCursor::setPos(w->mapToGlobal(QPoint(localPos.x(), rect.bottom() - 1)));
            m_lastMousePos.setY(rect.height());
        } else if (localPos.y() >= rect.bottom()) {
            QCursor::setPos(w->mapToGlobal(QPoint(localPos.x(), rect.top() + 1)));
            m_lastMousePos.setY(0);
        }

        if (localPos.x() < rect.left()) {
            QCursor::setPos(w->mapToGlobal(QPoint(rect.right() - 1, localPos.y())));
            m_lastMousePos.setX(rect.width());
        } else if (localPos.x() >= rect.right()) {
            QCursor::setPos(w->mapToGlobal(QPoint(rect.left() + 1, localPos.y())));
            m_lastMousePos.setX(0);
        }
    }

    void setOrbitPivotFromCameraView() {
        const float y = m_camera.yaw();
        const float pch = m_camera.pitch();
        QVector3D front(
            std::cos(qDegreesToRadians(y)) * std::cos(qDegreesToRadians(pch)),
            std::sin(qDegreesToRadians(pch)),
            std::sin(qDegreesToRadians(y)) * std::cos(qDegreesToRadians(pch))
        );
        front.normalize();
        m_camera.setOrbitPivot(m_camera.position() + front * m_camera.orbitDistance());
    }

    void adjustOrbitDistance(const QPoint &numDegrees) {
        const float steps = static_cast<float>(numDegrees.y()) / 15.f;
        float d = m_camera.orbitDistance();
        const float factor = std::pow(1.15f, -steps);
        d *= factor;
        d = qBound(m_orbitDistanceMin, d, m_orbitDistanceMax);
        m_camera.setOrbitDistance(d);
    }

    void adjustFpsSpeed(const QPoint &numDegrees) {
        // Wheel up (positive y) => augmente la vitesse
        const float steps = static_cast<float>(numDegrees.y()) / 15.f; // 1 cran = 15°
        if (steps == 0.f) return;
        float s = m_camera.speed();
        // Multiplicatif pour des variations fluides
        const float factor = std::pow(1.15f, steps); // up => *1.15, down => /1.15
        s *= factor;
        // Bornes de sécurité
        s = qBound(m_cameraSpeedMin, s, m_cameraSpeedMax);
        m_camera.setSpeed(s);
    }

    void updateOrbitMode(const QVector3D &front) {
        if (!m_wasOrbiting) {
            m_camera.setOrbitPivot(m_camera.position() + front * m_camera.orbitDistance());
            m_wasOrbiting = true;
        }
        m_camera.setPosition(m_camera.orbitPivot() - front * m_camera.orbitDistance());
    }

    void updateFpsMode(const QVector3D &front, const float dt) {
        if (m_rightButtonDown) {
            m_mode = Mode::Fps;
            const QVector3D right = QVector3D::crossProduct(front, QVector3D(0, 1, 0)).normalized();
            const float spd = m_camera.speed();
            QVector3D pos = m_camera.position();

            if (m_moveDirections[MoveDirection::Forward]) pos += front * (spd * dt);
            if (m_moveDirections[MoveDirection::Backward]) pos -= front * (spd * dt);
            if (m_moveDirections[MoveDirection::Left]) pos -= right * (spd * dt);
            if (m_moveDirections[MoveDirection::Right]) pos += right * (spd * dt);
            if (m_moveDirections[MoveDirection::Up]) pos.setY(pos.y() + spd * dt);
            if (m_moveDirections[MoveDirection::Down]) pos.setY(pos.y() - spd * dt);

            m_camera.setPosition(pos);
        } else {
            m_mode = Mode::Orbit;
        }
    }
};

#endif // CLAYAPP_CAMERACONTROLLER_H
