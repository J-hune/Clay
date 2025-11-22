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

void MyGLItem::keyPressEvent(QKeyEvent *event) {
    m_input.keyPress(event);
    update();
}

void MyGLItem::keyReleaseEvent(QKeyEvent *event) {
    m_input.keyRelease(event);
    update();
}

void MyGLItem::mousePressEvent(QMouseEvent *event) {
    m_input.mousePress(event);
    forceActiveFocus();
    update();
}

void MyGLItem::mouseMoveEvent(QMouseEvent *event) {
    m_input.mouseMove(event, m_yaw, m_pitch, m_mouseSensitivity);
    update();
}

void MyGLItem::mouseReleaseEvent(QMouseEvent *event) {
    m_input.mouseRelease(event);
    update();
}

// Renderer OpenGL
class GLRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions {
public:
    GLRenderer() { initializeOpenGLFunctions(); m_timer.start(); }

    void synchronize(QQuickFramebufferObject *item) override {
        auto *glItem = qobject_cast<MyGLItem*>(item);
        if (!glItem) return; // On protège en cas de type inattendu
        // On persiste la position mise à jour côté rendu dans l'item
        glItem->m_camera.setPosition(m_camera.position());
        // On copie l'état courant de l'item côté rendu
        m_camera.setPosition(glItem->m_camera.position());
        m_yaw = glItem->m_yaw;
        m_pitch = glItem->m_pitch;
        m_input.copyFrom(glItem->m_input);
        m_grid.setResolution(glItem->m_gridResolution);
    }

    void render() override {
        // On calcule le delta time
        const qint64 ns = m_timer.nsecsElapsed();
        m_timer.restart();
        const double dtSec = qBound(0.0, static_cast<double>(ns) / 1e9, 0.1); // On limite les sauts
        const float dt = static_cast<float>(dtSec);

        // On met à jour la caméra
        m_camera.update(dt, m_input, m_yaw, m_pitch);

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

        // On crée la matrice de vue
        const QMatrix4x4 view = m_camera.viewMatrix();

        // On charge les matrices dans la pile fixe
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(toPtr(proj));
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(toPtr(view));

        // On dessine la grille
        m_grid.draw(this);

        update(); // On continue l'animation
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
    float m_yaw = -90.f;
    float m_pitch = 0.f;
};

QQuickFramebufferObject::Renderer *MyGLItem::createRenderer() const { return new GLRenderer(); }
