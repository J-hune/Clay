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
            emit mouseNDCChanged();
            emit raycastRequested();
        }
    }
}

void RaycastControllerQml::reset() {
    if (m_lastHasHit) {
        m_lastHasHit = false;
        m_lastHitPosition = QVector3D();
        emit hasHitChanged();
        emit hitPositionChanged();
    }
}

void RaycastControllerQml::notifyRaycastComplete(bool hasHit, const QVector3D &hitPosition) {
    if (m_lastHasHit != hasHit) {
        m_lastHasHit = hasHit;
        emit hasHitChanged();
    }

    if (hasHit && m_lastHitPosition != hitPosition) {
        m_lastHitPosition = hitPosition;
        emit hitPositionChanged();
    } else if (!hasHit) {
        // Reset position si pas de hit
        m_lastHitPosition = QVector3D();
        emit hitPositionChanged();
    }
}

