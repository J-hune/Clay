#ifndef CLAYAPP_GLRENDERER_H
#define CLAYAPP_GLRENDERER_H

#include <QQuickFramebufferObject>
#include <QElapsedTimer>

#include "CameraController.h"
#include "Grid.h"
#include "TerrainGpu.h"
#include "RaycastController.h"
#include "BrushManager.h"

class GLViewport; // forward

enum class RedrawReason {
    None            = 0,
    TerrainChanged  = 1 << 0,
    GridChanged     = 1 << 1,
    CameraMoved     = 1 << 2,
    RaycastChanged  = 1 << 3
};

inline RedrawReason operator|(RedrawReason a, RedrawReason b) {
    return static_cast<RedrawReason>(static_cast<int>(a) | static_cast<int>(b));
}
inline RedrawReason& operator|=(RedrawReason &a, RedrawReason b) {
    a = a | b;
    return a;
}
inline bool operator&(RedrawReason a, RedrawReason b) {
    return (static_cast<int>(a) & static_cast<int>(b)) != 0;
}

class GLRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLExtraFunctions {
public:
    GLRenderer(GLViewport *viewport);
    void synchronize(QQuickFramebufferObject *item) override;
    void render() override;
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;

private:
    // Synchronize helpers
    void syncViewportState();
    void syncTerrain();
    void syncInteractionState();
    void syncBrushState();

    // Render helpers
    float computeDeltaTime();
    void updateFPS(float dt);
    void updateCamera(float dt, bool &cameraDirty);
    bool shouldRenderFrame(bool cameraDirty) const;
    void prepareGLState();
    void computeMatrices(QMatrix4x4 &proj, QMatrix4x4 &view);
    void drawScene(const QMatrix4x4 &proj, const QMatrix4x4 &view);
    void processRaycast(const QMatrix4x4 &proj, const QMatrix4x4 &view);
    void finalizeFrame();

    // Brush application
    void applyPendingStrokes();

    // Redraw policy
    void requestRedraw(RedrawReason reason);

    // Matrix helpers
    void updateProjectionMatrix(int width, int height, QMatrix4x4 &proj);
    void updateViewMatrix(const Camera &camera, QMatrix4x4 &view);

private:
    QElapsedTimer m_timer;
    GLViewport *m_viewport = nullptr;
    Grid m_grid;
    bool m_drawGrid = true;
    bool m_drawAxes = true;
    float m_fpsAccum = -1.f;
    TerrainGpu m_terrainGpu;
    bool m_terrainReady = false;
    int m_lastTerrainRevision = -1;

    // Etats précédents pour détecter un changement côté QML
    int m_prevGridResolution = -1;
    bool m_prevDrawGrid = true;
    bool m_prevDrawAxes = true;

    // Raycast
    RaycastController m_raycastController;
    bool m_mouseMoved = false;

    // Brushes
    BrushManager m_brushManager;
    bool m_brushModelRefreshed = false;

    // Redraw management
    RedrawReason m_redrawReasons = RedrawReason::None;
};

#endif // CLAYAPP_GLRENDERER_H
