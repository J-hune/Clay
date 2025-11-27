#ifndef CLAYAPP_RAYCASTCONTROLLER_H
#define CLAYAPP_RAYCASTCONTROLLER_H

#include <QVector2D>
#include <QOpenGLExtraFunctions>

class TerrainRaycast;
class TerrainGpu;

class RaycastController {
public:
    RaycastController();

    void initialize(QOpenGLExtraFunctions *gl) const;

    void updateMousePosition(const QVector2D &ndc);

    void perform(QOpenGLExtraFunctions *gl,
                 const QMatrix4x4 &proj,
                 const QMatrix4x4 &view,
                 GLuint heightmapTex,
                 int texResolution,
                 float heightScale);

    bool hasHit() const { return m_hasHit; }
    QVector3D hitPosition() const { return m_hitPosition; }

    void reset();

private:
    TerrainRaycast *m_raycast = nullptr;
    QVector2D m_mouseNDC{0.f, 0.f};
    bool m_hasHit = false;
    QVector3D m_hitPosition;
};

#endif // CLAYAPP_RAYCASTCONTROLLER_H
