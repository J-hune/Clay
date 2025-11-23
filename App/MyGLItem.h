#ifndef CLAYAPP_MYGLITEM_H
#define CLAYAPP_MYGLITEM_H

#include <QQuickFramebufferObject>
#include <QVector3D>

#include "BrushManager.h"
#include "CameraController.h"

class MyGLItem : public QQuickFramebufferObject {
    Q_OBJECT

public:
    explicit MyGLItem(QQuickItem *parent = nullptr);

    Q_PROPERTY(int gridResolution READ gridResolution WRITE setGridResolution NOTIFY gridResolutionChanged)
    Q_PROPERTY(QVector3D cameraPosition READ cameraPosition NOTIFY cameraPositionChanged)
    Q_PROPERTY(float yaw READ yaw NOTIFY yawChanged)
    Q_PROPERTY(float pitch READ pitch NOTIFY pitchChanged)
    Q_PROPERTY(float fps READ fps NOTIFY fpsChanged)
    Q_PROPERTY(float mouseSensitivity READ mouseSensitivity WRITE setMouseSensitivity NOTIFY mouseSensitivityChanged)
    Q_PROPERTY(bool drawGrid READ drawGrid WRITE setDrawGrid NOTIFY drawGridChanged)
    Q_PROPERTY(bool drawAxes READ drawAxes WRITE setDrawAxes NOTIFY drawAxesChanged)
    Q_PROPERTY(float cameraSpeed READ cameraSpeed WRITE setCameraSpeed NOTIFY cameraSpeedChanged)
    Q_PROPERTY(float orbitDistance READ orbitDistance WRITE setOrbitDistance NOTIFY orbitDistanceChanged)
    Q_PROPERTY(float cameraSpeedMin READ cameraSpeedMin CONSTANT)
    Q_PROPERTY(float cameraSpeedMax READ cameraSpeedMax CONSTANT)
    Q_PROPERTY(float orbitDistanceMin READ orbitDistanceMin CONSTANT)
    Q_PROPERTY(float orbitDistanceMax READ orbitDistanceMax CONSTANT)
    Q_PROPERTY(QString selectedBrush READ selectedBrush WRITE setSelectedBrush NOTIFY selectedBrushChanged)
    Q_PROPERTY(float brushSize READ brushSize WRITE setBrushSize NOTIFY brushSizeChanged)
    Q_PROPERTY(float brushStrength READ brushStrength WRITE setBrushStrength NOTIFY brushStrengthChanged)

    [[nodiscard]] Renderer *createRenderer() const override;

    // Getters accessibles depuis QML
    QVector3D cameraPosition() const { return m_cameraController.camera().position(); }
    float yaw() const { return m_cameraController.camera().yaw(); }
    float pitch() const { return m_cameraController.camera().pitch(); }
    float fps() const { return m_fps; }
    float cameraSpeed() const { return m_cameraController.speed(); }
    float mouseSensitivity() const { return m_cameraController.mouseSensitivity(); }
    int gridResolution() const { return m_gridResolution; }
    bool drawGrid() const { return m_drawGrid; }
    bool drawAxes() const { return m_drawAxes; }
    float orbitDistance() const { return m_cameraController.orbitDistance(); }
    float cameraSpeedMin() const { return m_cameraController.cameraSpeedMin(); }
    float cameraSpeedMax() const { return m_cameraController.cameraSpeedMax(); }
    float orbitDistanceMin() const { return m_cameraController.orbitDistanceMin(); }
    float orbitDistanceMax() const { return m_cameraController.orbitDistanceMax(); }
    QString selectedBrush() const { return m_brushManager.selectedBrush(); }
    float brushSize() const { return m_brushManager.brushSize(); }
    float brushStrength() const { return m_brushManager.brushStrength(); }


    // Setters modifiables depuis QML
    void setCameraSpeed(float s);
    void setMouseSensitivity(float s);
    void setGridResolution(int r);
    void setDrawGrid(bool v);
    void setDrawAxes(bool v);
    void setOrbitDistance(float d);
    void setBrushSize(float s);
    void setBrushStrength(float s);
    void setSelectedBrush(const QString &p);

signals:
    void gridResolutionChanged();
    void cameraPositionChanged();
    void yawChanged();
    void pitchChanged();
    void fpsChanged();
    void mouseSensitivityChanged();
    void drawGridChanged();
    void drawAxesChanged();
    void cameraSpeedChanged();
    void orbitDistanceChanged();
    void selectedBrushChanged();
    void brushSizeChanged();
    void brushStrengthChanged();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;

private:
    friend class GLRenderer;
    CameraController m_cameraController;
    int m_gridResolution = 100;
    float m_fps = 0.f;
    bool m_drawGrid = true;
    bool m_drawAxes = true;
    BrushManager m_brushManager = BrushManager(this);
};

#endif // CLAYAPP_MYGLITEM_H
