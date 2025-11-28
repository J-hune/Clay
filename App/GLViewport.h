#ifndef CLAYAPP_GLVIEWPORT_H
#define CLAYAPP_GLVIEWPORT_H

#include "Grid.h"
#include <QQuickFramebufferObject>
#include <QtCore/QTimer>

// Forward declarations
class CameraControllerQml;
class BrushManagerQml;
class RaycastControllerQml;
class TerrainManagerQml;

/**
 * @brief GLViewport - Responsable uniquement du rendering OpenGL
 * Les composants (Camera, Brush, Raycast, Terrain) sont gérés par des QObject séparés
 */
class GLViewport : public QQuickFramebufferObject {
    Q_OBJECT

    Q_PROPERTY(int gridResolution READ gridResolution WRITE setGridResolution NOTIFY gridResolutionChanged)
    Q_PROPERTY(bool drawGrid READ drawGrid WRITE setDrawGrid NOTIFY drawGridChanged)
    Q_PROPERTY(bool drawAxes READ drawAxes WRITE setDrawAxes NOTIFY drawAxesChanged)
    Q_PROPERTY(float fps READ fps NOTIFY fpsChanged)

    // Références aux composants externes
    Q_PROPERTY(QObject* cameraController READ cameraController WRITE setCameraController NOTIFY cameraControllerChanged)
    Q_PROPERTY(QObject* brushManager READ brushManager WRITE setBrushManager NOTIFY brushManagerChanged)
    Q_PROPERTY(QObject* raycastController READ raycastController WRITE setRaycastController NOTIFY raycastControllerChanged)
    Q_PROPERTY(QObject* terrainManager READ terrainManager WRITE setTerrainManager NOTIFY terrainManagerChanged)

public:
    explicit GLViewport(QQuickItem *parent = nullptr);

    [[nodiscard]] Renderer *createRenderer() const override;

    // Synchronisation avec le renderer
    void setFpsFromRenderer(float fps);

    // Grid properties
    int gridResolution() const { return m_grid.resolution(); }
    void setGridResolution(int r);
    bool drawGrid() const { return m_drawGrid; }
    void setDrawGrid(bool v);
    bool drawAxes() const { return m_drawAxes; }
    void setDrawAxes(bool v);
    float fps() const { return m_fps; }

    // Accès aux composants
    QObject* cameraController() const { return m_cameraController; }
    void setCameraController(QObject* controller);
    QObject* brushManager() const { return m_brushManager; }
    void setBrushManager(QObject* manager);
    QObject* raycastController() const { return m_raycastController; }
    void setRaycastController(QObject* controller);
    QObject* terrainManager() const { return m_terrainManager; }
    void setTerrainManager(QObject* manager);

    // Accès typé pour le renderer
    CameraControllerQml* cameraControllerTyped() const;
    BrushManagerQml* brushManagerTyped() const;
    RaycastControllerQml* raycastControllerTyped() const;
    TerrainManagerQml* terrainManagerTyped() const;

    // Accès à la grille (pour le renderer)
    const Grid& grid() const { return m_grid; }

signals:
    void gridResolutionChanged();
    void drawGridChanged();
    void drawAxesChanged();
    void fpsChanged();
    void cameraControllerChanged();
    void brushManagerChanged();
    void raycastControllerChanged();
    void terrainManagerChanged();

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

    void applyBrushAtCurrentPosition();

    Grid m_grid;
    float m_fps = 0.f;
    bool m_drawGrid = true;
    bool m_drawAxes = true;
    bool m_isLeftButtonPressed = false;

    // Timer pour application continue du brush
    QTimer m_brushTimer;

    // Pointeurs vers les composants externes
    QObject* m_cameraController = nullptr;
    QObject* m_brushManager = nullptr;
    QObject* m_raycastController = nullptr;
    QObject* m_terrainManager = nullptr;
};

#endif // CLAYAPP_GLVIEWPORT_H

