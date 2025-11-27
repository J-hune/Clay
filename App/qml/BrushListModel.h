#ifndef CLAYAPP_BRUSHLISTMODEL_H
#define CLAYAPP_BRUSHLISTMODEL_H

#include <QAbstractListModel>
#include "../BrushManager.h"

/**
 * @brief Modèle Qt pour exposer la liste des brushes au QML
 * Permet d'afficher les brushes dans un GridView ou ListView
 */
class BrushListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum BrushRoles {
        BrushIndexRole = Qt::UserRole + 1,
        BrushNameRole,
        BrushFilePathRole,
        BrushValidRole
    };

    explicit BrushListModel(QObject *parent = nullptr);

    // Interface QAbstractListModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Méthode pour mettre à jour depuis BrushManager
    void updateFromManager(const BrushManager &manager);

private:
    std::vector<BrushDescriptor> m_brushes;
};

#endif // CLAYAPP_BRUSHLISTMODEL_H

