#include "MyGLItem.h"
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>

class GLRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions {
public:
    GLRenderer() { initializeOpenGLFunctions(); }

    void render() override {
        glViewport(0, 0, framebufferObject()->width(), framebufferObject()->height());
        glClearColor(0.1f, 0.1f, 0.1f, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        // petit triangle
        constexpr GLfloat verts[] = {
            0.0f, 0.8f,
            -0.8f, -0.8f,
            0.8f, -0.8f
        };

        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, verts);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDisableClientState(GL_VERTEX_ARRAY);

        update(); // permet l’animation continue
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        QOpenGLFramebufferObjectFormat fmt;
        fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        return new QOpenGLFramebufferObject(size, fmt);
    }
};

QQuickFramebufferObject::Renderer *MyGLItem::createRenderer() const { return new GLRenderer(); }
