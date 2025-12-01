#ifndef CLAYAPP_GLRENDERER_H
#define CLAYAPP_GLRENDERER_H

#include <QQuickFramebufferObject>
#include <QElapsedTimer>

#include "CameraController.h"
#include "TerrainGpu.h"
#include "RaycastController.h"
#include "BrushManager.h"
#include "TerrainBrushOp.h"
#include "RenderState.h"
#include "UndoRedoManager.h"

class GLViewport; // forward


class GLRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLExtraFunctions {
public:
    GLRenderer(GLViewport *viewport);
    void synchronize(QQuickFramebufferObject *item) override;
    void render() override;
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;

    bool exportHeightmap(const QString &filePath);

private:
    // Constantes
    static constexpr float TERRAIN_MIN_X = -50.0f;
    static constexpr float TERRAIN_MAX_X = 50.0f;
    static constexpr float TERRAIN_MIN_Z = -50.0f;
    static constexpr float TERRAIN_MAX_Z = 50.0f;

    // Synchronisation (GUI -> Render thread)
    void syncViewportState();
    void syncTerrain();
    void syncBrushManager();
    void syncMousePosition();
    void syncExportRequest();

    // Boucle de rendu
    float computeDeltaTime();
    void updateFPS(float dt);
    void updateCamera(float dt, bool &cameraDirty);
    bool shouldRenderFrame(bool cameraDirty) const;
    
    void drawFrame();
    void processRaycast(const QMatrix4x4 &proj, const QMatrix4x4 &view);

    // Application des brushes
    void applyPendingStrokes();
    void applyContinuousBrush();
    void applyBrushAtPosition(const QVector3D &worldPos, int brushIndex,
                              float size, float strength, BrushOpType operation);

    // Undo/Redo
    void performUndo();
    void performRedo();

    // Helpers
    void initializeBrushManager();
    void linkQmlControllers();
    void updateBrushAsyncLoading();
    bool canApplyContinuousBrush() const;

private:
    // Chronomètre pour le calcul du delta time
    QElapsedTimer m_timer;

    // Pointeur vers le viewport QML
    GLViewport *m_viewport = nullptr;

    // État global du rendu (flags, cache, etc.)
    RenderState m_state;

    // Ressources GPU pour le rendu du terrain
    TerrainGpu m_terrainGpu;

    // Contrôleurs partagés entre le thread GUI et le thread de rendu
    CameraController m_cameraController;
    RaycastController m_raycastController;
    BrushManager m_brushManager;

    // Opérateur de brush pour modifier le terrain
    TerrainBrushOp m_brushOp;

    // Gestionnaire d'undo/redo
    UndoRedoManager m_undoRedoManager;
};

#endif // CLAYAPP_GLRENDERER_H
