#include "GLRenderer.h"
#include "GLViewport.h"
#include "qml/CameraControllerQml.h"
#include "qml/BrushManagerQml.h"
#include "qml/RaycastControllerQml.h"
#include "qml/TerrainManagerQml.h"
#include "TerrainGpu.h"
#include "CameraController.h"
#include "Grid.h"
#include "ImageUtils.h"
#include "Log.h"
#include <QOpenGLFramebufferObjectFormat>
#include <QDir>

GLRenderer::GLRenderer(GLViewport *viewport) : m_viewport(viewport) {
    initializeOpenGLFunctions();
    m_timer.start();
    m_terrainGpu.initialize(this);
    m_raycastController.initialize(this);
    m_brushManager.initialize();

    // Configure le dossier de stockage des brushes à <exe>/brushes
    const QString exeDir = QCoreApplication::applicationDirPath();
    const QString brushesDir = QDir(exeDir).filePath(QStringLiteral("brushes"));
    m_brushManager.setStorageDirectory(brushesDir);
    m_brushManager.loadFromDirectory(brushesDir);

    // Injection des instances partagées dans les wrappers QML
    if (auto* brushQml = m_viewport->brushManagerTyped()) {
        brushQml->setSharedManager(&m_brushManager);
        brushQml->refreshBrushModel();
    }

    if (auto* raycastQml = m_viewport->raycastControllerTyped()) {
        raycastQml->setSharedController(&m_raycastController);
    }

    if (auto* cameraQml = m_viewport->cameraControllerTyped()) {
        cameraQml->setSharedController(&m_cameraController);
    }

    m_brushOp.initialize(this);
    LOG_INFO() << "GLRenderer initialisé";
}

void GLRenderer::synchronize(QQuickFramebufferObject *item) {
    // On s'attend à recevoir le même viewport passé au constructeur
    auto *glItem = qobject_cast<GLViewport*>(item);
    if (!glItem) return;

    m_viewport = glItem;

    syncViewportState();
    syncTerrain();
    syncBrush();
    syncInteractionState();
}

void GLRenderer::syncViewportState() {
    if (!m_viewport) return;

    // FPS -> viewport
    m_viewport->setFpsFromRenderer(m_fpsAccum);

    // Flags affichage
    m_drawGrid = m_viewport->drawGrid();
    m_drawAxes = m_viewport->drawAxes();

    // Upload des brushes en attente (dans le thread de rendu)
    const int prevPending = m_brushManager.pendingUploadsCount();
    m_brushManager.uploadPendingBrushes();
    const int newPending = m_brushManager.pendingUploadsCount();

    // Notifier l'UI si la progression a changé
    if (prevPending != newPending) {
        if (auto* brushQml = m_viewport->brushManagerTyped()) {
            emit brushQml->loadingProgressChanged();
        }
    }

    // Détection des changements
    const int currentGridRes = m_viewport->grid().resolution();
    if (m_prevGridResolution != currentGridRes ||
        m_prevDrawGrid != m_drawGrid ||
        m_prevDrawAxes != m_drawAxes) {
        requestRedraw(RedrawReason::GridChanged);
        m_prevGridResolution = currentGridRes;
        m_prevDrawGrid = m_drawGrid;
        m_prevDrawAxes = m_drawAxes;
    }
}

void GLRenderer::syncTerrain() {
    if (!m_viewport) return;

    auto* terrainQml = m_viewport->terrainManagerTyped();
    if (!terrainQml) return;

    // Vérifier si le terrain a besoin d'être uploadé
    if (terrainQml->needsUpload() && terrainQml->revision() != m_lastTerrainRevision) {
        m_lastTerrainRevision = terrainQml->revision();

        // Configuration du terrain GPU
        m_terrainGpu.setGridResolution(terrainQml->resolution(), terrainQml->resolution());
        m_terrainGpu.setTextureResolution(terrainQml->heightmapResolution());

        // Reconstruction selon le mode
        if (terrainQml->mode() == 1 && !terrainQml->heightmapSource().isEmpty()) {
            const QImage img = loadHeightImage(terrainQml->heightmapSource());
            if (!img.isNull()) {
                m_terrainGpu.rebuild(this, img, terrainQml->heightScale());
                LOG_INFO() << "Terrain reconstruit (heightmap) - res=" << terrainQml->resolution()
                           << ", texRes=" << terrainQml->heightmapResolution()
                           << ", scale=" << terrainQml->heightScale();
            } else {
                m_terrainGpu.rebuildFlat(this, terrainQml->heightScale());
                LOG_WARN() << "Impossible de charger la heightmap, terrain plat utilisé";
            }
        } else {
            m_terrainGpu.rebuildFlat(this, terrainQml->heightScale());
            LOG_INFO() << "Terrain plat reconstruit - res=" << terrainQml->resolution()
                       << ", texRes=" << terrainQml->heightmapResolution()
                       << ", scale=" << terrainQml->heightScale();
        }

        terrainQml->setNeedsUpload(false);
        m_terrainReady = true;
        requestRedraw(RedrawReason::TerrainChanged);
    }
}

void GLRenderer::syncBrush() {
    if (m_brushManager.isDirty()) {
        // Mise à jour de la prévisualisation du brush
        m_terrainGpu.setBrushPreview(
            m_brushManager.currentBrushIndex(),
            m_brushManager.brushSize(),
            m_brushManager.brushStrength()
        );

        requestRedraw(RedrawReason::BrushChanged);
        m_brushManager.clearDirty();
    }
}

void GLRenderer::syncInteractionState() {
    if (!m_viewport) return;

    auto* raycastQml = m_viewport->raycastControllerTyped();
    if (raycastQml) {
        const QVector2D currentNDC = raycastQml->mouseNDC();
        // On vérifie si la position de la souris a changé
        if ((currentNDC - m_lastMouseNDC).lengthSquared() > 1e-6f) {
            m_mouseMoved = true;
            m_lastMouseNDC = currentNDC;
        }
    }
}

void GLRenderer::render() {
    const float dt = computeDeltaTime();
    updateFPS(dt);

    bool cameraDirty = false;
    updateCamera(dt, cameraDirty);

    if (!shouldRenderFrame(cameraDirty)) {
        return;
    }

    applyPendingStrokes();
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
    m_cameraController.update(dt);
    cameraDirty = m_cameraController.camera().isDirty();

    if (cameraDirty) {
        m_cameraController.camera().clearDirty();
    }
}

bool GLRenderer::shouldRenderFrame(bool cameraDirty) const {
    const bool hasRedrawReason = (m_redrawReasons != RedrawReason::None);
    const bool hasPendingStrokes = !m_brushManager.pendingStrokes().empty();
    return hasRedrawReason || cameraDirty || m_mouseMoved || hasPendingStrokes;
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
    updateViewMatrix(m_cameraController.camera(), view);
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

    m_viewport->grid().draw(this, m_drawGrid, m_drawAxes);

    if (m_terrainReady) {
        m_terrainGpu.setBrushTextureArray(m_brushManager.brushTextureArrayId());
        m_terrainGpu.draw(this, proj, view, m_cameraController.camera().frontVector());
    }
}

void GLRenderer::processRaycast(const QMatrix4x4 &proj, const QMatrix4x4 &view) {
    if (!m_terrainReady || !m_viewport) return;

    auto* raycastQml = m_viewport->raycastControllerTyped();
    auto* terrainQml = m_viewport->terrainManagerTyped();

    if (!raycastQml || !terrainQml) return;

    // Si la caméra bouge, on réinitialise le raycast
    if (m_cameraController.isMovingCamera()) {
        m_terrainGpu.clearRaycastHit();
        m_raycastController.reset();
        raycastQml->reset();
        return;
    }

    // Sinon, on effectue le raycast
    if (m_mouseMoved) {
        m_raycastController.updateMousePosition(raycastQml->mouseNDC());
        m_raycastController.perform(
            this,
            proj,
            view,
            m_terrainGpu.heightmapTexture(),
            terrainQml->heightmapResolution(),
            terrainQml->heightScale()
        );

        const bool hasHit = m_raycastController.hasHit();
        const QVector3D hitPos = m_raycastController.hitPosition();

        if (hasHit) {
            m_terrainGpu.setRaycastHit(hitPos);
            raycastQml->notifyRaycastComplete(hasHit, hitPos);
            requestRedraw(RedrawReason::RaycastChanged);
        } else {
            m_terrainGpu.clearRaycastHit();
            raycastQml->notifyRaycastComplete(hasHit, QVector3D());
        }
    }
}

void GLRenderer::applyPendingStrokes() {
    if (!m_terrainReady) return;

    const auto &strokes = m_brushManager.pendingStrokes();
    if (strokes.empty()) return;

    auto* terrainQml = m_viewport->terrainManagerTyped();
    if (!terrainQml) return;

    for (const auto &stroke : strokes) {
        if (!m_brushManager.isValidBrushIndex(stroke.brushIndex)) continue;

        const auto opType = static_cast<BrushOpType>(stroke.operation);

        m_brushOp.applyBrush(
            this,
            opType,
            m_terrainGpu.heightmapTexture(),
            terrainQml->heightmapResolution(),
            stroke.worldPos,
            stroke.size,
            stroke.strength,
            m_brushManager.brushTextureArrayId(),
            stroke.brushIndex,
            -50.0f, 50.0f,
            -50.0f, 50.0f,
            terrainQml->heightScale(),
            m_cameraController.camera().frontVector()
        );
    }

    m_brushManager.clearPendingStrokes();
    requestRedraw(RedrawReason::TerrainChanged);
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
