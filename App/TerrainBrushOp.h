#ifndef CLAYAPP_TERRAINBRUSHOP_H
#define CLAYAPP_TERRAINBRUSHOP_H

#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QScopedPointer>

enum class BrushOpType {
    Raise = 0,
    Lower = 1,
    Smooth = 2,
};

class TerrainBrushOp {
public:
    void initialize(QOpenGLExtraFunctions *gl);
    void cleanup(QOpenGLExtraFunctions *gl);

    void applyBrush(QOpenGLExtraFunctions *gl,
                   BrushOpType opType,
                   GLuint heightmapTexture,
                   int heightmapResolution,
                   const QVector3D &worldPos,
                   float brushSize,
                   float brushStrength,
                   GLuint brushTextureArray,
                   int brushIndex,
                   float terrainMinX,
                   float terrainMaxX,
                   float terrainMinZ,
                   float terrainMaxZ,
                   float heightScale);

private:
    void ensureProgram(QOpenGLExtraFunctions *gl);

    QScopedPointer<QOpenGLShaderProgram> m_computeProgram;
};

#endif

