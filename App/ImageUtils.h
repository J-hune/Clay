#ifndef CLAYAPP_IMAGEUTILS_H
#define CLAYAPP_IMAGEUTILS_H

#include <QImageReader>
#include <QFileInfo>
#include <iostream>

// Chargement d'image utilitaire (supporte EXR via QImageReader si disponible) // TODO FIX
inline QImage loadHeightImage(const QUrl &url) {
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
    }
    img.load(path);
    return img;
}

#endif // CLAYAPP_IMAGEUTILS_H