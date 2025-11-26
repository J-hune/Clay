#ifndef CLAYAPP_IMAGEUTILS_H
#define CLAYAPP_IMAGEUTILS_H

#include <QImageReader>
#include <QFileInfo>
#include "Log.h"

// Chargement d'image utilitaire (supporte EXR via QImageReader si disponible) // TODO FIX
inline QImage loadHeightImage(const QUrl &url) {
    const QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
    QImage img;
    QImageReader reader(path);
    const QString suffix = QFileInfo(path).suffix().toLower();
    LOG_INFO() << "Chargement heightmap: " << path.toStdString() << " (suffixe: " << suffix.toStdString() << ")";
    if (suffix == QLatin1String("exr")) reader.setFormat("exr");
    if (reader.canRead()) {
        reader.setAutoTransform(true);
        img = reader.read();
        if (!img.isNull()) {
            LOG_INFO() << "Heightmap lue avec succès: " << img.width() << "x" << img.height() << " format=" << img.format();
            return img;
        }
        LOG_WARN() << "Lecture heightmap: image vide (" << reader.errorString().toStdString() << ")";
    } else {
        LOG_WARN() << "Impossible de lire l'image via QImageReader: " << reader.errorString().toStdString();
    }
    if (!img.load(path)) {
        LOG_ERROR() << "Échec chargement heightmap via QImage::load: " << path.toStdString();
    } else {
        LOG_INFO() << "Heightmap chargée via QImage::load: " << img.width() << "x" << img.height();
    }
    return img;
}

#endif // CLAYAPP_IMAGEUTILS_H