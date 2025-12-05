#ifndef CLAYAPP_GLVIEWPORT_H
#define CLAYAPP_GLVIEWPORT_H

#include "Grid.h"
#include <QQuickFramebufferObject>

// Forward declarations
class CameraControllerQml;
class BrushManagerQml;
class RaycastControllerQml;
class TerrainManagerQml;
class ErosionControllerQml;

/**
 * @brief GLViewport - Point d'entrée pour le rendu OpenGL et la gestion des événements
 *
 * Responsabilités :
 * - Capture des événements utilisateur (souris, clavier)
 * - Propriétés visuelles (grille, axes, FPS)
 * - Création du renderer OpenGL
 *
 * La logique métier est déléguée aux composants QML et au GLRenderer.
 */
class GLViewport : public QQuickFramebufferObject {
    Q_OBJECT

    // Propriétés d'affichage
    Q_PROPERTY(int gridResolution READ gridResolution WRITE setGridResolution NOTIFY gridResolutionChanged)
    Q_PROPERTY(bool drawGrid READ drawGrid WRITE setDrawGrid NOTIFY drawGridChanged)
    Q_PROPERTY(bool drawAxes READ drawAxes WRITE setDrawAxes NOTIFY drawAxesChanged)
    Q_PROPERTY(float fps READ fps NOTIFY fpsChanged)

    // Composants métier (injectés depuis QML)
    Q_PROPERTY(QObject* cameraController READ cameraController WRITE setCameraController NOTIFY cameraControllerChanged)
    Q_PROPERTY(QObject* brushManager READ brushManager WRITE setBrushManager NOTIFY brushManagerChanged)
    Q_PROPERTY(QObject* raycastController READ raycastController WRITE setRaycastController NOTIFY raycastControllerChanged)
    Q_PROPERTY(QObject* terrainManager READ terrainManager WRITE setTerrainManager NOTIFY terrainManagerChanged)
    Q_PROPERTY(QObject* erosionController READ erosionController WRITE setErosionController NOTIFY erosionControllerChanged)

public:
    explicit GLViewport(QQuickItem *parent = nullptr);

    [[nodiscard]] Renderer *createRenderer() const override;

    // Propriétés d'affichage
    int gridResolution() const { return m_grid.resolution(); }
    void setGridResolution(int r);

    bool drawGrid() const { return m_drawGrid; }
    void setDrawGrid(bool v);

    bool drawAxes() const { return m_drawAxes; }
    void setDrawAxes(bool v);

    float fps() const { return m_fps; }
    void setFpsFromRenderer(float fps); // Appelé par le renderer

    // Accès aux composants
    QObject* cameraController() const { return m_cameraController; }
    void setCameraController(QObject* controller);

    QObject* brushManager() const { return m_brushManager; }
    void setBrushManager(QObject* manager);

    QObject* raycastController() const { return m_raycastController; }
    void setRaycastController(QObject* controller);

    QObject* terrainManager() const { return m_terrainManager; }
    void setTerrainManager(QObject* manager);

    QObject* erosionController() const { return m_erosionController; }
    void setErosionController(QObject* controller);

    // Accès pour le renderer
    const Grid& grid() const { return m_grid; }

    CameraControllerQml* cameraControllerTyped() const;
    BrushManagerQml* brushManagerTyped() const;
    RaycastControllerQml* raycastControllerTyped() const;
    TerrainManagerQml* terrainManagerTyped() const;
    ErosionControllerQml* erosionControllerTyped() const;

    // Export
    Q_INVOKABLE void exportHeightmap(const QString &filePath);

signals:
    void gridResolutionChanged();
    void drawGridChanged();
    void drawAxesChanged();
    void fpsChanged();
    void cameraControllerChanged();
    void brushManagerChanged();
    void raycastControllerChanged();
    void terrainManagerChanged();
    void erosionControllerChanged();

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

    Grid m_grid;
    bool m_drawGrid = true;
    bool m_drawAxes = true;
    float m_fps = 0.f;

    QObject* m_cameraController = nullptr;
    QObject* m_brushManager = nullptr;
    QObject* m_raycastController = nullptr;
    QObject* m_terrainManager = nullptr;
    QObject* m_erosionController = nullptr;

    QString m_pendingExportPath;
    bool m_pendingErosion = false;
};

#endif // CLAYAPP_GLVIEWPORT_H
