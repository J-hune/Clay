#ifndef CLAYAPP_GLRENDERER_H
#define CLAYAPP_GLRENDERER_H

#include <QQuickFramebufferObject>
#include <QElapsedTimer>

#include "CameraController.h"
#include "Grid.h"
#include "TerrainGpu.h"

class GLViewport; // forward

class GLRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions {
public:
    GLRenderer(GLViewport *viewport);
    void synchronize(QQuickFramebufferObject *item) override;
    void render() override;
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;

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
    bool m_needRedraw = false; // demande ponctuelle (terrain rebuild / changement grille / axes)

    // Etats précédents pour détecter un changement côté QML
    int m_prevGridResolution = -1;
    bool m_prevDrawGrid = true;
    bool m_prevDrawAxes = true;
};

#endif // CLAYAPP_GLRENDERER_H
