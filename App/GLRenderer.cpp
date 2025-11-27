#include "GLRenderer.h"
#include "GLViewport.h"
#include "TerrainGpu.h"
#include "CameraController.h"
#include "Grid.h"
#include "ImageUtils.h"
#include "Log.h"
#include <QOpenGLFramebufferObjectFormat>
#include <QQuickWindow>
#include <QMatrix4x4>
#include <QtGlobal>

GLRenderer::GLRenderer(GLViewport *viewport) : m_viewport(viewport) {
    initializeOpenGLFunctions();
    m_timer.start();
    m_terrainGpu.initialize(this);
    m_raycastController.initialize(this);
    m_brushManager.initialize(this);
    m_brushManager.loadFromDirectory(this, QStringLiteral("brushes"));
}

void GLRenderer::synchronize(QQuickFramebufferObject *item) {
    // On s'attend à recevoir le même viewport passé au constructeur
    auto *glItem = qobject_cast<GLViewport*>(item);
    if (!glItem) return;

    m_viewport = glItem;

    syncViewportState();
    syncTerrain();
    syncBrushState();
    syncInteractionState();
}

void GLRenderer::syncViewportState() {
    if (!m_viewport) return;

    // FPS -> viewport
    m_viewport->setFpsFromRenderer(m_fpsAccum);

    // Résolution grille
    m_grid.setResolution(m_viewport->gridResolution());

    // Flags affichage
    m_drawGrid = m_viewport->drawGrid();
    m_drawAxes = m_viewport->drawAxes();

    // Synchronisation des paramètres de brush vers le BrushManager
    m_brushManager.setCurrentBrushIndex(m_viewport->brushIndex());
    m_brushManager.setBrushSize(m_viewport->brushSize());
    m_brushManager.setBrushStrength(m_viewport->brushStrength());

    // Détection des changements
    if (m_prevGridResolution != m_grid.resolution() ||
        m_prevDrawGrid != m_drawGrid ||
        m_prevDrawAxes != m_drawAxes) {
        requestRedraw(RedrawReason::GridChanged);
        m_prevGridResolution = m_grid.resolution();
        m_prevDrawGrid = m_drawGrid;
        m_prevDrawAxes = m_drawAxes;
    }
}

void GLRenderer::syncTerrain() {
    if (!m_viewport) return;

    if (m_viewport->userRequestedTerrain() &&
        m_viewport->terrainRevision() != m_lastTerrainRevision) {

        m_lastTerrainRevision = m_viewport->terrainRevision();

        // Configuration du terrain GPU
        m_terrainGpu.setGridResolution(m_viewport->terrainResolution(), m_viewport->terrainResolution());
        m_terrainGpu.setTextureResolution(m_viewport->heightmapResolution());

        // Reconstruction selon le mode
        if (m_viewport->terrainMode() == 1 && !m_viewport->heightmapSource().isEmpty()) {
            const QImage img = loadHeightImage(m_viewport->heightmapSource());
            if (!img.isNull()) {
                m_terrainGpu.rebuild(this, img, m_viewport->heightScale());
                LOG_INFO() << "Terrain reconstruit (heightmap) - res=" << m_viewport->terrainResolution()
                           << ", texRes=" << m_viewport->heightmapResolution()
                           << ", scale=" << m_viewport->heightScale();
            } else {
                m_terrainGpu.rebuildFlat(this, m_viewport->heightScale());
                LOG_WARN() << "Impossible de charger la heightmap, terrain plat utilisé";
            }
        } else {
            m_terrainGpu.rebuildFlat(this, m_viewport->heightScale());
            LOG_INFO() << "Terrain plat reconstruit - res=" << m_viewport->terrainResolution()
                       << ", texRes=" << m_viewport->heightmapResolution()
                       << ", scale=" << m_viewport->heightScale();
        }

        m_terrainReady = true;
        requestRedraw(RedrawReason::TerrainChanged);
    }
}

void GLRenderer::syncBrushState() {
    if (!m_viewport) return;

    if (m_brushManager.brushCount() <= 0) {
        m_brushManager.setCurrentBrushIndex(-1);
    } else if (!m_brushManager.isValidBrushIndex(m_brushManager.currentBrushIndex())) {
        m_brushManager.setCurrentBrushIndex(0);
    }

    // Propager les paramètres de brush vers le GPU terrain
    m_terrainGpu.setBrushPreview(
        m_brushManager.currentBrushIndex(),
        m_brushManager.brushSize(),
        m_brushManager.brushStrength()
    );
}

void GLRenderer::syncInteractionState() {
    if (!m_viewport) return;

    m_mouseMoved = m_viewport->m_raycastRequested;
    if (m_mouseMoved) {
        m_viewport->invalidateRaycast();
    }
}

void GLRenderer::render() {
    const float dt = computeDeltaTime();
    updateFPS(dt);

    bool cameraDirty = false;
    updateCamera(dt, cameraDirty);

    applyPendingStrokes();

    if (!shouldRenderFrame(cameraDirty)) {
        return;
    }

    prepareGLState();

    QMatrix4x4 proj, view;
    computeMatrices(proj, view);
    drawScene(proj, view);
    processRaycast(proj, view);
    finalizeFrame();
}

float GLRenderer::computeDeltaTime() {
    const qint64 ns = m_timer.nsecsElapsed();
    m_timer.restart();
    const double dtSec = qBound(0.0, static_cast<double>(ns) / 1e9, 0.1);
    return static_cast<float>(dtSec);
}

void GLRenderer::updateFPS(float dt) {
    if (dt > 0.f) {
        const float instantFps = 1.0f / dt;
        if (m_fpsAccum < 0.f) {
            m_fpsAccum = instantFps;
        } else {
            m_fpsAccum = m_fpsAccum * 0.9f + instantFps * 0.1f;
        }
    }
}

void GLRenderer::updateCamera(float dt, bool &cameraDirty) {
    if (!m_viewport) {
        cameraDirty = false;
        return;
    }

    m_viewport->m_cameraController.update(dt);
    cameraDirty = m_viewport->m_cameraController.camera().isDirty();

    if (cameraDirty) {
        m_viewport->m_cameraController.camera().clearDirty();
        emit m_viewport->cameraPositionChanged();
        emit m_viewport->yawChanged();
        emit m_viewport->pitchChanged();
    }
}

bool GLRenderer::shouldRenderFrame(bool cameraDirty) const {
    const bool hasRedrawReason = (m_redrawReasons != RedrawReason::None);
    return hasRedrawReason || cameraDirty || m_mouseMoved;
}

void GLRenderer::prepareGLState() {
    glViewport(0, 0, framebufferObject()->width(), framebufferObject()->height());
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.129f, 0.141f, 0.161f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLRenderer::computeMatrices(QMatrix4x4 &proj, QMatrix4x4 &view) {
    const int w = framebufferObject()->width();
    const int h = framebufferObject()->height();

    updateProjectionMatrix(w, h, proj);

    if (m_viewport) {
        updateViewMatrix(m_viewport->m_cameraController.camera(), view);
    }
}

void GLRenderer::updateProjectionMatrix(int width, int height, QMatrix4x4 &proj) {
    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    proj.perspective(60.f, aspect, 0.1f, 1000.f);
    proj.scale(1.f, -1.f, 1.f);
}

void GLRenderer::updateViewMatrix(const Camera &camera, QMatrix4x4 &view) {
    view = camera.viewMatrix();
}

void GLRenderer::drawScene(const QMatrix4x4 &proj, const QMatrix4x4 &view) {
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(proj.constData());
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(view.constData());

    m_grid.draw(this, m_drawGrid, m_drawAxes);

    if (m_terrainReady) {
        m_terrainGpu.setBrushTextureArray(m_brushManager.brushTextureArrayId());
        m_terrainGpu.draw(this, proj, view);
    }
}

void GLRenderer::processRaycast(const QMatrix4x4 &proj, const QMatrix4x4 &view) {
    if (!m_terrainReady || !m_viewport) return;

    // Si la caméra bouge, on réinitialise le raycast
    if (m_viewport->m_cameraController.isMovingCamera()) {
        m_terrainGpu.clearRaycastHit();
        m_raycastController.reset();
        m_viewport->invalidateRaycast();
        return;
    }

    // Sinon, si la souris a bougé, on effectue le raycast
    if (m_mouseMoved) {
        m_raycastController.updateMousePosition(m_viewport->m_mouseNDC);
        m_raycastController.perform(
            this,
            proj,
            view,
            m_terrainGpu.heightmapTexture(),
            m_viewport->heightmapResolution(),
            m_viewport->heightScale()
        );

        if (m_raycastController.hasHit()) {
            m_terrainGpu.setRaycastHit(m_raycastController.hitPosition());
            // TODO: plus tard, on pourra utiliser cette position pour le preview de brush
            requestRedraw(RedrawReason::RaycastChanged);
        } else {
            m_terrainGpu.clearRaycastHit();
        }
    }
}

void GLRenderer::applyPendingStrokes() {
    if (!m_terrainReady) return;

    const auto &strokes = m_brushManager.pendingStrokes();
    if (strokes.empty()) return;

    // TODO: dans une itération suivante, appeler un compute shader dans TerrainGpu
    m_brushManager.clearPendingStrokes();
}

void GLRenderer::finalizeFrame() {
    // Reset des raisons de redraw
    m_redrawReasons = RedrawReason::None;
    m_mouseMoved = false;

    // On demande la prochaine frame
    update();
}

void GLRenderer::requestRedraw(RedrawReason reason) {
    m_redrawReasons |= reason;
    QMetaObject::invokeMethod(m_viewport, "update", Qt::QueuedConnection);
}

QOpenGLFramebufferObject *GLRenderer::createFramebufferObject(const QSize &size) {
    QOpenGLFramebufferObjectFormat fmt;
    fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    requestRedraw(RedrawReason::TerrainChanged); // Premier dessin
    auto *fbo = new QOpenGLFramebufferObject(size, fmt);
    LOG_INFO() << "FBO créé: " << size.width() << "x" << size.height();
    return fbo;
}
