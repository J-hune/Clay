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

GLRenderer::GLRenderer() {
    initializeOpenGLFunctions();
    m_timer.start();
    m_terrainGpu.initialize(this);
}

void GLRenderer::synchronize(QQuickFramebufferObject *item) {
    auto *glItem = qobject_cast<GLViewport*>(item);
    if (!glItem) return;

    const QVector3D oldPos = glItem->m_cameraController.camera().position();
    if (!qFuzzyCompare(oldPos.x(), glItem->m_cameraController.camera().position().x()) ||
        !qFuzzyCompare(oldPos.y(), glItem->m_cameraController.camera().position().y()) ||
        !qFuzzyCompare(oldPos.z(), glItem->m_cameraController.camera().position().z())) {
        emit glItem->cameraPositionChanged();
    }

    // fps
    glItem->setFpsFromRenderer(m_fpsAccum);

    m_cameraController.camera().setPosition(glItem->m_cameraController.camera().position());
    m_cameraController.camera().setSpeed(glItem->m_cameraController.camera().speed());
    m_cameraController.camera().setYaw(glItem->m_cameraController.camera().yaw());
    m_cameraController.camera().setPitch(glItem->m_cameraController.camera().pitch());
    m_cameraController.camera().setOrbitPivot(glItem->m_cameraController.camera().orbitPivot());
    m_cameraController.camera().setOrbitDistance(glItem->m_cameraController.camera().orbitDistance());
    m_cameraController.camera().setMouseSensitivity(glItem->m_cameraController.camera().mouseSensitivity());
    m_cameraController.copyInputFrom(glItem->m_cameraController);

    m_grid.setResolution(glItem->gridResolution());
    m_drawGrid = glItem->drawGrid();
    m_drawAxes = glItem->drawAxes();

    // Gestion révision terrain
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
        m_needRedraw = true;
    }
}

void GLRenderer::render() {
    const qint64 ns = m_timer.nsecsElapsed();
    m_timer.restart();
    const double dtSec = qBound(0.0, static_cast<double>(ns) / 1e9, 0.1);
    const float dt = static_cast<float>(dtSec);

    if (dt > 0.f) {
        const float instantFps = 1.0f / dt;
        if (m_fpsAccum < 0.f) m_fpsAccum = instantFps; else m_fpsAccum = m_fpsAccum * 0.9f + instantFps * 0.1f;
    }

    m_cameraController.update(dt);

    glViewport(0, 0, framebufferObject()->width(), framebufferObject()->height());
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.129f, 0.141f, 0.161f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const int w = framebufferObject()->width();
    const int h = framebufferObject()->height();
    const float aspect = h > 0 ? static_cast<float>(w) / static_cast<float>(h) : 1.0f;

    QMatrix4x4 proj; proj.perspective(60.f, aspect, 0.1f, 1000.f);
    proj.scale(1.f, -1.f, 1.f);
    const QMatrix4x4 view = m_cameraController.camera().viewMatrix();

    glMatrixMode(GL_PROJECTION); glLoadMatrixf(proj.constData());
    glMatrixMode(GL_MODELVIEW); glLoadMatrixf(view.constData());

    m_grid.draw(this, m_drawGrid, m_drawAxes);

    // On ne dessine le terrain si prêt et si besoin
    const bool cameraDirty = m_cameraController.camera().isDirty();
    const bool shouldDrawTerrain = m_terrainReady && (m_needRedraw || cameraDirty || m_cameraController.hasActiveInput());
    if (shouldDrawTerrain) {
        m_terrainGpu.draw(this, proj, view);
        if (m_needRedraw) m_needRedraw = false; // clear the one-shot flag
        if (cameraDirty) m_cameraController.camera().clearDirty();
    }

    // On ne redemande un redraw que s'il y a une interaction utilisateur
    if (m_cameraController.hasActiveInput() || m_needRedraw) {
        update();
    }
}

QOpenGLFramebufferObject *GLRenderer::createFramebufferObject(const QSize &size) {
    QOpenGLFramebufferObjectFormat fmt;
    fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    return new QOpenGLFramebufferObject(size, fmt);
}
