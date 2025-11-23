#include "MyGLItem.h"
#include "Grid.h"
#include "CameraController.h"
#include "TerrainMesh.h"
#include "TerrainGpu.h"
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObjectFormat>
#include <QMatrix4x4>
#include <QObject>
#include <QElapsedTimer>
#include <QtGlobal>
#include <QQuickWindow>
#include <cmath>
#include <iostream>
#include <QImageReader>
#include <QFileInfo>

static QImage loadHeightImage(const QUrl &url) {
    const QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
    QImage img;
    QImageReader reader(path);
    const QString suffix = QFileInfo(path).suffix().toLower();
    std::cout << "Loading heightmap image from: " << path.toStdString() << " (suffix: " << suffix.toStdString() << ")" << std::endl;
    if (suffix == QLatin1String("exr")) reader.setFormat("exr");
    if (reader.canRead()) {
        reader.setAutoTransform(true);
        img = reader.read();
        if (!img.isNull()) return img;
    } else {
        std::cout << "Impossible de lire l'image: " << reader.errorString().toStdString() << std::endl;
        // TODO faire une alerte image + implémenter EXR
    }
    // Fallback simple
    img.load(path);
    return img;
}

MyGLItem::MyGLItem(QQuickItem *parent) : QQuickFramebufferObject(parent) {
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setFlag(ItemIsFocusScope, true);
    setFlag(ItemAcceptsInputMethod, true);
    setFocus(true);
}

void MyGLItem::setGridResolution(int r) {
    const int old = m_grid.resolution();
    m_grid.setResolution(r);
    if (m_grid.resolution() != old) {
        emit gridResolutionChanged();
        update();
    }
}

void MyGLItem::setCameraSpeed(const float s) {
    const float old = m_cameraController.speed();
    m_cameraController.setSpeed(s);
    if (!qFuzzyCompare(old, m_cameraController.speed())) {
        emit cameraSpeedChanged();
    }
}
void MyGLItem::setMouseSensitivity(float s) {
    if (s < 0.f) s = 0.f;
    if (qFuzzyCompare(m_cameraController.mouseSensitivity(), s)) return;
    m_cameraController.setMouseSensitivity(s);
    emit mouseSensitivityChanged();
}
void MyGLItem::setDrawGrid(const bool v) {
    if (m_drawGrid == v) return;
    m_drawGrid = v;
    emit drawGridChanged();
}
void MyGLItem::setDrawAxes(const bool v) {
    if (m_drawAxes == v) return;
    m_drawAxes = v;
    emit drawAxesChanged();
}
void MyGLItem::setOrbitDistance(const float d) {
    const float old = m_cameraController.orbitDistance();
    m_cameraController.setOrbitDistance(d);
    if (!qFuzzyCompare(old, m_cameraController.orbitDistance())) emit orbitDistanceChanged();
}


void MyGLItem::setTerrainResolution(int r) {
    if (r < 2) r = 2;
    if (r == m_terrainResolution) return;
    m_terrainResolution = r;
    emit terrainResolutionChanged();
}
void MyGLItem::setHeightmapResolution(int r) {
    constexpr int allowed[4] = {512, 1024, 2048, 4096};
    int chosen = 512;
    for (const int v : allowed) if (r == v) { chosen = v; break; }
    if (chosen == m_heightmapResolution) return;
    m_heightmapResolution = chosen;
    ++m_terrainRevision; // rebuild GPU texture
    emit heightmapResolutionChanged();
    update();
}

void MyGLItem::setHeightmapSource(const QUrl &url) {
    if (url == m_heightmapSource) return;
    m_heightmapSource = url;
    emit heightmapSourceChanged();
}
void MyGLItem::setHeightScale(float s) {
    if (s < 0.f) s = 0.f;
    if (qFuzzyCompare(s, m_heightScale)) return;
    m_heightScale = s;
    emit heightScaleChanged();
}
void MyGLItem::setTerrainMode(int m) {
    if (m == m_terrainMode) return;
    if (m < 0 || m > 1) return; // limitation actuelle
    m_terrainMode = m;
    emit terrainModeChanged();
}

void MyGLItem::generateTerrain() {
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

void MyGLItem::keyPressEvent(QKeyEvent *event) {
    m_cameraController.handleKeyPress(event);
    update();
}

void MyGLItem::keyReleaseEvent(QKeyEvent *event) {
    m_cameraController.handleKeyRelease(event);
    update();
}

void MyGLItem::mousePressEvent(QMouseEvent *event) {
    m_cameraController.handleMousePress(event, window());
    forceActiveFocus();
    update();
}

void MyGLItem::mouseMoveEvent(QMouseEvent *event) {
    const QQuickWindow *w = window();
    const QPoint itemPosInWindow = mapToScene(QPointF()).toPoint();
    const QPoint localPos = w->mapFromGlobal(QCursor::pos());
    const QRect rect(itemPosInWindow, QSize(width(), height()));

    const float oldYaw = m_cameraController.camera().yaw();
    const float oldPitch = m_cameraController.camera().pitch();
    m_cameraController.handleMouseMove(event, w, localPos, rect);
    if (!qFuzzyCompare(oldYaw, m_cameraController.camera().yaw())) emit yawChanged();
    if (!qFuzzyCompare(oldPitch, m_cameraController.camera().pitch())) emit pitchChanged();

    update();
}

void MyGLItem::mouseReleaseEvent(QMouseEvent *event) {
    m_cameraController.handleMouseRelease(event, window());
    update();
}

void MyGLItem::wheelEvent(QWheelEvent *event) {
    const float oldDist = m_cameraController.orbitDistance();
    const float oldSpeed = m_cameraController.speed();
    m_cameraController.handleWheel(event);
    const bool distChanged = !qFuzzyCompare(oldDist, m_cameraController.orbitDistance());
    const bool speedChanged = !qFuzzyCompare(oldSpeed, m_cameraController.speed());
    if (speedChanged) emit cameraSpeedChanged();
    if (distChanged) emit orbitDistanceChanged();
    if (speedChanged || distChanged) update();
}

// Renderer OpenGL
class GLRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions {
public:
    GLRenderer() {
        initializeOpenGLFunctions();
        m_timer.start();
        m_terrainGpu.initialize(this);
    }

    void synchronize(QQuickFramebufferObject *item) override {
        auto *glItem = qobject_cast<MyGLItem*>(item);
        if (!glItem) return;
        // Synchronisation caméra / stats
        const QVector3D oldPos = glItem->m_cameraController.camera().position();
        glItem->m_cameraController.camera().setPosition(m_cameraController.camera().position());
        if (!qFuzzyCompare(oldPos.x(), glItem->m_cameraController.camera().position().x()) ||
            !qFuzzyCompare(oldPos.y(), glItem->m_cameraController.camera().position().y()) ||
            !qFuzzyCompare(oldPos.z(), glItem->m_cameraController.camera().position().z())) {
            emit glItem->cameraPositionChanged();
        }
        const float oldFps = glItem->m_fps;
        glItem->m_fps = m_fpsAccum;
        if (!qFuzzyCompare(oldFps, glItem->m_fps)) emit glItem->fpsChanged();

        // Copie état utilisateur
        m_cameraController.camera().setPosition(glItem->m_cameraController.camera().position());
        m_cameraController.camera().setSpeed(glItem->m_cameraController.camera().speed());
        m_cameraController.camera().setOrbitDistance(glItem->m_cameraController.camera().orbitDistance());
        m_cameraController.camera().setOrbitPivot(glItem->m_cameraController.camera().orbitPivot());
        m_cameraController.camera().setYaw(glItem->m_cameraController.camera().yaw());
        m_cameraController.camera().setPitch(glItem->m_cameraController.camera().pitch());
        m_cameraController.camera().setMouseSensitivity(glItem->m_cameraController.camera().mouseSensitivity());
        m_cameraController.copyInputFrom(glItem->m_cameraController);
        m_grid.setResolution(glItem->m_grid.resolution());
        m_drawGrid = glItem->m_drawGrid;
        m_drawAxes = glItem->m_drawAxes;

        // Gestion révision terrain
        if (glItem->m_userRequestedTerrain && glItem->m_terrainRevision != m_lastTerrainRevision) {
            m_lastTerrainRevision = glItem->m_terrainRevision;
            m_terrainGpu.setGridResolution(glItem->m_terrainResolution, glItem->m_terrainResolution);
            m_terrainGpu.setTextureResolution(glItem->m_heightmapResolution);
            if (glItem->m_terrainMode == 1 && !glItem->m_heightmapSource.isEmpty()) {
                const QImage img = loadHeightImage(glItem->m_heightmapSource);
                if (!img.isNull())m_terrainGpu.rebuild(this, img, glItem->m_heightScale);
                else m_terrainGpu.rebuildFlat(this, glItem->m_heightScale);
            } else {
                m_terrainGpu.rebuildFlat(this, glItem->m_heightScale);
            }
            m_terrainReady = true;
        }
    }

    void render() override {
        // On calcule le delta time
        const qint64 ns = m_timer.nsecsElapsed();
        m_timer.restart();
        const double dtSec = qBound(0.0, static_cast<double>(ns) / 1e9, 0.1); // On limite les sauts
        const float dt = static_cast<float>(dtSec);

        // Mise à jour FPS (simple lissage exponentiel)
        if (dt > 0.f) {
            const float instantFps = 1.0f / dt;
            if (m_fpsAccum < 0.f) m_fpsAccum = instantFps; else m_fpsAccum = m_fpsAccum * 0.9f + instantFps * 0.1f;
        }

        // Mise à jour caméra via le contrôleur
        m_cameraController.update(dt);

        // On prépare l'état GL
        glViewport(0, 0, framebufferObject()->width(), framebufferObject()->height());
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.129f, 0.141f, 0.161f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const int w = framebufferObject()->width();
        const int h = framebufferObject()->height();
        const float aspect = h > 0 ? static_cast<float>(w) / static_cast<float>(h) : 1.0f;

        // On crée la matrice de projection
        QMatrix4x4 proj; proj.perspective(60.f, aspect, 0.1f, 1000.f);
        proj.scale(1.f, -1.f, 1.f); // On corrige l'inversion Y du FBO
        const QMatrix4x4 view = m_cameraController.camera().viewMatrix();

        // On charge les matrices dans la pile fixe
        glMatrixMode(GL_PROJECTION); glLoadMatrixf(proj.constData());
        glMatrixMode(GL_MODELVIEW); glLoadMatrixf(view.constData());

        // On dessine la grille / axes selon les flags
        m_grid.draw(this, m_drawGrid, m_drawAxes);

        // Terrain shaderisé
        if (m_terrainReady) {
            m_terrainGpu.draw(this, proj, view);
        }

        update();
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        QOpenGLFramebufferObjectFormat fmt;
        fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        return new QOpenGLFramebufferObject(size, fmt);
    }

private:
    QElapsedTimer m_timer;
    CameraController m_cameraController;
    Grid m_grid;
    bool m_drawGrid = true;
    bool m_drawAxes = true;
    float m_fpsAccum = -1.f;
    TerrainGpu m_terrainGpu;
    bool m_terrainReady = false;
    int m_lastTerrainRevision = -1;
};

QQuickFramebufferObject::Renderer *MyGLItem::createRenderer() const { return new GLRenderer(); }
