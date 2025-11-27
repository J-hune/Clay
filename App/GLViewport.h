#ifndef CLAYAPP_GLVIEWPORT_H
#define CLAYAPP_GLVIEWPORT_H

#include "CameraController.h"
#include "Grid.h"
#include "TerrainMesh.h"
#include <QQuickFramebufferObject>
#include <QVector3D>

class GLViewport : public QQuickFramebufferObject {
    Q_OBJECT

public:
    explicit GLViewport(QQuickItem *parent = nullptr);

    // Methodes de synchronisation avec le renderer
    void setFpsFromRenderer(float fps);
    bool userRequestedTerrain() const { return m_userRequestedTerrain; }
    int terrainRevision() const { return m_terrainRevision; }

    Q_PROPERTY(int gridResolution READ gridResolution WRITE setGridResolution NOTIFY gridResolutionChanged)
    Q_PROPERTY(QVector3D cameraPosition READ cameraPosition NOTIFY cameraPositionChanged)
    Q_PROPERTY(float yaw READ yaw NOTIFY yawChanged)
    Q_PROPERTY(float pitch READ pitch NOTIFY pitchChanged)
    Q_PROPERTY(float fps READ fps NOTIFY fpsChanged)
    Q_PROPERTY(float mouseSensitivity READ mouseSensitivity WRITE setMouseSensitivity NOTIFY mouseSensitivityChanged)
    Q_PROPERTY(bool drawGrid READ drawGrid WRITE setDrawGrid NOTIFY drawGridChanged)
    Q_PROPERTY(bool drawAxes READ drawAxes WRITE setDrawAxes NOTIFY drawAxesChanged)
    Q_PROPERTY(float cameraSpeed READ cameraSpeed WRITE setCameraSpeed NOTIFY cameraSpeedChanged)
    Q_PROPERTY(float orbitDistance READ orbitDistance WRITE setOrbitDistance NOTIFY orbitDistanceChanged)
    Q_PROPERTY(float cameraSpeedMin READ cameraSpeedMin CONSTANT)
    Q_PROPERTY(float cameraSpeedMax READ cameraSpeedMax CONSTANT)
    Q_PROPERTY(float orbitDistanceMin READ orbitDistanceMin CONSTANT)
    Q_PROPERTY(float orbitDistanceMax READ orbitDistanceMax CONSTANT)
    Q_PROPERTY(int terrainResolution READ terrainResolution WRITE setTerrainResolution NOTIFY terrainResolutionChanged)
    Q_PROPERTY(int heightmapResolution READ heightmapResolution WRITE setHeightmapResolution NOTIFY heightmapResolutionChanged)
    Q_PROPERTY(bool terrainReady READ terrainReady NOTIFY terrainReadyChanged)
    Q_PROPERTY(QUrl heightmapSource READ heightmapSource WRITE setHeightmapSource NOTIFY heightmapSourceChanged)
    Q_PROPERTY(float heightScale READ heightScale WRITE setHeightScale NOTIFY heightScaleChanged)
    Q_PROPERTY(int terrainMode READ terrainMode WRITE setTerrainMode NOTIFY terrainModeChanged) // 0=Flat 1=Heightmap

    [[nodiscard]] Renderer *createRenderer() const override;

    // Getters accessibles depuis QML
    QVector3D cameraPosition() const { return m_cameraController.camera().position(); }
    float yaw() const { return m_cameraController.camera().yaw(); }
    float pitch() const { return m_cameraController.camera().pitch(); }
    float fps() const { return m_fps; }
    float cameraSpeed() const { return m_cameraController.speed(); }
    float mouseSensitivity() const { return m_cameraController.mouseSensitivity(); }
    int gridResolution() const { return m_grid.resolution(); }
    bool drawGrid() const { return m_drawGrid; }
    bool drawAxes() const { return m_drawAxes; }
    float orbitDistance() const { return m_cameraController.orbitDistance(); }
    float cameraSpeedMin() const { return m_cameraController.cameraSpeedMin(); }
    float cameraSpeedMax() const { return m_cameraController.cameraSpeedMax(); }
    float orbitDistanceMin() const { return m_cameraController.orbitDistanceMin(); }
    float orbitDistanceMax() const { return m_cameraController.orbitDistanceMax(); }
    int terrainResolution() const { return m_terrainResolution; }
    int heightmapResolution() const { return m_heightmapResolution; }
    bool terrainReady() const { return m_terrainReady; }
    QUrl heightmapSource() const { return m_heightmapSource; }
    float heightScale() const { return m_heightScale; }
    int terrainMode() const { return m_terrainMode; }

    // Setters modifiables depuis QML
    void setCameraSpeed(float s);
    void setMouseSensitivity(float s);
    void setGridResolution(int r);
    void setDrawGrid(bool v);
    void setDrawAxes(bool v);
    void setOrbitDistance(float d);
    void setTerrainResolution(int r);
    void setHeightmapResolution(int r);
    void setHeightmapSource(const QUrl &url);
    void setHeightScale(float s);
    void setTerrainMode(int m);
    Q_INVOKABLE void generateTerrain();

signals:
    void gridResolutionChanged();
    void cameraPositionChanged();
    void yawChanged();
    void pitchChanged();
    void fpsChanged();
    void mouseSensitivityChanged();
    void drawGridChanged();
    void drawAxesChanged();
    void cameraSpeedChanged();
    void orbitDistanceChanged();
    void terrainResolutionChanged();
    void terrainReadyChanged();
    void heightmapSourceChanged();
    void heightScaleChanged();
    void terrainModeChanged();
    void heightmapResolutionChanged();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;

private:
    friend class GLRenderer;
    CameraController m_cameraController;
    Grid m_grid;
    float m_fps = 0.f;
    bool m_drawGrid = true;
    bool m_drawAxes = true;

    // Terrain
    TerrainMesh m_terrainMesh; // maillage actuel
    int m_terrainResolution = 256; // densité (X=Z)
    bool m_terrainReady = false;
    int m_terrainRevision = 0; // incrémenté à chaque génération pour forcer re-upload
    QUrl m_heightmapSource; // source image
    float m_heightScale = 30.f;
    int m_terrainMode = 0; // 0 flat, 1 heightmap
    bool m_userRequestedTerrain = false; // ajout: flag demande rebuild terrain
    int m_heightmapResolution = 512; // ajout: résolution texture heightmap par défaut

    // Raycast GPU
    QVector2D m_mouseNDC{0.f, 0.f}; // Position souris en NDC
    bool m_raycastRequested = false;

    // Méthode pour réinitialiser le raycast
    void invalidateRaycast() { m_raycastRequested = false; }
};

#endif // CLAYAPP_GLVIEWPORT_H
