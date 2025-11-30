#include "BrushManager.h"
#include "Log.h"

#include <algorithm>
#include <QtOpenGL>

void BrushManager::initialize(int maxBrushes, int brushTextureSize) {
    m_maxBrushes = maxBrushes;
    m_brushTextureSize = brushTextureSize;

    if (m_texArray == 0) {
        glGenTextures(1, &m_texArray);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_texArray);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Alloue le storage pour tout l'array (R32F)
        glTexImage3D(GL_TEXTURE_2D_ARRAY,
                     0,
                     GL_R32F,
                     m_brushTextureSize,
                     m_brushTextureSize,
                     m_maxBrushes,
                     0,
                     GL_RED,
                     GL_FLOAT,
                     nullptr);

        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        LOG_INFO() << "Texture array de brushes créé: size=" << m_brushTextureSize << " maxLayers=" << m_maxBrushes;
    }
}

void BrushManager::destroy() {
    m_brushes.clear();
}

bool BrushManager::isValidBrushIndex(const int index) const {
    return index >= 0 && index < static_cast<int>(m_brushes.size()) && m_brushes[static_cast<size_t>(index)].valid;
}

bool BrushManager::uploadBrush(int layerIndex, const QImage &src) {
    if (m_texArray == 0) {
        LOG_ERROR() << "Impossible d'uploader un brush, texture array non initialisée";
        return false;
    }
    if (layerIndex < 0 || layerIndex >= m_maxBrushes) {
        LOG_WARN() << "Indice de layer brush invalide: " << layerIndex;
        return false;
    }

    // Mise au format et à la taille attendus
    QImage img = src;
    if (img.isNull()) {
        LOG_WARN() << "Image de brush invalide";
        return false;
    }

    if (img.width() != m_brushTextureSize || img.height() != m_brushTextureSize) {
        img = img.scaled(m_brushTextureSize, m_brushTextureSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    // Convertit en float grayscale
    std::vector<float> data;
    data.resize(static_cast<size_t>(m_brushTextureSize) * static_cast<size_t>(m_brushTextureSize));
    for (int y = 0; y < m_brushTextureSize; ++y) {
        for (int x = 0; x < m_brushTextureSize; ++x) {
            const QRgb p = img.pixel(x, y);
            const int r = qRed(p);
            const int g = qGreen(p);
            const int b = qBlue(p);
            const float gray = (r * 0.299f + g * 0.587f + b * 0.114f) / 255.f;
            data[static_cast<size_t>(y) * static_cast<size_t>(m_brushTextureSize) + static_cast<size_t>(x)] = gray;
        }
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, m_texArray);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                    0,
                    0,
                    0,
                    layerIndex,
                    m_brushTextureSize,
                    m_brushTextureSize,
                    1,
                    GL_RED,
                    GL_FLOAT,
                    data.data());
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    LOG_INFO() << "Brush uploadé dans layer=" << layerIndex;
    return true;
}

void BrushManager::loadFromDirectory(const QString &directoryPath) {
    if (m_texArray == 0) {
        LOG_WARN() << "BrushManager::loadFromDirectory appelé avant initialize";
        return;
    }

    QDir dir(directoryPath);
    if (!dir.exists()) {
        LOG_WARN() << "Dossier de brushes introuvable: " << directoryPath.toStdString();
        return;
    }

    const QStringList filters{QStringLiteral("*.png"), QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"), QStringLiteral("*.bmp")};
    const QFileInfoList files = dir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name);

    m_brushes.clear();

    const int maxToLoad = std::min(m_maxBrushes, static_cast<int>(files.size()));
    for (int i = 0; i < maxToLoad; ++i) {
        const QFileInfo &fi = files.at(i);
        const QImage img(fi.absoluteFilePath());
        if (img.isNull()) {
            LOG_WARN() << "Échec chargement brush: " << fi.absoluteFilePath().toStdString();
            continue;
        }

        if (!uploadBrush(i, img)) {
            continue;
        }

        BrushDescriptor desc;
        desc.id = i;
        desc.name = fi.baseName();
        desc.filePath = fi.absoluteFilePath();
        desc.valid = true;
        desc.needsUpload = false;
        m_brushes.push_back(desc);
    }

    LOG_INFO() << "Brushes chargés depuis " << directoryPath.toStdString() << ": " << m_brushes.size();
}

int BrushManager::addBrushFromFile(const QString &filePath) {
    if (m_texArray == 0) {
        LOG_WARN() << "BrushManager::addBrushFromFile appelé avant initialize";
        return -1;
    }

    if (static_cast<int>(m_brushes.size()) >= m_maxBrushes) {
        LOG_WARN() << "Nombre maximal de brushes atteint";
        return -1;
    }

    const QImage img(filePath);
    if (img.isNull()) {
        LOG_WARN() << "Impossible de charger le brush: " << filePath.toStdString();
        return -1;
    }

    const int layerIndex = static_cast<int>(m_brushes.size());

    // On crée le descripteur mais on ne l'uploade pas tout de suite
    // L'upload se fera dans le thread de rendu via uploadPendingBrushes()
    BrushDescriptor desc;
    desc.id = layerIndex;
    desc.name = QFileInfo(filePath).baseName();
    desc.filePath = filePath;
    desc.valid = true;
    desc.needsUpload = true;  // Marquer pour upload différé
    m_brushes.push_back(desc);

    LOG_INFO() << "Brush ajouté (en attente d'upload GPU): " << filePath.toStdString();
    return layerIndex;
}

int BrushManager::addBrushFromExternalFile(const QString &sourcePath) {
    if (sourcePath.isEmpty()) return -1;

    const QString storage = m_storageDir;
    if (storage.isEmpty()) {
        LOG_WARN() << "Le dossier de stockage des brushes n'est pas défini.";
        return -1;
    }

    QDir dir(storage);
    if (!dir.exists()) {
        if (!dir.mkpath(storage)) {
            LOG_WARN() << "Impossible de créer le dossier de stockage des brushes: " << storage.toStdString();
            return -1;
        }
    }

    QString source = sourcePath;
    if (source.startsWith("file://")) source = QUrl(source).toLocalFile();
    if (!QFile::exists(source)) {
        LOG_WARN() << "Fichier source de brush introuvable: " << source.toStdString();
        return -1;
    }

    // Compute a destination path avoiding collisions
    const QString baseName = QFileInfo(source).fileName();
    QString destPath = dir.filePath(baseName);
    int suffix = 1;
    while (QFile::exists(destPath)) {
        const QString base = QFileInfo(baseName).completeBaseName();
        const QString suf = QFileInfo(baseName).suffix();
        destPath = dir.filePath(base + QStringLiteral("_%1.").arg(suffix) + suf);
        ++suffix;
    }

    if (!QFile::copy(source, destPath)) {
        LOG_WARN() << "Échec de la copie du brush dans le dossier de stockage: " << destPath.toStdString();
        return -1;
    }

    int idx = addBrushFromFile(destPath);
    if (idx < 0) {
        // Si l'upload a raté, on supprime le fichier copié
        QFile::remove(destPath);
        return -1;
    }

    return idx;
}

bool BrushManager::removeBrushAndFile(const int brushIndex) {
    if (!isValidBrushIndex(brushIndex)) return false;

    const QString path = m_brushes[static_cast<size_t>(brushIndex)].filePath;

    // On invalide le descripteur en premierr
    m_brushes[static_cast<size_t>(brushIndex)].valid = false;

    bool removed = false;
    if (!path.isEmpty() && QFile::exists(path)) {
        removed = QFile::remove(path);
        if (!removed) {
            LOG_WARN() << "Échec de la suppression du fichier de brush: " << path.toStdString();
        }
    }

    return removed;
}

void BrushManager::enqueueStroke(const QVector3D &worldPos) {
    if (!isValidBrushIndex(m_currentBrushIndex)) {
        return;
    }
    PendingStroke s;
    s.worldPos = worldPos;
    s.brushIndex = m_currentBrushIndex;
    s.size = m_brushSize;
    s.strength = m_brushStrength;
    s.operation = m_operation;
    m_pendingStrokes.push_back(s);
}

void BrushManager::removeBrush(const int brushIndex) {
    if (!isValidBrushIndex(brushIndex)) return;
    m_brushes[static_cast<size_t>(brushIndex)].valid = false;
    // On laisse les données GPU en place pour éviter de recaler tout l'array.
}

void BrushManager::uploadPendingBrushes() {
    for (auto &brush : m_brushes) {
        if (brush.needsUpload && brush.valid) {
            const QImage img(brush.filePath);
            if (!img.isNull()) {
                if (uploadBrush(brush.id, img)) {
                    brush.needsUpload = false;
                    LOG_INFO() << "Brush uploadé sur GPU: " << brush.filePath.toStdString();
                } else {
                    LOG_WARN() << "Échec de l'upload GPU pour: " << brush.filePath.toStdString();
                }
            } else {
                LOG_WARN() << "Impossible de charger l'image pour upload: " << brush.filePath.toStdString();
            }
        }
    }
}

void BrushManager::setCurrentBrushIndex(int index) {
    if (m_currentBrushIndex != index) {
        m_currentBrushIndex = index;
        m_dirty = true;
    }
}

void BrushManager::setBrushSize(float r) {
    if (qAbs(m_brushSize - r) > 1e-6f) {
        m_brushSize = r;
        m_dirty = true;
    }
}

void BrushManager::setBrushStrength(float s) {
    if (qAbs(m_brushStrength - s) > 1e-6f) {
        m_brushStrength = s;
        m_dirty = true;
    }
}

void BrushManager::setOperation(BrushOpType op) {
    if (m_operation != op) {
        m_operation = op;
        m_dirty = true;
    }
}

