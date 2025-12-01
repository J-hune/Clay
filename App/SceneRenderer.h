#ifndef CLAYAPP_SCENERENDERER_H
#define CLAYAPP_SCENERENDERER_H

#include <QVector3D>
#include <QOpenGLExtraFunctions>

class Grid;
class TerrainGpu;
class CameraController;

/**
 * @brief SceneRenderer - Responsable du dessin de la scène 3D
 *
 * Sépare la logique de rendu de la gestion du renderer principal.
 * Contient les méthodes pour dessiner la grille, le terrain, etc.
 */
class SceneRenderer {
public:
    /**
     * @brief Dessine toute la scène 3D
     */
    static void drawScene(
        QOpenGLExtraFunctions *gl,
        const Grid &grid,
        TerrainGpu &terrainGpu,
        GLuint brushTextureArrayId,
        bool drawGrid,
        bool drawAxes,
        bool terrainReady,
        const QMatrix4x4 &proj,
        const QMatrix4x4 &view,
        const QVector3D &cameraFront
    );

    /**
     * @brief Configure l'état OpenGL pour le rendu
     */
    static void setupRenderState(QOpenGLExtraFunctions *gl, int width, int height);

    /**
     * @brief Nettoie le framebuffer
     */
    static void clearFramebuffer(QOpenGLExtraFunctions *gl);

    /**
     * @brief Calcule les matrices de projection et vue
     */
    static void computeMatrices(
        int width,
        int height,
        const CameraController &camera,
        QMatrix4x4 &outProj,
        QMatrix4x4 &outView
    );
};

#endif // CLAYAPP_SCENERENDERER_H
