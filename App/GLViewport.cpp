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

    // Configuration du timer pour l'application continue du brush
    m_brushTimer.setInterval(16); // ~60 FPS
    connect(&m_brushTimer, &QTimer::timeout, this, &GLViewport::applyBrushAtCurrentPosition);
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
    if (event->button() == Qt::LeftButton) {
        m_isLeftButtonPressed = true;
        auto *raycast = raycastControllerTyped();
        auto *brush = brushManagerTyped();
        auto *camera = cameraControllerTyped();

        if (raycast && brush && raycast->hasHit() && camera && !camera->isMovingCamera()) {
            brush->enqueueStroke(raycast->hitPosition());
            if (!m_brushTimer.isActive()) {
                m_brushTimer.start();
            }
        }
    }

    if (auto *camera = cameraControllerTyped()) {
        camera->handleMousePress(event, window());
    }
    forceActiveFocus();
    update();
}

void GLViewport::mouseMoveEvent(QMouseEvent *event) {
    auto *camera = cameraControllerTyped();
    auto *raycast = raycastControllerTyped();
    const QQuickWindow *w = window();

    if (!w) {
        update();
        return;
    }

    const QPoint itemPosInWindow = mapToScene(QPointF()).toPoint();
    const QPoint localPos = w->mapFromGlobal(QCursor::pos());
    const QRect rect(itemPosInWindow, QSize(static_cast<int>(width()), static_cast<int>(height())));

    // Gestion du mouvement de caméra
    if (camera) {
        camera->handleMouseMove(event, const_cast<QQuickWindow *>(w), localPos, rect);

        // On arrête le timer si on commence à bouger la caméra
        if (camera->isMovingCamera() && m_brushTimer.isActive()) {
            m_brushTimer.stop();
        }
    }

    // Mise à jour du raycast
    if (raycast) {
        const QPointF itemPos = mapFromScene(localPos);
        raycast->updateMousePosition(itemPos, static_cast<float>(width()), static_cast<float>(height()));

        // Application du brush si clic gauche maintenu
        if (m_isLeftButtonPressed && raycast->hasHit() && camera && !camera->isMovingCamera()) {
            if (auto *brush = brushManagerTyped()) {
                brush->enqueueStroke(raycast->hitPosition());
            }
        }
    }

    update();
}

void GLViewport::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_isLeftButtonPressed = false;
        m_brushTimer.stop();
    }

    if (auto *camera = cameraControllerTyped()) {
        camera->handleMouseRelease(event, window());
    }
    update();
}

void GLViewport::wheelEvent(QWheelEvent *event) {
    // Alt + Molette : changer la taille du pinceau
    if (event->modifiers() & Qt::AltModifier) {
        if (auto *brush = brushManagerTyped()) {
            const QPoint numDegrees = event->angleDelta() / 8; // 1 "degree" = 1/8 de tour
            const float delta = numDegrees.x() / 15.f; // 1 cran = 15°
            const float newSize = qMax(0.5f, qMin(100.0f, brush->brushSize() + delta));
            brush->setBrushSize(newSize);
        }

        event->accept();
        update();
        return;
    }

    // Sinon, comportement normal de la caméra
    if (auto *camera = cameraControllerTyped()) {
        camera->handleWheel(event);
    }
    update();
}

void GLViewport::hoverMoveEvent(QHoverEvent *event) {
    // Ne faire le raycast que si on ne bouge pas la caméra
    auto *camera = cameraControllerTyped();
    if (camera && camera->isMovingCamera()) {
        return;
    }

    if (auto *raycast = raycastControllerTyped()) {
        raycast->updateMousePosition(event->position(), static_cast<float>(width()), static_cast<float>(height()));
        update();
    }
}

void GLViewport::applyBrushAtCurrentPosition() {
    auto *raycast = raycastControllerTyped();
    auto *brush = brushManagerTyped();
    auto *camera = cameraControllerTyped();

    // Vérifier toutes les conditions pour appliquer le brush
    const bool canApplyBrush = m_isLeftButtonPressed &&
                               raycast && brush && camera &&
                               raycast->hasHit() &&
                               !camera->isMovingCamera();

    if (canApplyBrush) {
        brush->enqueueStroke(raycast->hitPosition());
        update();
        if (window()) {
            window()->update();
        }
    } else if (m_brushTimer.isActive()) {
        // Arrêter le timer si les conditions ne sont plus remplies
        m_brushTimer.stop();
    }
}

QQuickFramebufferObject::Renderer *GLViewport::createRenderer() const {
    return new GLRenderer(const_cast<GLViewport *>(this));
}
