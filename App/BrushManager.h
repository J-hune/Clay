#ifndef CLAYAPP_BRUSHMANAGER_H
#define CLAYAPP_BRUSHMANAGER_H

#include <qimage.h>
#include <QObject>
#include <QStringList>
#include <GL/gl.h>
#include <atomic>

#include "Camera.h"
#include "Grid.h"

class BrushManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList brushes READ brushes NOTIFY brushesChanged)
public:
    explicit BrushManager(QObject *parent = nullptr);

    Q_INVOKABLE void refresh(); // Scanne le dossier des brushes
    QStringList brushes() const { return m_brushPaths; } // Liste des chemins de brosses disponibles
    QString selectedBrush() const { return m_selectedBrushPath; } // Chemin de la brosse sélectionnée

    // Sélectionne une nouvelle brosse (charge l'image)
    Q_INVOKABLE void loadBrushTexture(const QString &newPath);
    void uploadTextureIfNeeded();

    // Dessine le placeholder de brosse à l'écran
    void drawPlaceholder(QVector3D hitPoint, const Camera &camera);
    // Convertit une position écran (QPoint) en un point d'intersection avec le plan Y = m_planeY
    bool screenToPlane(const QPoint &screenPos, int viewW, int viewH,const Camera &camera, float fovDeg, const Grid *grid, QVector3D &outHit) const;

    // Paramètres brosse (taille, force)
    float brushSize() const { return m_brushSize; }
    float brushStrength() const { return m_brushStrength; }
    void setBrushStrength(float s) { m_brushStrength = s; updateAlphaFromLuminance();}
    void setBrushSize(float s) { m_brushSize = s; }

    signals:
        void brushesChanged();

private:
    // Scanne le dossier des brushes et met à jour m_brushPaths
    void scanBrushFolder();

    // Met à jour le canal alpha de l'image de la brosse en fonction de la luminance et de la force
    void updateAlphaFromLuminance();

    QStringList m_brushPaths;
    GLuint m_brushTex = 0;
    QImage m_brushImage;
    QString m_selectedBrushPath; // chemin actuel (pour détecter changement)
    QString m_selectedBrush;

    float m_planeY = 0.01f; // légèrement au-dessus de la grille
    float m_brushSize = 20.f;
    float m_brushStrength = 25.f;

    std::atomic_bool m_needsUpload { false };
};

#endif //CLAYAPP_BRUSHMANAGER_H