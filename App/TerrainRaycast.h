#ifndef CLAYAPP_TERRAINRAYCAST_H
#define CLAYAPP_TERRAINRAYCAST_H

#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QScopedPointer>

// Structure résultat raycast (doit matcher le SSBO côté shader)
struct RaycastResult {
    float posX, posY, posZ, pad1;
    float normalX, normalY, normalZ, pad2;
    float hitFlag; // 1.0 = hit, 0.0 = no hit
    float pad3, pad4, pad5;
};

class TerrainRaycast {
public:
    void initialize(QOpenGLExtraFunctions *gl);
    void cleanup(QOpenGLExtraFunctions *gl);

    // Lance un raycast GPU depuis mouseNDC (x,y en [-1,1])
    void performRaycast(QOpenGLExtraFunctions *gl,
                       const QVector2D &mouseNDC,
                       const QMatrix4x4 &projectionMatrix,
                       const QMatrix4x4 &viewMatrix,
                       GLuint heightmapTexture,
                       int heightmapResolution,
                       float heightScale);

    // Lit le résultat depuis le SSBO
    RaycastResult readResult(QOpenGLExtraFunctions *gl);

    void setTerrainBounds(float minX, float maxX, float minZ, float maxZ) {
        m_terrainMinX = minX;
        m_terrainMaxX = maxX;
        m_terrainMinZ = minZ;
        m_terrainMaxZ = maxZ;
    }

private:
    void ensureProgram(QOpenGLExtraFunctions *gl);
    void ensureSSBO(QOpenGLExtraFunctions *gl);

    QScopedPointer<QOpenGLShaderProgram> m_computeProgram;
    GLuint m_ssbo = 0;

    // Bounds du terrain
    float m_terrainMinX = -1.0f;
    float m_terrainMaxX = 1.0f;
    float m_terrainMinZ = -1.0f;
    float m_terrainMaxZ = 1.0f;
};

#endif // CLAYAPP_TERRAINRAYCAST_H

