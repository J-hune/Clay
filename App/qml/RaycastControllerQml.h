#ifndef CLAYAPP_RAYCASTCONTROLLERQML_H
#define CLAYAPP_RAYCASTCONTROLLERQML_H

#include "../RaycastController.h"

/**
 * @brief Wrapper QObject pour RaycastController, exposable au QML
 * Ne possède PAS son propre RaycastController, mais agit comme proxy vers une instance partagée
 */
class RaycastControllerQml : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool hasHit READ hasHit NOTIFY hasHitChanged)
    Q_PROPERTY(QVector3D hitPosition READ hitPosition NOTIFY hitPositionChanged)
    Q_PROPERTY(QVector2D mouseNDC READ mouseNDC NOTIFY mouseNDCChanged)

public:
    explicit RaycastControllerQml(QObject *parent = nullptr);

    // Injection du RaycastController partagé (appelé par GLRenderer)
    void setSharedController(RaycastController *controller);
    RaycastController* sharedController() const { return m_sharedController; }

    // Getters pour Q_PROPERTY
    bool hasHit() const { return m_lastHasHit; }
    QVector3D hitPosition() const { return m_lastHitPosition; }
    QVector2D mouseNDC() const { return m_mouseNDC; }

    // Méthodes invocables depuis QML
    Q_INVOKABLE void updateMousePosition(const QPointF &localPos, float viewportWidth, float viewportHeight);
    Q_INVOKABLE void reset();

    // Pour notifier les changements après le raycast (appelé par GLRenderer)
    void notifyRaycastComplete(bool hasHit, const QVector3D &hitPosition);

signals:
    void hasHitChanged();
    void hitPositionChanged();
    void mouseNDCChanged();
    void raycastRequested(); // Signal pour indiquer qu'un raycast est nécessaire

private:
    RaycastController *m_sharedController = nullptr;  // Référence partagée
    QVector2D m_mouseNDC{0.f, 0.f};
    bool m_lastHasHit = false;
    QVector3D m_lastHitPosition;
};

#endif // CLAYAPP_RAYCASTCONTROLLERQML_H

