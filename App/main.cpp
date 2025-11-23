#include <QApplication>
#include <QQmlApplicationEngine>
#include <qqmlcontext.h>

#include "BrushManager.h"
#include "MyGLItem.h"
#include "autogen/environment.h"

int main(int argc, char *argv[]) {
    set_qt_environment();
    QApplication app(argc, argv);

    qmlRegisterType<MyGLItem>("MyGL", 1, 0, "MyGLItem");


    BrushManager *brushLib = new BrushManager(&app);

    QQmlApplicationEngine engine;
    const QUrl url(mainQmlFile);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);
    engine.rootContext()->setContextProperty("BrushLib", brushLib);
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/qml");
    engine.addImportPath(":/");
    engine.load(url);

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
