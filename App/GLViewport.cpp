#include "GLViewport.h"
#include "GLRenderer.h"
#include "qml/CameraControllerQml.h"
#include "qml/BrushManagerQml.h"
#include "qml/RaycastControllerQml.h"
#include "qml/TerrainManagerQml.h"
#include <QCursor>

GLViewport::GLViewport(QQuickItem *parent) : QQuickFramebufferObject(parent) {
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setFlag(ItemIsFocusScope, true);
    setFlag(ItemAcceptsInputMethod, true);
    setFocus(true);
}

void GLViewport::setFpsFromRenderer(float fps) {
    if (!qFuzzyCompare(m_fps, fps)) {
        m_fps = fps;
        emit fpsChanged();
    }
}

void GLViewport::setGridResolution(int r) {
    const int old = m_grid.resolution();
    m_grid.setResolution(r);
    if (m_grid.resolution() != old) {
        emit gridResolutionChanged();
        update();
    }
}

void GLViewport::setDrawGrid(const bool v) {
    if (m_drawGrid == v) return;
    m_drawGrid = v;
    emit drawGridChanged();
    update();
}

void GLViewport::setDrawAxes(const bool v) {
    if (m_drawAxes == v) return;
    m_drawAxes = v;
    emit drawAxesChanged();
    update();
}

void GLViewport::setCameraController(QObject *controller) {
    if (m_cameraController == controller) return;
    m_cameraController = controller;
    emit cameraControllerChanged();
}

void GLViewport::setBrushManager(QObject *manager) {
    if (m_brushManager == manager) return;
    m_brushManager = manager;
    emit brushManagerChanged();
}

void GLViewport::setRaycastController(QObject *controller) {
    if (m_raycastController == controller) return;
    m_raycastController = controller;
    emit raycastControllerChanged();
}

void GLViewport::setTerrainManager(QObject *manager) {
    if (m_terrainManager == manager) return;
    m_terrainManager = manager;
    emit terrainManagerChanged();
}

CameraControllerQml *GLViewport::cameraControllerTyped() const {
    return qobject_cast<CameraControllerQml *>(m_cameraController);
}

BrushManagerQml *GLViewport::brushManagerTyped() const {
    return qobject_cast<BrushManagerQml *>(m_brushManager);
}

RaycastControllerQml *GLViewport::raycastControllerTyped() const {
    return qobject_cast<RaycastControllerQml *>(m_raycastController);
}

TerrainManagerQml *GLViewport::terrainManagerTyped() const {
    return qobject_cast<TerrainManagerQml *>(m_terrainManager);
}

void GLViewport::keyPressEvent(QKeyEvent *event) {
    if (auto *camera = cameraControllerTyped()) {
        camera->handleKeyPress(event);
    }
    update();
}

void GLViewport::keyReleaseEvent(QKeyEvent *event) {
    if (auto *camera = cameraControllerTyped()) {
        camera->handleKeyRelease(event);
    }
    update();
}

void GLViewport::mousePressEvent(QMouseEvent *event) {
    if (auto *camera = cameraControllerTyped()) {
        camera->handleMousePress(event, window());
    }
    forceActiveFocus();
    update();
}

void GLViewport::mouseMoveEvent(QMouseEvent *event) {
    if (auto *camera = cameraControllerTyped()) {
        const QQuickWindow *w = window();
        if (w) {
            const QPoint itemPosInWindow = mapToScene(QPointF()).toPoint();
            const QPoint localPos = w->mapFromGlobal(QCursor::pos());
            const QRect rect(itemPosInWindow, QSize(static_cast<int>(width()), static_cast<int>(height())));

            camera->handleMouseMove(event, const_cast<QQuickWindow *>(w), localPos, rect);
        }
    }

    // Mise à jour raycast si disponible
    if (auto *raycast = raycastControllerTyped()) {
        const QQuickWindow *w = window();
        if (w) {
            const QPoint localPos = w->mapFromGlobal(QCursor::pos());
            const QPointF itemPos = mapFromScene(localPos);
            raycast->updateMousePosition(itemPos, static_cast<float>(width()), static_cast<float>(height()));
        }
    }

    update();
}

void GLViewport::mouseReleaseEvent(QMouseEvent *event) {
    if (auto *camera = cameraControllerTyped()) {
        camera->handleMouseRelease(event, window());
    }
    update();
}

void GLViewport::wheelEvent(QWheelEvent *event) {
    if (auto *camera = cameraControllerTyped()) {
        camera->handleWheel(event);
    }
    update();
}

void GLViewport::hoverMoveEvent(QHoverEvent *event) {
    // Ne faire le raycast que si on ne bouge pas la caméra
    if (auto *camera = cameraControllerTyped()) {
        if (camera->isMovingCamera()) {
            return;
        }
    }

    if (auto *raycast = raycastControllerTyped()) {
        const QPointF localPosF = event->position();
        raycast->updateMousePosition(localPosF, static_cast<float>(width()), static_cast<float>(height()));
        update();
    }
}

QQuickFramebufferObject::Renderer *GLViewport::createRenderer() const {
    return new GLRenderer(const_cast<GLViewport *>(this));
}
