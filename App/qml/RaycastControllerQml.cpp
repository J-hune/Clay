#include "RaycastControllerQml.h"

RaycastControllerQml::RaycastControllerQml(QObject *parent)
    : QObject(parent) {
}

void RaycastControllerQml::updateMousePosition(const QPointF &localPos, float viewportWidth, float viewportHeight) {
    if (viewportWidth > 0.f && viewportHeight > 0.f) {
        const QVector2D newNDC(
            (2.0f * static_cast<float>(localPos.x()) / viewportWidth) - 1.0f,
            (2.0f * static_cast<float>(localPos.y()) / viewportHeight) - 1.0f
        );

        // Seuil de changement significatif pour éviter trop de raycast
        if ((newNDC - m_mouseNDC).lengthSquared() > 1e-6f) {
            m_mouseNDC = newNDC;
            m_controller.updateMousePosition(m_mouseNDC);
            emit mouseNDCChanged();
            emit raycastRequested();
        }
    }
}

void RaycastControllerQml::reset() {
    m_controller.reset();
    if (m_lastHasHit != m_controller.hasHit()) {
        m_lastHasHit = m_controller.hasHit();
        emit hasHitChanged();
    }
}

void RaycastControllerQml::notifyRaycastComplete() {
    bool currentHasHit = m_controller.hasHit();
    QVector3D currentHitPos = m_controller.hitPosition();

    if (m_lastHasHit != currentHasHit) {
        m_lastHasHit = currentHasHit;
        emit hasHitChanged();
    }

    if (currentHasHit && m_lastHitPosition != currentHitPos) {
        m_lastHitPosition = currentHitPos;
        emit hitPositionChanged();
    }
}

