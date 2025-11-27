#ifndef CLAYAPP_TERRAINMANAGERQML_H
#define CLAYAPP_TERRAINMANAGERQML_H

#include <QUrl>
#include "../TerrainMesh.h"

/**
 * @brief Manager QML pour le terrain
 * Gère la génération, les paramètres et expose les propriétés
 */
class TerrainManagerQml : public QObject {
    Q_OBJECT

    Q_PROPERTY(int resolution READ resolution WRITE setResolution NOTIFY resolutionChanged)
    Q_PROPERTY(int heightmapResolution READ heightmapResolution WRITE setHeightmapResolution NOTIFY heightmapResolutionChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
    Q_PROPERTY(QUrl heightmapSource READ heightmapSource WRITE setHeightmapSource NOTIFY heightmapSourceChanged)
    Q_PROPERTY(float heightScale READ heightScale WRITE setHeightScale NOTIFY heightScaleChanged)
    Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY modeChanged) // 0=Flat, 1=Heightmap
    Q_PROPERTY(int triangleCount READ triangleCount NOTIFY triangleCountChanged)

public:
    explicit TerrainManagerQml(QObject *parent = nullptr);

    // Accès au mesh interne (pour GLRenderer)
    TerrainMesh& mesh() { return m_mesh; }
    const TerrainMesh& mesh() const { return m_mesh; }

    // Revision pour forcer re-upload GPU
    int revision() const { return m_revision; }
    bool needsUpload() const { return m_needsUpload; }
    void setNeedsUpload(bool needs) { m_needsUpload = needs; }

    // Getters pour Q_PROPERTY
    int resolution() const { return m_resolution; }
    int heightmapResolution() const { return m_heightmapResolution; }
    bool ready() const { return m_ready; }
    QUrl heightmapSource() const { return m_heightmapSource; }
    float heightScale() const { return m_heightScale; }
    int mode() const { return m_mode; }
    int triangleCount() const;

    // Setters
    void setResolution(int r);
    void setHeightmapResolution(int r);
    void setHeightmapSource(const QUrl &url);
    void setHeightScale(float s);
    void setMode(int m);

    // Méthodes invocables depuis QML
    Q_INVOKABLE void generate();

signals:
    void resolutionChanged();
    void heightmapResolutionChanged();
    void readyChanged();
    void heightmapSourceChanged();
    void heightScaleChanged();
    void modeChanged();
    void triangleCountChanged();
    void terrainGenerated(); // Signal pour notifier le renderer

private:
    TerrainMesh m_mesh;
    int m_resolution = 256;
    int m_heightmapResolution = 512;
    bool m_ready = false;
    QUrl m_heightmapSource;
    float m_heightScale = 50.0f;
    int m_mode = 0; // 0=Flat, 1=Heightmap
    int m_revision = 0;
    bool m_needsUpload = false;
};

#endif // CLAYAPP_TERRAINMANAGERQML_H
