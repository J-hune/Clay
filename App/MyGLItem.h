#ifndef CLAYAPP_MYGLITEM_H
#define CLAYAPP_MYGLITEM_H

#include <QQuickFramebufferObject>

class MyGLItem : public QQuickFramebufferObject {
    Q_OBJECT

public:
    Renderer *createRenderer() const override;
};

#endif //CLAYAPP_MYGLITEM_H
