#ifndef CLAYAPP_RAYCASTCONTROLLERQML_H
#define CLAYAPP_RAYCASTCONTROLLERQML_H

#include "../RaycastController.h"

/**
 * @brief Wrapper QObject pour RaycastController, exposable au QML
 * Gère le raycast GPU et expose les résultats
 */
class RaycastControllerQml : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool hasHit READ hasHit NOTIFY hasHitChanged)
    Q_PROPERTY(QVector3D hitPosition READ hitPosition NOTIFY hitPositionChanged)
    Q_PROPERTY(QVector2D mouseNDC READ mouseNDC NOTIFY mouseNDCChanged)

public:
    explicit RaycastControllerQml(QObject *parent = nullptr);

    // Accès au contrôleur interne (pour GLRenderer)
    RaycastController& controller() { return m_controller; }
    const RaycastController& controller() const { return m_controller; }

    // Getters pour Q_PROPERTY
    bool hasHit() const { return m_controller.hasHit(); }
    QVector3D hitPosition() const { return m_controller.hitPosition(); }
    QVector2D mouseNDC() const { return m_mouseNDC; }

    // Méthodes invocables depuis QML
    Q_INVOKABLE void updateMousePosition(const QPointF &localPos, float viewportWidth, float viewportHeight);
    Q_INVOKABLE void reset();

    // Pour notifier les changements après le raycast (appelé par GLRenderer)
    void notifyRaycastComplete();

signals:
    void hasHitChanged();
    void hitPositionChanged();
    void mouseNDCChanged();
    void raycastRequested(); // Signal pour indiquer qu'un raycast est nécessaire

private:
    RaycastController m_controller;
    QVector2D m_mouseNDC{0.f, 0.f};
    bool m_lastHasHit = false;
    QVector3D m_lastHitPosition;
};

#endif // CLAYAPP_RAYCASTCONTROLLERQML_H

