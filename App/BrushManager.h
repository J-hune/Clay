#ifndef CLAYAPP_BRUSHMANAGER_H
#define CLAYAPP_BRUSHMANAGER_H

#include <vector>
#include <QImage>
#include <QOpenGLFunctions>
#include <QVector3D>

struct BrushDescriptor {
    int id = -1;              // index dans le texture array
    QString name;             // nom affichable
    QString filePath;         // chemin d'origine
    bool valid = false;       // slot occupé
};

struct PendingStroke {
    QVector3D worldPos;
    int brushIndex = 0;
    float size = 1.0f;
    float strength = 1.0f;
};

class BrushManager {
public:
    BrushManager() = default;
    ~BrushManager() = default;

    void initialize(QOpenGLFunctions *gl, int maxBrushes = 32, int brushTextureSize = 128);
    void destroy(QOpenGLFunctions *gl);

    // Chargement initial depuis un dossier ("brushes" par défaut)
    void loadFromDirectory(QOpenGLFunctions *gl, const QString &directoryPath);

    // Ajout dynamique d'un brush
    int addBrushFromFile(QOpenGLFunctions *gl, const QString &filePath);

    // Invalidation d'un brush
    void removeBrush(int brushIndex);

    GLuint brushTextureArrayId() const { return m_texArray; }
    int brushCount() const { return static_cast<int>(m_brushes.size()); }
    const std::vector<BrushDescriptor> &brushes() const { return m_brushes; }
    bool isValidBrushIndex(int index) const;
    int brushTextureSize() const { return m_brushTextureSize; }

    // État courant du brush (contrôlé par l'UI)
    void setCurrentBrushIndex(int index) { m_currentBrushIndex = index; }
    int currentBrushIndex() const { return m_currentBrushIndex; }
    void setBrushSize(float r) { m_brushSize = r; }
    float brushSize() const { return m_brushSize; }
    void setBrushStrength(float s) { m_brushStrength = s; }
    float brushStrength() const { return m_brushStrength; }

    // Gestion des strokes (file d'actions à appliquer)
    void enqueueStroke(const QVector3D &worldPos);
    const std::vector<PendingStroke> &pendingStrokes() const { return m_pendingStrokes; }
    void clearPendingStrokes() { m_pendingStrokes.clear(); }

private:
    bool uploadBrush(QOpenGLFunctions *gl, int layerIndex, const QImage &img);

    GLuint m_texArray = 0;
    int m_maxBrushes = 0;
    int m_brushTextureSize = 0;
    std::vector<BrushDescriptor> m_brushes;

    // État courant du brush
    int m_currentBrushIndex = 0;
    float m_brushSize = 2.0f;
    float m_brushStrength = 1.0f;

    // File de strokes à appliquer
    std::vector<PendingStroke> m_pendingStrokes;
};

#endif // CLAYAPP_BRUSHMANAGER_H
