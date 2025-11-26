#ifndef CLAYAPP_TERRAINGPU_H
#define CLAYAPP_TERRAINGPU_H

#include <vector>
#include <QImage>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QScopedPointer>

class TerrainGpu {
public:
    void initialize(QOpenGLFunctions *gl);
    void rebuild(QOpenGLFunctions *gl, const QImage &heightImage, float heightScale);
    void rebuildFlat(QOpenGLFunctions *gl, float heightScale);
    void draw(QOpenGLFunctions *gl, const QMatrix4x4 &proj, const QMatrix4x4 &view);

    void setGridResolution(int rx, int rz) { m_gridResX = rx; m_gridResZ = rz; m_dirtyMesh = true; }
    void setTextureResolution(int r) { m_texRes = r; m_dirtyTexture = true; }

    GLuint heightmapTexture() const { return m_tex; }

    void setRaycastHit(const QVector3D &pos) { m_hitPos = pos; m_hitValid = true; }
    void clearRaycastHit() { m_hitValid = false; }

private:
    void ensureMesh(QOpenGLFunctions *gl);
    void ensureProgram(const QOpenGLFunctions *gl);
    void ensureTexture(QOpenGLFunctions *gl);
    void uploadHeightData(QOpenGLFunctions *gl, const float *data, int width, int height);

    // GL resources
    GLuint m_vbo = 0; // UVs (vec2)
    GLuint m_ibo = 0;
    GLuint m_tex = 0; // GL_R32F
    std::vector<unsigned int> m_indices;

    QScopedPointer<QOpenGLShaderProgram> m_program;

    // State
    int m_gridResX = 256;
    int m_gridResZ = 256;
    int m_texRes = 512;
    float m_heightScale = 30.f;

    bool m_dirtyMesh = true;
    bool m_dirtyTexture = true;
    bool m_haveTextureData = false;

    // Raycast hit visualization
    QVector3D m_hitPos{0.0f, 0.0f, 0.0f};
    bool m_hitValid = false;
};

#endif // CLAYAPP_TERRAINGPU_H