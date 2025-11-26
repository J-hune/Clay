#include "GLViewport.h"
#include "GLRenderer.h"
#include "Grid.h"
#include "CameraController.h"
#include "TerrainMesh.h"
#include <QOpenGLFramebufferObjectFormat>
#include <QObject>
#include <QtGlobal>
#include <QQuickWindow>
#include <cmath>
#include "ImageUtils.h"
#include <QCursor>

GLViewport::GLViewport(QQuickItem *parent) : QQuickFramebufferObject(parent) {
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setFlag(ItemIsFocusScope, true);
    setFlag(ItemAcceptsInputMethod, true);
    setFocus(true);
}

void GLViewport::setFpsFromRenderer(float fps) {
    if (!qFuzzyCompare(m_fps, fps)) { m_fps = fps; emit fpsChanged(); }
}

void GLViewport::setGridResolution(int r) {
    const int old = m_grid.resolution();
    m_grid.setResolution(r);
    if (m_grid.resolution() != old) {
        emit gridResolutionChanged();
        update();
    }
}

void GLViewport::setCameraSpeed(const float s) {
    const float old = m_cameraController.speed();
    m_cameraController.setSpeed(s);
    if (!qFuzzyCompare(old, m_cameraController.speed())) {
        emit cameraSpeedChanged();
    }
}
void GLViewport::setMouseSensitivity(float s) {
    if (s < 0.f) s = 0.f;
    if (qFuzzyCompare(m_cameraController.mouseSensitivity(), s)) return;
    m_cameraController.setMouseSensitivity(s);
    emit mouseSensitivityChanged();
}
void GLViewport::setDrawGrid(const bool v) {
    if (m_drawGrid == v) return;
    m_drawGrid = v;
    emit drawGridChanged();
}
void GLViewport::setDrawAxes(const bool v) {
    if (m_drawAxes == v) return;
    m_drawAxes = v;
    emit drawAxesChanged();
}
void GLViewport::setOrbitDistance(const float d) {
    const float old = m_cameraController.orbitDistance();
    m_cameraController.setOrbitDistance(d);
    if (!qFuzzyCompare(old, m_cameraController.orbitDistance())) emit orbitDistanceChanged();
}


void GLViewport::setTerrainResolution(int r) {
    if (r < 2) r = 2;
    if (r == m_terrainResolution) return;
    m_terrainResolution = r;
    emit terrainResolutionChanged();
}
void GLViewport::setHeightmapResolution(int r) {
    constexpr int allowed[4] = {512, 1024, 2048, 4096};
    int chosen = 512;
    for (const int v : allowed) if (r == v) { chosen = v; break; }
    if (chosen == m_heightmapResolution) return;
    m_heightmapResolution = chosen;
    ++m_terrainRevision; // rebuild GPU texture
    emit heightmapResolutionChanged();
    update();
}

void GLViewport::setHeightmapSource(const QUrl &url) {
    if (url == m_heightmapSource) return;
    m_heightmapSource = url;
    emit heightmapSourceChanged();
}
void GLViewport::setHeightScale(float s) {
    if (s < 0.f) s = 0.f;
    if (qFuzzyCompare(s, m_heightScale)) return;
    m_heightScale = s;
    emit heightScaleChanged();
}
void GLViewport::setTerrainMode(int m) {
    if (m == m_terrainMode) return;
    if (m < 0 || m > 1) return; // limitation actuelle
    m_terrainMode = m;
    emit terrainModeChanged();
}

void GLViewport::generateTerrain() {
    m_userRequestedTerrain = true;
    m_terrainReady = false;
    const int res = m_terrainResolution;
    if (m_terrainMode == 0) { // flat
        m_terrainMesh.buildFlat(res, res);
    } else if (m_terrainMode == 1) { // heightmap
        const QImage img = loadHeightImage(m_heightmapSource);
        if (!img.isNull()) m_terrainMesh.buildHeightmap(img, res, res, m_heightScale); else m_terrainMesh.buildFlat(res, res);
    }
    m_terrainReady = m_terrainMesh.isValid();
    ++m_terrainRevision;
    emit terrainReadyChanged();
    update();
}

void GLViewport::keyPressEvent(QKeyEvent *event) {
    m_cameraController.handleKeyPress(event);
    update();
}

void GLViewport::keyReleaseEvent(QKeyEvent *event) {
    m_cameraController.handleKeyRelease(event);
    update();
}

void GLViewport::mousePressEvent(QMouseEvent *event) {
    m_cameraController.handleMousePress(event, window());
    forceActiveFocus();
    update();
}

void GLViewport::mouseMoveEvent(QMouseEvent *event) {
    const QQuickWindow *w = window();
    const QPoint itemPosInWindow = mapToScene(QPointF()).toPoint();
    const QPoint localPos = w->mapFromGlobal(QCursor::pos());
    const QRect rect(itemPosInWindow, QSize(static_cast<int>(width()), static_cast<int>(height())));

    const float oldYaw = m_cameraController.camera().yaw();
    const float oldPitch = m_cameraController.camera().pitch();
    m_cameraController.handleMouseMove(event, w, localPos, rect);
    if (!qFuzzyCompare(oldYaw, m_cameraController.camera().yaw())) emit yawChanged();
    if (!qFuzzyCompare(oldPitch, m_cameraController.camera().pitch())) emit pitchChanged();

    // On calcule la position de la souris en NDC pour le raycast
    const QPointF itemPos = mapFromScene(localPos);
    const float w_width = static_cast<float>(width());
    const float w_height = static_cast<float>(height());

    if (w_width > 0.f && w_height > 0.f) {
        m_mouseNDC = QVector2D(
            (2.0f * itemPos.x() / w_width) - 1.0f,
            (2.0f * itemPos.y() / w_height) - 1.0f
        );
        m_raycastRequested = true;
        update();
    } else {
        qWarning("GLViewport::mouseMoveEvent : dimensions invalides pour NDC");
    }
}

void GLViewport::mouseReleaseEvent(QMouseEvent *event) {
    m_cameraController.handleMouseRelease(event, window());
    update();
}

void GLViewport::wheelEvent(QWheelEvent *event) {
    const float oldDist = m_cameraController.orbitDistance();
    const float oldSpeed = m_cameraController.speed();
    m_cameraController.handleWheel(event);
    const bool distChanged = !qFuzzyCompare(oldDist, m_cameraController.orbitDistance());
    const bool speedChanged = !qFuzzyCompare(oldSpeed, m_cameraController.speed());
    if (speedChanged) emit cameraSpeedChanged();
    if (distChanged) emit orbitDistanceChanged();
    if (speedChanged || distChanged) update();
}

// Event de déplacement de la souris (sans bouton appuyé)
void GLViewport::hoverMoveEvent(QHoverEvent *event) {
    const float w_width = static_cast<float>(width());
    const float w_height = static_cast<float>(height());

    if (w_width > 0.f && w_height > 0.f) {
        // Utiliser la position locale de l'événement hover
        const QPointF localPosF = event->position();
        const QVector2D newNDC(
            (2.0f * localPosF.x() / w_width) - 1.0f,
            (2.0f * localPosF.y() / w_height) - 1.0f
        );
        if ((newNDC - m_mouseNDC).lengthSquared() > 1e-6f) {
            m_mouseNDC = newNDC;
            m_raycastRequested = true;
            update();
        }
    } else {
        qWarning("GLViewport::hoverMoveEvent : dimensions invalides pour NDC");
    }
}

// Renderer OpenGL
QQuickFramebufferObject::Renderer *GLViewport::createRenderer() const {
    return new GLRenderer(const_cast<GLViewport*>(this));
}
