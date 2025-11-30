#include <QApplication>
#include <QFontDatabase>
#include <QQmlApplicationEngine>

#include "GLViewport.h"
#include "qml/CameraControllerQml.h"
#include "qml/BrushManagerQml.h"
#include "qml/RaycastControllerQml.h"
#include "qml/TerrainManagerQml.h"
#include "autogen/environment.h"
#include "Log.h"

int main(int argc, char *argv[]) {
    set_qt_environment();
    QApplication app(argc, argv);
    Log::setLevel(Log::Level::Info);

    // Chargement des fonts Open Sans
    int fontIdRegular = QFontDatabase::addApplicationFont(":/qt/qml/ClayContent/fonts/OpenSans-Regular.ttf");
    int fontIdSemiBold = QFontDatabase::addApplicationFont(":/qt/qml/ClayContent/fonts/OpenSans-SemiBold.ttf");

    if (fontIdRegular != -1) {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontIdRegular);
        if (!fontFamilies.isEmpty()) {
            QString family = fontFamilies.at(0);
            QFont defaultFont(family);
            QApplication::setFont(defaultFont);
            LOG_INFO() << "Font par défaut définie: " << family.toStdString();
        } else {
            LOG_WARN() << "Familles de font trouvées mais vide";
        }
    } else {
        LOG_WARN() << "Impossible de charger la font Open Sans Regular, utilisation de la font système";
    }

    if (fontIdSemiBold != -1) {
        LOG_INFO() << "Font Open Sans SemiBold chargée avec succès";
    } else {
        LOG_WARN() << "Impossible de charger la font Open Sans SemiBold";
    }

    qmlRegisterType<GLViewport>("MyGL", 1, 0, "GLViewport");
    qmlRegisterType<CameraControllerQml>("MyGL", 1, 0, "CameraController");
    qmlRegisterType<BrushManagerQml>("MyGL", 1, 0, "BrushManager");
    qmlRegisterType<RaycastControllerQml>("MyGL", 1, 0, "RaycastController");
    qmlRegisterType<TerrainManagerQml>("MyGL", 1, 0, "TerrainManager");

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
