#ifndef CLAYAPP_MYGLITEM_H
#define CLAYAPP_MYGLITEM_H

#include <QQuickFramebufferObject>
#include "InputManager.h"
#include "Camera.h"

class MyGLItem : public QQuickFramebufferObject {
    Q_OBJECT

public:
    explicit MyGLItem(QQuickItem *parent = nullptr);

    Q_PROPERTY(int gridResolution READ gridResolution WRITE setGridResolution NOTIFY gridResolutionChanged)

    [[nodiscard]] Renderer *createRenderer() const override;

    [[nodiscard]] int gridResolution() const { return m_gridResolution; }
    void setGridResolution(int r);

signals:
    void gridResolutionChanged();

protected:
    // On délègue la gestion des événements à InputManager
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    // On expose l'état pour synchronisation avec le renderer
    friend class GLRenderer;
    InputManager m_input;
    Camera m_camera;
    float m_yaw = -90.f;
    float m_pitch = 0.f;
    float m_mouseSensitivity = 0.15f;
    int m_gridResolution = 100;
};

#endif // CLAYAPP_MYGLITEM_H
