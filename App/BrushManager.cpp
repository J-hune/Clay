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

    const QImage img = src.size() == QSize(m_brushTextureSize, m_brushTextureSize)
                           ? src
                           : src.scaled(m_brushTextureSize, m_brushTextureSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    if (img.isNull()) {
        LOG_WARN() << "Image de brush invalide";
        return false;
    }

    QImage grayImg = img.convertToFormat(QImage::Format_Grayscale16);

    const int n = m_brushTextureSize * m_brushTextureSize;
    if (m_tempUploadBuffer.size() < n) m_tempUploadBuffer.resize(n);

    const ushort *srcPtr = reinterpret_cast<const ushort *>(grayImg.constBits());
    std::transform(srcPtr, srcPtr + n, m_tempUploadBuffer.begin(), [](ushort v) { return v / 65535.0f; });

    glBindTexture(GL_TEXTURE_2D_ARRAY, m_texArray);
    glTexSubImage3D(
        GL_TEXTURE_2D_ARRAY,
        0, 0, 0, layerIndex,
        m_brushTextureSize, m_brushTextureSize, 1,
        GL_RED, GL_FLOAT, m_tempUploadBuffer.data()
    );
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    LOG_DEBUG() << "Brush uploadé dans layer=" << layerIndex;
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

    if (files.isEmpty()) {
        LOG_WARN() << "Aucun brush trouvé dans: " << directoryPath.toStdString();
        return;
    }

    m_brushes.clear();
    m_brushes.reserve(static_cast<size_t>(std::min(m_maxBrushes, static_cast<int>(files.size()))));

    const int maxToLoad = std::min(m_maxBrushes, static_cast<int>(files.size()));

    // Charger le premier brush de manière synchrone pour avoir un brush utilisable immédiatement
    if (maxToLoad > 0) {
        const QFileInfo &firstFile = files.at(0);
        const QImage firstImage(firstFile.absoluteFilePath());

        BrushDescriptor firstDesc;
        firstDesc.id = 0;
        firstDesc.name = firstFile.baseName();
        firstDesc.filePath = firstFile.absoluteFilePath();
        firstDesc.valid = true;

        if (!firstImage.isNull() && uploadBrush(0, firstImage)) {
            firstDesc.needsUpload = false;
            LOG_INFO() << "Premier brush chargé (synchrone): " << firstFile.fileName().toStdString();
        } else {
            firstDesc.needsUpload = true;
            LOG_WARN() << "Échec du chargement synchrone du premier brush, sera réessayé en asynchrone";
        }

        m_brushes.push_back(firstDesc);
    }

    // Créer les descripteurs pour les brushes restants (chargement asynchrone)
    for (int i = 1; i < maxToLoad; ++i) {
        const QFileInfo &fi = files.at(i);

        BrushDescriptor desc;
        desc.id = i;
        desc.name = fi.baseName();
        desc.filePath = fi.absoluteFilePath();
        desc.valid = true;
        desc.needsUpload = true;  // Upload asynchrone
        m_brushes.push_back(desc);
    }

    // Marquer comme dirty pour forcer le rendu pendant le chargement asynchrone
    if (pendingUploadsCount() > 0) {
        m_dirty = true;
    }

    LOG_INFO() << "Brushes enregistrés: " << m_brushes.size()
               << " (1 chargé, " << (maxToLoad - 1) << " en attente)";
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
    // Configuration: nombre de brushes à charger par frame pour équilibrer performance/fluidité
    constexpr int MAX_UPLOADS_PER_FRAME = 1;
    int uploadedThisFrame = 0;
    bool hasPendingBrushes = false;

    for (auto &brush: m_brushes | std::views::filter([](auto &b) { return b.valid && b.needsUpload; })) {
        hasPendingBrushes = true;

        if (uploadedThisFrame >= MAX_UPLOADS_PER_FRAME) break;

        QImage img(brush.filePath);
        if (img.isNull()) {
            LOG_WARN() << "Impossible de charger l'image: " << brush.filePath.toStdString();
            brush.needsUpload = false;
            brush.valid = false;
            continue;
        }

        if (uploadBrush(brush.id, img)) {
            brush.needsUpload = false;
            uploadedThisFrame++;
            LOG_DEBUG() << "Brush " << (brush.id + 1) << "/" << m_brushes.size()<< " chargé: " << brush.name.toStdString();
        } else {
            LOG_ERROR() << "Échec de l'upload GPU pour: " << brush.filePath.toStdString();
            brush.needsUpload = false;
            brush.valid = false;
        }
    }

    // Maintenir le dirty flag tant que le chargement n'est pas terminé
    if (hasPendingBrushes) m_dirty = true;
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

int BrushManager::pendingUploadsCount() const {
    int count = 0;
    for (const auto &brush : m_brushes) {
        if (brush.valid && brush.needsUpload) {
            count++;
        }
    }
    return count;
}

bool BrushManager::isLoadingComplete() const {
    return pendingUploadsCount() == 0;
}

float BrushManager::loadingProgress() const {
    if (m_brushes.empty()) return 1.0f;

    int total = 0;
    int loaded = 0;
    for (const auto &brush : m_brushes) {
        if (brush.valid) {
            total++;
            if (!brush.needsUpload) {
                loaded++;
            }
        }
    }

    return total > 0 ? static_cast<float>(loaded) / static_cast<float>(total) : 1.0f;
}

