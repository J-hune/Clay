#ifndef CLAYAPP_BRUSHMANAGERQML_H
#define CLAYAPP_BRUSHMANAGERQML_H

#include "../BrushManager.h"

/**
 * @brief Wrapper QObject pour BrushManager, exposable au QML
 * Gère l'état des brushes et expose les propriétés
 */
class BrushManagerQml : public QObject {
    Q_OBJECT

    Q_PROPERTY(int brushIndex READ brushIndex WRITE setBrushIndex NOTIFY brushIndexChanged)
    Q_PROPERTY(float brushSize READ brushSize WRITE setBrushSize NOTIFY brushSizeChanged)
    Q_PROPERTY(float brushStrength READ brushStrength WRITE setBrushStrength NOTIFY brushStrengthChanged)
    Q_PROPERTY(int brushCount READ brushCount NOTIFY brushCountChanged)

public:
    explicit BrushManagerQml(QObject *parent = nullptr);

    // Accès au manager interne (pour GLRenderer)
    BrushManager& manager() { return m_manager; }
    const BrushManager& manager() const { return m_manager; }

    // Getters pour Q_PROPERTY
    int brushIndex() const { return m_manager.currentBrushIndex(); }
    float brushSize() const { return m_manager.brushSize(); }
    float brushStrength() const { return m_manager.brushStrength(); }
    int brushCount() const { return m_manager.brushCount(); }

    // Setters
    void setBrushIndex(int index);
    void setBrushSize(float size);
    void setBrushStrength(float strength);

    // Méthodes invocables depuis QML
    Q_INVOKABLE void enqueueStroke(const QVector3D &worldPos);

signals:
    void brushIndexChanged();
    void brushSizeChanged();
    void brushStrengthChanged();
    void brushCountChanged();

private:
    BrushManager m_manager;
};

#endif // CLAYAPP_BRUSHMANAGERQML_H
