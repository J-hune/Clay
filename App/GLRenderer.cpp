#include "GLRenderer.h"
#include "GLViewport.h"
#include "TerrainGpu.h"
#include "CameraController.h"
#include "Grid.h"
#include "ImageUtils.h"
#include <QOpenGLFramebufferObjectFormat>
#include <QQuickWindow>
#include <QMatrix4x4>
#include <QtGlobal>
#include <iostream>

GLRenderer::GLRenderer(GLViewport *viewport) : m_viewport(viewport) {
    initializeOpenGLFunctions();
    m_timer.start();
    m_terrainGpu.initialize(this);
    m_terrainRaycast.initialize(this);
}

void GLRenderer::synchronize(QQuickFramebufferObject *item) {
    // On s'attend à recevoir le même viewport passé au constructeur
    auto *glItem = qobject_cast<GLViewport*>(item);
    if (!glItem) return;

    // fps -> item
    glItem->setFpsFromRenderer(m_fpsAccum);

    // Détection des changements grille / axes nécessitant un redraw unique
    m_grid.setResolution(glItem->gridResolution());
    m_drawGrid = glItem->drawGrid();
    m_drawAxes = glItem->drawAxes();
    if (m_prevGridResolution != m_grid.resolution() || m_prevDrawGrid != m_drawGrid || m_prevDrawAxes != m_drawAxes) {
        m_needRedraw = true; // changements statiques
        m_prevGridResolution = m_grid.resolution();
        m_prevDrawGrid = m_drawGrid;
        m_prevDrawAxes = m_drawAxes;
    }

    // Gestion révision terrain GPU (rebuild GPU seulement, mesh CPU déjà généré côté viewport)
    if (glItem->userRequestedTerrain() && glItem->terrainRevision() != m_lastTerrainRevision) {
        m_lastTerrainRevision = glItem->terrainRevision();
        m_terrainGpu.setGridResolution(glItem->terrainResolution(), glItem->terrainResolution());
        m_terrainGpu.setTextureResolution(glItem->heightmapResolution());
        if (glItem->terrainMode() == 1 && !glItem->heightmapSource().isEmpty()) {
            const QImage img = loadHeightImage(glItem->heightmapSource());
            if (!img.isNull()) m_terrainGpu.rebuild(this, img, glItem->heightScale());
            else m_terrainGpu.rebuildFlat(this, glItem->heightScale());
        } else {
            m_terrainGpu.rebuildFlat(this, glItem->heightScale());
        }
        m_terrainReady = true;
        m_needRedraw = true; // première frame après rebuild
    }

    // Si on a besoin d'un redraw statique (terrain/grille/axes) on redemande update.
    if (m_needRedraw) glItem->update();
}

void GLRenderer::render() {
    const qint64 ns = m_timer.nsecsElapsed();
    m_timer.restart();
    const double dtSec = qBound(0.0, static_cast<double>(ns) / 1e9, 0.1);
    const float dt = static_cast<float>(dtSec);

    // Mise à jour FPS
    if (dt > 0.f) {
        const float instantFps = 1.0f / dt;
        if (m_fpsAccum < 0.f) m_fpsAccum = instantFps;
        else m_fpsAccum = m_fpsAccum * 0.9f + instantFps * 0.1f;
    }

    // Mise à jour de la camera
    bool cameraDirty = false;
    if (m_viewport) {
        m_viewport->m_cameraController.update(dt); // avance la caméra (marque dirty si changé)
        cameraDirty = m_viewport->m_cameraController.camera().isDirty();

        if (cameraDirty) {
            m_viewport->m_cameraController.camera().clearDirty();
            emit m_viewport->cameraPositionChanged();
            emit m_viewport->yawChanged();
            emit m_viewport->pitchChanged();
        }
    }

    // Politique de redraw minimal: on dessine uniquement si quelque chose a changé.
    const bool needFrame = m_needRedraw || cameraDirty;
    if (!needFrame) {
        // Pas de changement: on ne refait pas le clear/draw -> laisse le FBO tel quel.
        // Pour Qt Quick FBO renderer, on doit tout de même invalider GL state proprement si nécessaire.
        return;
    }

    // Configuration OpenGL
    glViewport(0, 0, framebufferObject()->width(), framebufferObject()->height());
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.129f, 0.141f, 0.161f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const int w = framebufferObject()->width();
    const int h = framebufferObject()->height();
    const float aspect = h > 0 ? static_cast<float>(w) / static_cast<float>(h) : 1.0f;

    QMatrix4x4 proj; proj.perspective(60.f, aspect, 0.1f, 1000.f);
    proj.scale(1.f, -1.f, 1.f);
    QMatrix4x4 view;
    if (m_viewport) view = m_viewport->m_cameraController.camera().viewMatrix();

    glMatrixMode(GL_PROJECTION); glLoadMatrixf(proj.constData());
    glMatrixMode(GL_MODELVIEW); glLoadMatrixf(view.constData());

    m_grid.draw(this, m_drawGrid, m_drawAxes);

    if (m_terrainReady) {
        m_terrainGpu.draw(this, proj, view);

        // Si un raycast est demandé, on l'exécute
        if (m_viewport && m_viewport->m_raycastRequested) {
            m_viewport->m_raycastRequested = false;

            // On configure les bounds du terrain
            m_terrainRaycast.setTerrainBounds(-50.0f, 50.0f, 50.0f, -50.0f);
            m_terrainRaycast.performRaycast(
                this,
                m_viewport->m_mouseNDC,
                proj,
                view,
                m_terrainGpu.heightmapTexture(),
                m_viewport->heightmapResolution(),
                m_viewport->heightScale()
            );

            // On lit le résultat
            RaycastResult result = m_terrainRaycast.readResult(this);

            if (result.hitFlag > 0.5f) {
                const QVector3D hitPos(result.posX, result.posY, result.posZ);
                m_terrainGpu.setRaycastHit(hitPos);
                m_needRedraw = true;
            } else {
                m_terrainGpu.clearRaycastHit();
            }
        }
    }

    // Reset redraw ponctuel
    if (m_needRedraw) m_needRedraw = false;

    // Si input actif ou caméra encore dirty (mouvement continu), on schedule la frame suivante
    update();
}

QOpenGLFramebufferObject *GLRenderer::createFramebufferObject(const QSize &size) {
    QOpenGLFramebufferObjectFormat fmt;
    fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    m_needRedraw = true;
    return new QOpenGLFramebufferObject(size, fmt);
}
