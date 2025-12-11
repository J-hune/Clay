#include "GLRenderer.h"
#include "GLViewport.h"
#include "qml/CameraControllerQml.h"
#include "qml/BrushManagerQml.h"
#include "qml/RaycastControllerQml.h"
#include "qml/TerrainManagerQml.h"
#include "qml/ErosionControllerQml.h"
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
    m_erosionBrushOp.initialize(this);

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

    if (auto* erosionQml = m_viewport->erosionControllerTyped()) {
        // Connecte le signal d'érosion pour marquer qu'une érosion doit être appliquée
        QObject::connect(erosionQml, &ErosionControllerQml::erosionRequested,
                         m_viewport, [this]() {
            m_viewport->m_pendingErosion = true;
            m_viewport->update();
        }, Qt::QueuedConnection);
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
    syncErosion();
}

void GLRenderer::syncExportRequest() {
    if (!m_viewport->m_pendingExportPath.isEmpty()) {
        exportHeightmap(m_viewport->m_pendingExportPath);
        m_viewport->m_pendingExportPath.clear();
    }
}

void GLRenderer::syncErosion() {
    if (!m_viewport) return;

    auto* erosionQml = m_viewport->erosionControllerTyped();
    if (!erosionQml) return;

    // Synchronise les paramètres d'érosion du thread GUI vers le thread de rendu
    m_erosion.setIterations(erosionQml->iterations());
    m_erosion.setNumParticles(erosionQml->numParticles());
    m_erosion.setInertia(erosionQml->inertia());
    m_erosion.setSedimentCapacity(erosionQml->sedimentCapacity());
    m_erosion.setDepositionPercentage(erosionQml->depositionPercentage());
    m_erosion.setErosionSpeed(erosionQml->erosionSpeed());
    m_erosion.setEvaporationSpeed(erosionQml->evaporationSpeed());
    m_erosion.setGravity(erosionQml->gravity());
    m_erosion.setMinSlope(erosionQml->minSlope());
    m_erosion.setMaxLifetime(erosionQml->maxLifetime());

    // Applique l'érosion si une requête est en attente
    if (m_viewport->m_pendingErosion) {
        applyErosion();
        m_viewport->m_pendingErosion = false;
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

    // On s'assure que le masque d'érosion existe et correspond à la résolution de la heightmap
    ensureErosionMaskTexture(terrainQml->heightmapResolution());
    // On fournit le masque au système d'érosion
    m_erosion.setErosionMaskTexture(m_erosionMaskTexture);

    // Invalide les buffers d'érosion pour forcer leur recréation
    m_erosion.invalidateBuffers();

    terrainQml->setNeedsUpload(false);
    m_state.terrainReady = true;
    m_state.requestRedraw(RedrawReason::TerrainChanged);
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

    // Mettre à jour l'overlay du masque d'érosion
    m_terrainGpu.setErosionMaskTexture(m_erosionMaskTexture);
    m_terrainGpu.setShowMaskOverlay(m_viewport ? m_viewport->m_erosionUiActive : false);

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

void GLRenderer::applyMaskAtPosition(const QVector3D &worldPos, int brushIndex,
                             float size, float strength, bool erase) {
    if (!m_state.terrainReady || m_erosionMaskTexture == 0) return;

    auto* terrainQml = m_viewport->terrainManagerTyped();
    if (!terrainQml) return;

    // Assure que la texture du masque existe et a la bonne taille
    ensureErosionMaskTexture(terrainQml->heightmapResolution());

    m_erosionBrushOp.applyMask(
        this,
        m_erosionMaskTexture,
        m_erosionMaskSize,
        worldPos,
        size,
        strength,
        m_brushManager.brushTextureArrayId(),
        brushIndex,
        TERRAIN_MIN_X, TERRAIN_MAX_X,
        TERRAIN_MIN_Z, TERRAIN_MAX_Z,
        m_cameraController.camera().frontVector(),
        erase
    );

    // Redessiner (overlay + preview)
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

    const QVector3D hitPos = m_raycastController.hitPosition();
    const int brushIdx = m_brushManager.currentBrushIndex();
    const float size = m_brushManager.brushSize();
    const float strength = m_brushManager.brushStrength();
    const BrushOpType operation = m_brushManager.operation();

    // Si l'opération est ErosionMaskAdd ou ErosionMaskErase, on applique le masque
    if (operation == BrushOpType::ErosionMaskAdd || operation == BrushOpType::ErosionMaskErase) {
        const bool erase = (operation == BrushOpType::ErosionMaskErase);
        applyMaskAtPosition(hitPos, brushIdx, size, strength, erase);
    } else {
        // Sinon, on applique le brush normalement (Raise, Lower, Smooth)
        applyBrushAtPosition(hitPos, brushIdx, size, strength, operation);
    }
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

void GLRenderer::applyErosion() {
    if (!m_terrainGpu.heightmapTexture()) {
        LOG_ERROR() << "Impossible d'appliquer l'érosion : heightmap non initialisée";
        return;
    }

    auto* terrainQml = m_viewport->terrainManagerTyped();
    if (!terrainQml) {
        LOG_ERROR() << "TerrainManager non disponible";
        return;
    }

    LOG_INFO() << "Application de l'érosion sur le terrain...";
    // On s'assure que le masque est bien à jour côté système d'érosion
    m_erosion.setErosionMaskTexture(m_erosionMaskTexture);

    // Applique l'érosion en 2 passes sur la heightmap GPU
    m_erosion.dispatch(
        this,
        m_terrainGpu.heightmapTexture(),
        terrainQml->heightmapResolution()
    );

    // Marque que le terrain a changé pour forcer un redraw
    m_state.requestRedraw(RedrawReason::TerrainChanged);

    LOG_INFO() << "Érosion appliquée avec succès";
}

void GLRenderer::ensureErosionMaskTexture(int size) {
    if (size <= 0) return;

    if (m_erosionMaskTexture == 0) {
        glGenTextures(1, &m_erosionMaskTexture);
    }

    if (m_erosionMaskSize != size) {
        m_erosionMaskSize = size;
        glBindTexture(GL_TEXTURE_2D, m_erosionMaskTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // Alloue/resize la texture en GL_R8
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, size, size, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        // Clear à 0 pour commencer avec un masque vide
        std::vector<uint8_t> zeroData(size * size, 0);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size, size, GL_RED, GL_UNSIGNED_BYTE, zeroData.data());

        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void GLRenderer::clearErosionMask() {
    if (m_erosionMaskTexture == 0 || m_erosionMaskSize <= 0) return;
    // Met tout à 0
    glBindTexture(GL_TEXTURE_2D, m_erosionMaskTexture);
    std::vector<uint8_t> zeroData(m_erosionMaskSize * m_erosionMaskSize, 0);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_erosionMaskSize, m_erosionMaskSize, GL_RED, GL_UNSIGNED_BYTE, zeroData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}
