#include <QApplication>
#include <QQmlApplicationEngine>

#include "GLViewport.h"
#include "autogen/environment.h"
#include "Log.h"

int main(int argc, char *argv[]) {
    set_qt_environment();
    QApplication app(argc, argv);
    Log::setLevel(Log::Level::Debug);

    qmlRegisterType<GLViewport>("MyGL", 1, 0, "GLViewport");

    QQmlApplicationEngine engine;
    const QUrl url(mainQmlFile);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);

    engine.addImportPath(QCoreApplication::applicationDirPath() + "/qml");
    engine.addImportPath(":/");
    engine.load(url);

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
