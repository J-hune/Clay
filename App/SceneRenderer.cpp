#include "SceneRenderer.h"
#include "Grid.h"
#include "TerrainGpu.h"
#include "CameraController.h"

void SceneRenderer::setupRenderState(QOpenGLExtraFunctions *gl, int width, int height) {
    gl->glViewport(0, 0, width, height);
    gl->glEnable(GL_DEPTH_TEST);
}

void SceneRenderer::clearFramebuffer(QOpenGLExtraFunctions *gl) {
    gl->glClearColor(0.129f, 0.141f, 0.161f, 1);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void SceneRenderer::computeMatrices(
    int width,
    int height,
    const CameraController &camera,
    QMatrix4x4 &outProj,
    QMatrix4x4 &outView
) {
    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;

    outProj.setToIdentity();
    outProj.perspective(60.f, aspect, 0.1f, 1000.f);
    outProj.scale(1.f, -1.f, 1.f);

    outView = camera.camera().viewMatrix();
}

void SceneRenderer::drawScene(
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
) {
    // Configurer les matrices avec l'API legacy OpenGL
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(proj.constData());
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(view.constData());

    // Dessiner la grille
    grid.draw(gl, drawGrid, drawAxes);

    // Dessiner le terrain si prêt
    if (terrainReady) {
        terrainGpu.setBrushTextureArray(brushTextureArrayId);
        terrainGpu.draw(gl, proj, view, cameraFront);
    }
}
