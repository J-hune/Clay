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
#include "SceneRenderer.h"
#include <QOpenGLFramebufferObjectFormat>
#include <QDir>

GLRenderer::GLRenderer(GLViewport *viewport) : m_viewport(viewport) {
    initializeOpenGLFunctions();
    m_timer.start();

    m_terrainGpu.initialize(this);
    m_raycastController.initialize(this);
    m_brushOp.initialize(this);

    initializeBrushManager();
    linkQmlControllers();

    LOG_INFO() << "GLRenderer initialisé";
}

void GLRenderer::initializeBrushManager() {
    m_brushManager.initialize();

    const QString exeDir = QCoreApplication::applicationDirPath();
    const QString brushesDir = QDir(exeDir).filePath(QStringLiteral("brushes"));

    m_brushManager.setStorageDirectory(brushesDir);
    m_brushManager.loadFromDirectory(brushesDir);
}

void GLRenderer::linkQmlControllers() {
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
}

void GLRenderer::synchronize(QQuickFramebufferObject *item) {
    auto *glItem = qobject_cast<GLViewport*>(item);
    if (!glItem) return;

    m_viewport = glItem;

    syncViewportState();
    syncTerrain();
    syncBrushManager();
    syncMousePosition();
    syncExportRequest();
}

void GLRenderer::syncExportRequest() {
    if (!m_viewport->m_pendingExportPath.isEmpty()) {
        exportHeightmap(m_viewport->m_pendingExportPath);
        m_viewport->m_pendingExportPath.clear();
    }

    if (m_viewport->m_pendingUndo) {
        performUndo();
        m_viewport->m_pendingUndo = false;
    }

    if (m_viewport->m_pendingRedo) {
        performRedo();
        m_viewport->m_pendingRedo = false;
    }
}

void GLRenderer::updateBrushAsyncLoading() {
    m_brushManager.uploadPendingBrushes();

    if (m_brushManager.pendingUploadsCount() > 0) {
        m_viewport->update();
    }

    if (auto* brushQml = m_viewport->brushManagerTyped()) {
        brushQml->notifyLoadingProgress();
    }
}

void GLRenderer::syncViewportState() {
    if (!m_viewport) return;

    m_viewport->setFpsFromRenderer(m_state.fpsAccum);

    m_state.drawGrid = m_viewport->drawGrid();
    m_state.drawAxes = m_viewport->drawAxes();

    // Détecte les changements de résolution de grille ou de flags d'affichage
    const int currentGridRes = m_viewport->grid().resolution();
    if (m_state.prevGridResolution != currentGridRes ||
        m_state.prevDrawGrid != m_state.drawGrid ||
        m_state.prevDrawAxes != m_state.drawAxes) {
        m_state.requestRedraw(RedrawReason::GridChanged);
        m_state.prevGridResolution = currentGridRes;
        m_state.prevDrawGrid = m_state.drawGrid;
        m_state.prevDrawAxes = m_state.drawAxes;
    }
}

void GLRenderer::syncBrushManager() {
    if (!m_viewport) return;

    // Met à jour l'aperçu du brush si nécessaire
    if (m_brushManager.isDirty()) {
        m_terrainGpu.setBrushPreview(
            m_brushManager.currentBrushIndex(),
            m_brushManager.brushSize(),
            m_brushManager.brushStrength()
        );
        m_state.requestRedraw(RedrawReason::BrushChanged);
        m_brushManager.clearDirty();
    }

    // Gère le chargement asynchrone des brushes
    updateBrushAsyncLoading();
}

void GLRenderer::syncMousePosition() {
    if (!m_viewport) return;

    if (auto* raycastQml = m_viewport->raycastControllerTyped()) {
        const QVector2D currentNDC = raycastQml->mouseNDC();
        if ((currentNDC - m_state.lastMouseNDC).lengthSquared() > 1e-6f) {
            m_state.mouseMoved = true;
            m_state.lastMouseNDC = currentNDC;
        }
    }
}

void GLRenderer::syncTerrain() {
    if (!m_viewport) return;

    auto* terrainQml = m_viewport->terrainManagerTyped();
    if (!terrainQml) return;

    // Vérifie si le terrain nécessite un upload (changement de configuration)
    if (!terrainQml->needsUpload() || terrainQml->revision() == m_state.lastTerrainRevision) {
        return;
    }

    m_state.lastTerrainRevision = terrainQml->revision();

    // Configure la résolution du terrain GPU
    m_terrainGpu.setGridResolution(terrainQml->resolution(), terrainQml->resolution());
    m_terrainGpu.setTextureResolution(terrainQml->heightmapResolution());

    // Reconstruit le terrain selon le mode (heightmap ou plat)
    const bool useHeightmap = (terrainQml->mode() == 1) && !terrainQml->heightmapSource().isEmpty();

    if (useHeightmap) {
        const QImage img = loadHeightImage(terrainQml->heightmapSource());
        if (!img.isNull()) {
            m_terrainGpu.rebuild(this, img, terrainQml->heightScale());
            LOG_INFO() << "Terrain reconstruit depuis heightmap - res=" << terrainQml->resolution()
                       << ", texRes=" << terrainQml->heightmapResolution()
                       << ", scale=" << terrainQml->heightScale();
        } else {
            m_terrainGpu.rebuildFlat(this, terrainQml->heightScale());
            LOG_WARN() << "Échec du chargement de la heightmap, utilisation d'un terrain plat";
        }
    } else {
        m_terrainGpu.rebuildFlat(this, terrainQml->heightScale());
        LOG_INFO() << "Terrain plat reconstruit - res=" << terrainQml->resolution()
                   << ", texRes=" << terrainQml->heightmapResolution()
                   << ", scale=" << terrainQml->heightScale();
    }

    terrainQml->setNeedsUpload(false);
    m_state.terrainReady = true;
    m_state.requestRedraw(RedrawReason::TerrainChanged);

    // Capture le snapshot initial pour l'historique undo/redo
    m_undoRedoManager.clear();  // Efface l'ancien historique
    m_undoRedoManager.captureSnapshot(this, m_terrainGpu.heightmapTexture(), terrainQml->heightmapResolution());
    LOG_INFO() << "Snapshot initial capturé pour undo/redo";
}

void GLRenderer::render() {
    const float dt = computeDeltaTime();
    updateFPS(dt);

    bool cameraDirty = false;
    updateCamera(dt, cameraDirty);

    // Détecte si le brush vient d'être relâché pour terminer le batch
    const bool brushCurrentlyActive = canApplyContinuousBrush();
    if (m_state.wasBrushActive && !brushCurrentlyActive) {
        // Le brush a été relâché, termine le batch et capture le snapshot final
        if (m_undoRedoManager.isBatching()) {
            m_undoRedoManager.endBatch();

            // Capture le snapshot après toutes les modifications
            auto* terrainQml = m_viewport->terrainManagerTyped();
            if (terrainQml) {
                m_undoRedoManager.captureSnapshot(this, m_terrainGpu.heightmapTexture(), terrainQml->heightmapResolution());
            }
        }
    }
    m_state.wasBrushActive = brushCurrentlyActive;

    if (!shouldRenderFrame(cameraDirty)) {
        return;
    }

    applyPendingStrokes();
    applyContinuousBrush();
    drawFrame();

    // Demande un nouveau rendu si le brush est actif (clic maintenu)
    // Utilise QMetaObject::invokeMethod pour appeler update() dans le thread GUI
    if (canApplyContinuousBrush()) {
        QMetaObject::invokeMethod(m_viewport, "update", Qt::QueuedConnection);
    }

    // Nettoie l'état pour la prochaine frame
    m_state.clearRedrawReasons();
    m_state.mouseMoved = false;
}

void GLRenderer::drawFrame() {
    const int w = framebufferObject()->width();
    const int h = framebufferObject()->height();

    // Prépare l'état OpenGL et nettoie le framebuffer
    SceneRenderer::setupRenderState(this, w, h);
    SceneRenderer::clearFramebuffer(this);

    // Calcule les matrices de projection et de vue
    QMatrix4x4 proj, view;
    SceneRenderer::computeMatrices(w, h, m_cameraController, proj, view);

    // Dessine la scène complète (terrain, grille, axes)
    SceneRenderer::drawScene(
        this,
        m_viewport->grid(),
        m_terrainGpu,
        m_brushManager.brushTextureArrayId(),
        m_state.drawGrid,
        m_state.drawAxes,
        m_state.terrainReady,
        proj,
        view,
        m_cameraController.camera().frontVector()
    );

    // Effectue le raycast pour déterminer la position du brush
    processRaycast(proj, view);
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
        if (m_state.fpsAccum < 0.f) {
            m_state.fpsAccum = instantFps;
        } else {
            m_state.fpsAccum = m_state.fpsAccum * 0.9f + instantFps * 0.1f;
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
    // Vérifie toutes les conditions qui nécessitent un nouveau rendu
    if (m_state.shouldRedraw()) return true;
    if (cameraDirty) return true;
    if (m_state.mouseMoved) return true;
    if (!m_brushManager.pendingStrokes().empty()) return true;
    if (m_brushManager.pendingUploadsCount() > 0) return true;

    // Vérifie si le brush est en cours d'application continue
    const bool isApplyingBrush = m_cameraController.isLeftButtonPressed()
                               && !m_cameraController.isMovingCamera()
                               && m_raycastController.hasHit();

    return isApplyingBrush;
}

void GLRenderer::processRaycast(const QMatrix4x4 &proj, const QMatrix4x4 &view) {
    if (!m_state.terrainReady || !m_viewport) return;

    auto* raycastQml = m_viewport->raycastControllerTyped();
    auto* terrainQml = m_viewport->terrainManagerTyped();
    if (!raycastQml || !terrainQml) return;

    // Réinitialise le raycast si la caméra est en mouvement
    if (m_cameraController.isMovingCamera()) {
        m_terrainGpu.clearRaycastHit();
        m_raycastController.reset();
        raycastQml->reset();
        return;
    }

    // Effectue le raycast uniquement si la souris a bougé
    // On pourrait faire une version ou on teste si le clic est maintenu aussi mais je trouve ça moins bien
    // const bool needsRaycast = m_state.mouseMoved || m_cameraController.isLeftButtonPressed();
    if (!m_state.mouseMoved) return;

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
        raycastQml->notifyRaycastComplete(true, hitPos);
        m_state.requestRedraw(RedrawReason::RaycastChanged);
    } else {
        m_terrainGpu.clearRaycastHit();
        raycastQml->notifyRaycastComplete(false, QVector3D());
    }
}

void GLRenderer::applyBrushAtPosition(const QVector3D &worldPos, int brushIndex,
                                      float size, float strength, BrushOpType operation) {
    auto* terrainQml = m_viewport->terrainManagerTyped();
    if (!terrainQml || !m_brushManager.isValidBrushIndex(brushIndex)) return;


    m_brushOp.applyBrush(
        this,
        operation,
        m_terrainGpu.heightmapTexture(),
        terrainQml->heightmapResolution(),
        worldPos,
        size,
        strength,
        m_brushManager.brushTextureArrayId(),
        brushIndex,
        TERRAIN_MIN_X, TERRAIN_MAX_X,
        TERRAIN_MIN_Z, TERRAIN_MAX_Z,
        terrainQml->heightScale(),
        m_cameraController.camera().frontVector()
    );

    m_state.requestRedraw(RedrawReason::TerrainChanged);
}

void GLRenderer::applyPendingStrokes() {
    if (!m_state.terrainReady) return;

    const auto &strokes = m_brushManager.pendingStrokes();
    if (strokes.empty()) return;

    for (const auto &stroke : strokes) {
        applyBrushAtPosition(
            stroke.worldPos,
            stroke.brushIndex,
            stroke.size,
            stroke.strength,
            stroke.operation
        );
    }

    // Capture le snapshot après l'application de tous les strokes
    auto* terrainQml = m_viewport->terrainManagerTyped();
    if (terrainQml) {
        m_undoRedoManager.captureSnapshot(this, m_terrainGpu.heightmapTexture(), terrainQml->heightmapResolution());
    }

    m_brushManager.clearPendingStrokes();
}

bool GLRenderer::canApplyContinuousBrush() const {
    return m_cameraController.isLeftButtonPressed()
        && !m_cameraController.isMovingCamera()
        && m_raycastController.hasHit();
}

void GLRenderer::applyContinuousBrush() {
    if (!m_state.terrainReady || !m_viewport) return;
    if (!canApplyContinuousBrush()) return;

    // Active le mode batch si ce n'est pas déjà fait
    if (!m_undoRedoManager.isBatching()) {
        m_undoRedoManager.beginBatch();
    }

    applyBrushAtPosition(
        m_raycastController.hitPosition(),
        m_brushManager.currentBrushIndex(),
        m_brushManager.brushSize(),
        m_brushManager.brushStrength(),
        m_brushManager.operation()
    );
}

QOpenGLFramebufferObject *GLRenderer::createFramebufferObject(const QSize &size) {
    QOpenGLFramebufferObjectFormat fmt;
    fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);

    // Demande un premier rendu de la scène
    m_state.requestRedraw(RedrawReason::TerrainChanged);

    auto *fbo = new QOpenGLFramebufferObject(size, fmt);
    LOG_INFO() << "FBO créé: " << size.width() << "x" << size.height();
    return fbo;
}

bool GLRenderer::exportHeightmap(const QString &filePath) {
    if (!m_state.terrainReady) {
        LOG_WARN() << "Export impossible: terrain non initialisé";
        return false;
    }

    LOG_INFO() << "Début de l'export de la heightmap vers: " << filePath.toStdString();

    // Récupère la heightmap depuis le GPU au format 16 bits
    const QImage heightmap = m_terrainGpu.exportHeightmap16(this);
    if (heightmap.isNull()) {
        LOG_ERROR() << "Échec de la récupération de la heightmap depuis le GPU";
        return false;
    }

    // Convertit le chemin QML (file://...) en chemin local si nécessaire
    QString localPath = filePath;
    if (localPath.startsWith("file://")) {
        localPath = localPath.mid(7);
    }

    // Sauvegarde l'image en PNG
    const bool success = heightmap.save(localPath, "PNG");
    if (success) {
        LOG_INFO() << "Heightmap exportée avec succès: " << localPath.toStdString();
    } else {
        LOG_ERROR() << "Échec de la sauvegarde de l'image: " << localPath.toStdString();
    }

    return success;
}

void GLRenderer::performUndo() {
    if (!m_state.terrainReady) {
        LOG_WARN() << "Undo impossible: terrain non initialisé";
        return;
    }

    if (m_undoRedoManager.undo(this, m_terrainGpu.heightmapTexture())) {
        m_state.requestRedraw(RedrawReason::TerrainChanged);
        // Force une mise à jour du viewport
        if (m_viewport) {
            QMetaObject::invokeMethod(m_viewport, "update", Qt::QueuedConnection);
        }
        LOG_DEBUG() << "Undo effectué - Nouvel index: " << m_undoRedoManager.currentIndex();
    }
}

void GLRenderer::performRedo() {
    if (!m_state.terrainReady) {
        LOG_WARN() << "Redo impossible: terrain non initialisé";
        return;
    }

    if (m_undoRedoManager.redo(this, m_terrainGpu.heightmapTexture())) {
        m_state.requestRedraw(RedrawReason::TerrainChanged);
        // Force une mise à jour du viewport
        if (m_viewport) {
            QMetaObject::invokeMethod(m_viewport, "update", Qt::QueuedConnection);
        }
        LOG_DEBUG() << "Redo effectué - Nouvel index: " << m_undoRedoManager.currentIndex();
    }
}
