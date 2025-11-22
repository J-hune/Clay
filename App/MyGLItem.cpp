#include "MyGLItem.h"
#include "Grid.h"
#include "InputManager.h"
#include "Camera.h"
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObjectFormat>
#include <QMatrix4x4>
#include <QObject>
#include <QElapsedTimer>
#include <QtGlobal>
#include <QQuickWindow>

// QMatrix4x4 vers float*
static const float *toPtr(const QMatrix4x4 &m) { return m.constData(); }

MyGLItem::MyGLItem(QQuickItem *parent) : QQuickFramebufferObject(parent) {
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setFlag(ItemIsFocusScope, true);
    setFlag(ItemAcceptsInputMethod, true);
    setFocus(true);
}

void MyGLItem::setGridResolution(int r) {
    r = qBound(1, r, 10000);
    if (m_gridResolution == r) return;
    m_gridResolution = r;
    emit gridResolutionChanged();
    update();
}

void MyGLItem::setCameraSpeed(const float s) {
    const float old = m_camera.speed();
    m_camera.setSpeed(s);
    if (!qFuzzyCompare(old, m_camera.speed())) {
        emit cameraSpeedChanged();
    }
}
void MyGLItem::setMouseSensitivity(float s) {
    if (s < 0.f) s = 0.f;
    if (qFuzzyCompare(m_camera.mouseSensitivity(), s)) return;
    m_camera.setMouseSensitivity(s);
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
    const float old = m_camera.orbitDistance();
    m_camera.setOrbitDistance(d);
    if (!qFuzzyCompare(old, m_camera.orbitDistance())) emit orbitDistanceChanged();
}

void MyGLItem::keyPressEvent(QKeyEvent *event) {
    m_input.keyPress(event);
    update();
}

void MyGLItem::keyReleaseEvent(QKeyEvent *event) {
    m_input.keyRelease(event);
    update();
}

void MyGLItem::mousePressEvent(QMouseEvent *event) {
    m_input.mousePress(event, window());
    forceActiveFocus();
    update();
}

void MyGLItem::mouseMoveEvent(QMouseEvent *event) {
    const QQuickWindow *w = window();
    const QPoint itemPosInWindow = mapToScene(QPointF()).toPoint();
    const QPoint localPos = w->mapFromGlobal(QCursor::pos());
    const QRect rect(itemPosInWindow, QSize(width(), height()));

    m_input.mouseMove(event, w, localPos, rect);
    if (m_input.rightButtonDown() || m_input.middleButtonDown()) {
        const QPoint delta = m_input.mouseDelta();
        float y = m_camera.yaw();
        float p = m_camera.pitch();
        const float sens = m_camera.mouseSensitivity();
        y += static_cast<float>(delta.x()) * sens;
        p += static_cast<float>(-delta.y()) * sens;
        m_camera.setYaw(y);
        m_camera.setPitch(p);
        emit yawChanged();
        emit pitchChanged();
    }

    update();
}

void MyGLItem::mouseReleaseEvent(QMouseEvent *event) {
    m_input.mouseRelease(event, window());

    update();
}

// TODO FIX
void MyGLItem::wheelEvent(QWheelEvent *event) {
    const QPoint numDegrees = event->angleDelta() / 8; // 1 "degree" = 1/8 de tour
    if (!numDegrees.isNull()) {
        const float steps = static_cast<float>(numDegrees.y()) / 15.f; // y: molette verticale
        if (steps != 0.f) {
            float d = m_camera.orbitDistance();
            const float factor = std::pow(1.15f, -steps); // wheel up -> rapproche
            d *= factor;
            d = qBound(0.1f, d, 1000.f);
            setOrbitDistance(d);
            update();
        }
    }
    event->accept();
}

// Renderer OpenGL
class GLRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions {
public:
    GLRenderer() { initializeOpenGLFunctions(); m_timer.start(); }

    void synchronize(QQuickFramebufferObject *item) override {
        auto *glItem = qobject_cast<MyGLItem*>(item);
        if (!glItem) return;
        const QVector3D oldPos = glItem->m_camera.position();
        glItem->m_camera.setPosition(m_camera.position());
        if (!qFuzzyCompare(oldPos.x(), glItem->m_camera.position().x()) ||
            !qFuzzyCompare(oldPos.y(), glItem->m_camera.position().y()) ||
            !qFuzzyCompare(oldPos.z(), glItem->m_camera.position().z())) {
            emit glItem->cameraPositionChanged();
        }
        const float oldFps = glItem->m_fps;
        glItem->m_fps = m_fpsAccum;
        if (!qFuzzyCompare(oldFps, glItem->m_fps)) emit glItem->fpsChanged();

        // Copie état utilisateur
        m_camera.setPosition(glItem->m_camera.position());
        m_camera.setSpeed(glItem->m_camera.speed());
        m_camera.setOrbitDistance(glItem->m_camera.orbitDistance());
        m_camera.setYaw(glItem->m_camera.yaw());
        m_camera.setPitch(glItem->m_camera.pitch());
        m_camera.setMouseSensitivity(glItem->m_camera.mouseSensitivity());
        m_input.copyFrom(glItem->m_input);
        m_grid.setResolution(glItem->m_gridResolution);
        m_drawGrid = glItem->m_drawGrid;
        m_drawAxes = glItem->m_drawAxes;
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

        // Mise à jour caméra
        m_camera.update(dt, m_input);

        // On prépare l'état GL
        glViewport(0, 0, framebufferObject()->width(), framebufferObject()->height());
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.12f, 0.12f, 0.12f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const int w = framebufferObject()->width();
        const int h = framebufferObject()->height();
        const float aspect = h > 0 ? static_cast<float>(w) / static_cast<float>(h) : 1.0f;

        // On crée la matrice de projection
        QMatrix4x4 proj; proj.perspective(60.f, aspect, 0.1f, 1000.f);
        proj.scale(1.f, -1.f, 1.f); // On corrige l'inversion Y du FBO
        const QMatrix4x4 view = m_camera.viewMatrix();

        // On charge les matrices dans la pile fixe
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(toPtr(proj));
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(toPtr(view));

        // On dessine la grille / axes selon les flags
        m_grid.draw(this, m_drawGrid, m_drawAxes);

        update();
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        QOpenGLFramebufferObjectFormat fmt;
        fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        return new QOpenGLFramebufferObject(size, fmt);
    }

private:
    QElapsedTimer m_timer;
    InputManager m_input;
    Camera m_camera;
    Grid m_grid;
    bool m_drawGrid = true;
    bool m_drawAxes = true;
    float m_fpsAccum = -1.f;
    MyGLItem *m_item = nullptr;
};

QQuickFramebufferObject::Renderer *MyGLItem::createRenderer() const { return new GLRenderer(); }
