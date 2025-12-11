#ifndef CLAYAPP_EROSIONBRUSHOP_H
#define CLAYAPP_EROSIONBRUSHOP_H

#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QScopedPointer>

class ErosionBrushOp {
public:
    void initialize(QOpenGLExtraFunctions *gl);
    void cleanup(QOpenGLExtraFunctions *gl);

    void applyMask(QOpenGLExtraFunctions *gl,
                   GLuint maskTexture,      // GL_R8
                   int maskResolution,
                   const QVector3D &worldPos,
                   float brushSize,
                   float brushStrength,
                   GLuint brushTextureArray,
                   int brushIndex,
                   float terrainMinX,
                   float terrainMaxX,
                   float terrainMinZ,
                   float terrainMaxZ,
                   const QVector3D &cameraForward,
                   bool erase);

private:
    void ensureProgram(QOpenGLExtraFunctions *gl);
    QScopedPointer<QOpenGLShaderProgram> m_program;
};

#endif // CLAYAPP_EROSIONBRUSHOP_H
