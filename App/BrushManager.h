#ifndef CLAYAPP_BRUSHMANAGER_H
#define CLAYAPP_BRUSHMANAGER_H

#include <vector>
#include <QImage>

#include "TerrainBrushOp.h"

struct BrushDescriptor {
    int id = -1;              // index dans le texture array
    QString name;             // nom affichable
    QString filePath;         // chemin d'origine
    bool valid = false;       // slot occupé
    bool needsUpload = false; // Marqueur pour upload différé
};

struct PendingStroke {
    QVector3D worldPos;
    int brushIndex = 0;
    float size = 1.0f;
    float strength = 1.0f;
    BrushOpType operation = BrushOpType::Raise;
};

class BrushManager {
public:
    BrushManager() = default;
    ~BrushManager() = default;

    void initialize(int maxBrushes = 64, int brushTextureSize = 1024);
    void destroy();

    // Chargement initial depuis un dossier ("brushes" par défaut)
    void loadFromDirectory(const QString &directoryPath);

    // Ajout dynamique d'un brush (le fichier doit être déjà dans le dossier géré par l'app)
    int addBrushFromFile(const QString &filePath);

    // Ajout dynamique d'un brush depuis un fichier externe : copie dans le dossier de stockage puis upload
    int addBrushFromExternalFile(const QString &sourcePath);

    // Invalidation d'un brush (ne supprime pas le fichier)
    void removeBrush(int brushIndex);

    // Supprime le brush et tente de supprimer aussi le fichier physique associé
    bool removeBrushAndFile(int brushIndex);

    GLuint brushTextureArrayId() const { return m_texArray; }
    int brushCount() const { return static_cast<int>(m_brushes.size()); }
    const std::vector<BrushDescriptor> &brushes() const { return m_brushes; }
    bool isValidBrushIndex(int index) const;
    int brushTextureSize() const { return m_brushTextureSize; }

    // État courant du brush (contrôlé par l'UI)
    void setCurrentBrushIndex(int index);
    int currentBrushIndex() const { return m_currentBrushIndex; }
    void setBrushSize(float r);
    float brushSize() const { return m_brushSize; }
    void setBrushStrength(float s);
    float brushStrength() const { return m_brushStrength; }
    void setOperation(BrushOpType op);
    BrushOpType operation() const { return m_operation; }

    // Dirty flag pour le rendu
    bool isDirty() const { return m_dirty; }
    void clearDirty() { m_dirty = false; }

    // Permet de définir ou récupérer le dossier de stockage des brushes
    void setStorageDirectory(const QString &dir) { m_storageDir = dir; }
    QString storageDirectory() const { return m_storageDir; }

    // Gestion des strokes (file d'actions à appliquer)
    void enqueueStroke(const QVector3D &worldPos);
    const std::vector<PendingStroke> &pendingStrokes() const { return m_pendingStrokes; }
    void clearPendingStrokes() { m_pendingStrokes.clear(); }

    // Upload des brushes en attente (appelé dans le thread de rendu)
    void uploadPendingBrushes();

private:
    bool uploadBrush(int layerIndex, const QImage &img);

    GLuint m_texArray = 0;
    int m_maxBrushes = 0;
    int m_brushTextureSize = 0;
    std::vector<BrushDescriptor> m_brushes;

    // État courant du brush
    int m_currentBrushIndex = 0;
    float m_brushSize = 2.0f;
    float m_brushStrength = 1.0f;
    BrushOpType m_operation = BrushOpType::Raise;
    bool m_dirty = false;

    // File de strokes à appliquer
    std::vector<PendingStroke> m_pendingStrokes;

    // Dossier de stockage des brushes
    QString m_storageDir;
};

#endif // CLAYAPP_BRUSHMANAGER_H
