#ifndef CLAYAPP_MYGLITEM_H
#define CLAYAPP_MYGLITEM_H

#include <QQuickFramebufferObject>
#include <QVector3D>
#include "InputManager.h"
#include "Camera.h"

class MyGLItem : public QQuickFramebufferObject {
    Q_OBJECT

public:
    explicit MyGLItem(QQuickItem *parent = nullptr);

    Q_PROPERTY(int gridResolution READ gridResolution WRITE setGridResolution NOTIFY gridResolutionChanged)
    Q_PROPERTY(QVector3D cameraPosition READ cameraPosition NOTIFY cameraPositionChanged)
    Q_PROPERTY(float yaw READ yaw NOTIFY yawChanged)
    Q_PROPERTY(float pitch READ pitch NOTIFY pitchChanged)
    Q_PROPERTY(float fps READ fps NOTIFY fpsChanged)
    Q_PROPERTY(float cameraSpeed READ cameraSpeed WRITE setCameraSpeed NOTIFY cameraSpeedChanged)
    Q_PROPERTY(float mouseSensitivity READ mouseSensitivity WRITE setMouseSensitivity NOTIFY mouseSensitivityChanged)
    Q_PROPERTY(bool drawGrid READ drawGrid WRITE setDrawGrid NOTIFY drawGridChanged)
    Q_PROPERTY(bool drawAxes READ drawAxes WRITE setDrawAxes NOTIFY drawAxesChanged)
    Q_PROPERTY(float orbitDistance READ orbitDistance WRITE setOrbitDistance NOTIFY orbitDistanceChanged)

    [[nodiscard]] Renderer *createRenderer() const override;

    // Getters accessibles depuis QML
    QVector3D cameraPosition() const { return m_camera.position(); }
    float yaw() const { return m_camera.yaw(); }
    float pitch() const { return m_camera.pitch(); }
    float fps() const { return m_fps; }
    float cameraSpeed() const { return m_camera.speed(); }
    float mouseSensitivity() const { return m_camera.mouseSensitivity(); }
    int gridResolution() const { return m_gridResolution; }
    bool drawGrid() const { return m_drawGrid; }
    bool drawAxes() const { return m_drawAxes; }
    float orbitDistance() const { return m_camera.orbitDistance(); }

    // Setters modifiables depuis QML
    void setCameraSpeed(float s);
    void setMouseSensitivity(float s);
    void setGridResolution(int r);
    void setDrawGrid(bool v);
    void setDrawAxes(bool v);
    void setOrbitDistance(float d);

signals:
    void gridResolutionChanged();
    void cameraPositionChanged();
    void yawChanged();
    void pitchChanged();
    void fpsChanged();
    void cameraSpeedChanged();
    void mouseSensitivityChanged();
    void drawGridChanged();
    void drawAxesChanged();
    void orbitDistanceChanged();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    friend class GLRenderer;
    InputManager m_input;
    Camera m_camera;
    int m_gridResolution = 100;
    float m_fps = 0.f;
    bool m_drawGrid = true;
    bool m_drawAxes = true;
};

#endif // CLAYAPP_MYGLITEM_H
