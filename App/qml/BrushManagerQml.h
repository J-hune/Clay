#ifndef CLAYAPP_BRUSHMANAGERQML_H
#define CLAYAPP_BRUSHMANAGERQML_H

#include "../BrushManager.h"
#include "BrushListModel.h"

/**
 * @brief Wrapper QObject pour BrushManager, exposable au QML
 * Gère l'état des brushes et expose les propriétés
 */
class BrushManagerQml : public QObject {
    Q_OBJECT

    Q_PROPERTY(int brushIndex READ brushIndex WRITE setBrushIndex NOTIFY brushIndexChanged)
    Q_PROPERTY(float brushSize READ brushSize WRITE setBrushSize NOTIFY brushSizeChanged)
    Q_PROPERTY(float brushStrength READ brushStrength WRITE setBrushStrength NOTIFY brushStrengthChanged)
    Q_PROPERTY(int brushOperation READ brushOperation WRITE setBrushOperation NOTIFY brushOperationChanged)
    Q_PROPERTY(int brushCount READ brushCount NOTIFY brushCountChanged)
    Q_PROPERTY(BrushListModel* brushModel READ brushModel NOTIFY brushModelChanged)

public:
    explicit BrushManagerQml(QObject *parent = nullptr);

    // Accès au manager interne (pour GLRenderer)
    BrushManager& manager() { return m_manager; }
    const BrushManager& manager() const { return m_manager; }

    // Getters pour Q_PROPERTY
    int brushIndex() const { return m_manager.currentBrushIndex(); }
    float brushSize() const { return m_manager.brushSize(); }
    float brushStrength() const { return m_manager.brushStrength(); }
    int brushOperation() const { return static_cast<int>(m_manager.operation()); }
    int brushCount() const { return m_brushModel.rowCount(); }
    BrushListModel* brushModel() { return &m_brushModel; }

    // Setters
    void setBrushIndex(int index);
    void setBrushSize(float size);
    void setBrushStrength(float strength);
    void setBrushOperation(int op);

    // Méthodes invocables depuis QML
    Q_INVOKABLE void enqueueStroke(const QVector3D &worldPos);

    // Appelé par GLRenderer après loadFromDirectory pour mettre à jour le modèle
    void refreshBrushModel(const BrushManager &manager);

signals:
    void brushIndexChanged();
    void brushSizeChanged();
    void brushStrengthChanged();
    void brushOperationChanged();
    void brushCountChanged();
    void brushModelChanged();

private:
    BrushManager m_manager;
    BrushListModel m_brushModel;
};

#endif // CLAYAPP_BRUSHMANAGERQML_H
