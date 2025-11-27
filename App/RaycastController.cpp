#include "RaycastController.h"
#include "TerrainRaycast.h"

RaycastController::RaycastController() {
    m_raycast = new TerrainRaycast();
}

void RaycastController::initialize(QOpenGLExtraFunctions *gl) const {
    if (m_raycast) {
        m_raycast->initialize(gl);
    }
}

void RaycastController::updateMousePosition(const QVector2D &ndc) {
    m_mouseNDC = ndc;
}

void RaycastController::perform(QOpenGLExtraFunctions *gl,
                                const QMatrix4x4 &proj,
                                const QMatrix4x4 &view,
                                GLuint heightmapTex,
                                int texResolution,
                                float heightScale) {
    if (!m_raycast) return;

    // Configuration des bounds du terrain
    m_raycast->setTerrainBounds(-50.0f, 50.0f, -50.0f, 50.0f);

    // Effectuer le raycast
    m_raycast->performRaycast(gl, m_mouseNDC, proj, view, heightmapTex, texResolution, heightScale);

    // Lire le résultat
    const RaycastResult result = m_raycast->readResult(gl);

    if (result.hitFlag > 0.5f) {
        m_hasHit = true;
        m_hitPosition = QVector3D(result.posX, result.posY, result.posZ);
    } else {
        m_hasHit = false;
        m_hitPosition = QVector3D();
    }
}

void RaycastController::reset() {
    m_hasHit = false;
    m_hitPosition = QVector3D();
}
