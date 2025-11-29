#ifndef CLAYAPP_BRUSHMANAGERQML_H
#define CLAYAPP_BRUSHMANAGERQML_H

#include "../BrushManager.h"
#include "BrushListModel.h"
#include <QList>

/**
 * @brief Wrapper QObject pour BrushManager, exposable au QML
 * Ne possède PAS son propre BrushManager, mais agit comme proxy vers une instance partagée
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

    // Injection du BrushManager partagé (appelé par GLRenderer)
    void setSharedManager(BrushManager *manager);
    BrushManager* sharedManager() const { return m_sharedManager; }

    // Getters pour Q_PROPERTY
    int brushIndex() const;
    float brushSize() const;
    float brushStrength() const;
    int brushOperation() const;
    int brushCount() const { return m_brushModel.rowCount(); }
    BrushListModel* brushModel() { return &m_brushModel; }

    // Setters
    void setBrushIndex(int index);
    void setBrushSize(float size);
    void setBrushStrength(float strength);
    void setBrushOperation(int op);

    // Méthodes invocables depuis QML
    Q_INVOKABLE void enqueueStroke(const QVector3D &worldPos);

    // Ajout / suppression depuis l'UI (QML)
    Q_INVOKABLE void requestAddBrush(const QString &filePath);
    Q_INVOKABLE void requestRemoveBrush(int index);
    Q_INVOKABLE void requestShowBrushInFolder(int index);

    // Appelé par GLRenderer après loadFromDirectory pour mettre à jour le modèle
    void refreshBrushModel();

signals:
    void brushIndexChanged();
    void brushSizeChanged();
    void brushStrengthChanged();
    void brushOperationChanged();
    void brushCountChanged();
    void brushModelChanged();

private:
    BrushManager *m_sharedManager = nullptr;
    BrushListModel m_brushModel;
};

#endif // CLAYAPP_BRUSHMANAGERQML_H
