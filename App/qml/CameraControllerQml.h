#ifndef CLAYAPP_CAMERACONTROLLERQML_H
#define CLAYAPP_CAMERACONTROLLERQML_H

#include <QKeyEvent>
#include "../CameraController.h"

/**
 * @brief Wrapper QObject pour CameraController, exposable au QML
 * Ne possède PAS son propre CameraController, mais agit comme proxy vers une instance partagée
 */
class CameraControllerQml : public QObject {
    Q_OBJECT

    Q_PROPERTY(QVector3D position READ position NOTIFY positionChanged)
    Q_PROPERTY(float yaw READ yaw NOTIFY yawChanged)
    Q_PROPERTY(float pitch READ pitch NOTIFY pitchChanged)
    Q_PROPERTY(float speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(float mouseSensitivity READ mouseSensitivity WRITE setMouseSensitivity NOTIFY mouseSensitivityChanged)
    Q_PROPERTY(float orbitDistance READ orbitDistance WRITE setOrbitDistance NOTIFY orbitDistanceChanged)
    Q_PROPERTY(float speedMin READ speedMin CONSTANT)
    Q_PROPERTY(float speedMax READ speedMax CONSTANT)
    Q_PROPERTY(float orbitDistanceMin READ orbitDistanceMin CONSTANT)
    Q_PROPERTY(float orbitDistanceMax READ orbitDistanceMax CONSTANT)
    Q_PROPERTY(bool isMovingCamera READ isMovingCamera NOTIFY isMovingCameraChanged)

public:
    explicit CameraControllerQml(QObject *parent = nullptr);

    // Injection du CameraController partagé (appelé par GLRenderer)
    void setSharedController(CameraController *controller);
    CameraController* sharedController() const { return m_sharedController; }

    // Getters pour Q_PROPERTY
    QVector3D position() const;
    float yaw() const;
    float pitch() const;
    float speed() const;
    float mouseSensitivity() const;
    float orbitDistance() const;
    float speedMin() const;
    float speedMax() const;
    float orbitDistanceMin() const;
    float orbitDistanceMax() const;
    bool isMovingCamera() const;

    // Setters
    void setSpeed(float s);
    void setMouseSensitivity(float s);
    void setOrbitDistance(float d);

    // Méthodes invocables depuis QML
    Q_INVOKABLE void handleKeyPress(QKeyEvent *event);
    Q_INVOKABLE void handleKeyRelease(QKeyEvent *event);
    Q_INVOKABLE void handleMousePress(QMouseEvent *event, QQuickWindow *window);
    Q_INVOKABLE void handleMouseMove(QMouseEvent *event, QQuickWindow *window, const QPoint &localPos, const QRect &rect);
    Q_INVOKABLE void handleMouseRelease(QMouseEvent *event, QQuickWindow *window);
    Q_INVOKABLE void handleWheel(QWheelEvent *event);

signals:
    void positionChanged();
    void yawChanged();
    void pitchChanged();
    void speedChanged();
    void mouseSensitivityChanged();
    void orbitDistanceChanged();
    void isMovingCameraChanged();

private:
    CameraController *m_sharedController = nullptr;  // Référence partagée
    QVector3D m_lastPosition;
    float m_lastYaw = -90.0f;
    float m_lastPitch = 0.0f;
    float m_lastSpeed = 5.0f;
    float m_lastOrbitDistance = 5.0f;
    bool m_lastIsMoving = false;

    void checkAndEmitChanges();
};

#endif // CLAYAPP_CAMERACONTROLLERQML_H

