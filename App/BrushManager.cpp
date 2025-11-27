#include "BrushManager.h"
#include "Log.h"

#include <algorithm>
#include <QtOpenGL>

void BrushManager::initialize(QOpenGLFunctions *gl, int maxBrushes, int brushTextureSize) {
    m_maxBrushes = maxBrushes;
    m_brushTextureSize = brushTextureSize;

    Q_UNUSED(gl);

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

void BrushManager::destroy(QOpenGLFunctions *gl) {
    if (m_texArray != 0) {
        gl->glDeleteTextures(1, &m_texArray);
        m_texArray = 0;
    }
    m_brushes.clear();
}

bool BrushManager::isValidBrushIndex(const int index) const {
    return index >= 0 && index < static_cast<int>(m_brushes.size()) && m_brushes[static_cast<size_t>(index)].valid;
}

bool BrushManager::uploadBrush(QOpenGLFunctions *gl, int layerIndex, const QImage &src) {
    Q_UNUSED(gl);
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

void BrushManager::loadFromDirectory(QOpenGLFunctions *gl, const QString &directoryPath) {
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

        if (!uploadBrush(gl, i, img)) {
            continue;
        }

        BrushDescriptor desc;
        desc.id = i;
        desc.name = fi.baseName();
        desc.filePath = fi.absoluteFilePath();
        desc.valid = true;
        m_brushes.push_back(desc);
    }

    LOG_INFO() << "Brushes chargés depuis " << directoryPath.toStdString() << ": " << m_brushes.size();
}

int BrushManager::addBrushFromFile(QOpenGLFunctions *gl, const QString &filePath) {
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
    if (!uploadBrush(gl, layerIndex, img)) {
        return -1;
    }

    BrushDescriptor desc;
    desc.id = layerIndex;
    desc.name = QFileInfo(filePath).baseName();
    desc.filePath = filePath;
    desc.valid = true;
    m_brushes.push_back(desc);

    return layerIndex;
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
    m_pendingStrokes.push_back(s);
}

void BrushManager::removeBrush(const int brushIndex) {
    if (!isValidBrushIndex(brushIndex)) return;
    m_brushes[static_cast<size_t>(brushIndex)].valid = false;
    // On laisse les données GPU en place pour éviter de recaler tout l'array.
}
