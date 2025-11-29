#include "BrushListModel.h"

BrushListModel::BrushListModel(QObject *parent)
    : QAbstractListModel(parent) {
}

int BrushListModel::rowCount(const QModelIndex &parent) const {
    return static_cast<int>(m_brushes.size());
}

QVariant BrushListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= static_cast<int>(m_brushes.size()))
        return {};

    const auto &brush = m_brushes[static_cast<size_t>(index.row())];

    switch (role) {
        case BrushIndexRole:
            return brush.id;
        case BrushNameRole:
            return brush.name;
        case BrushFilePathRole:
            return brush.filePath;
        case BrushValidRole:
            return brush.valid;
        default:
            return {};
    }
}

QHash<int, QByteArray> BrushListModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[BrushIndexRole] = "brushIndex";
    roles[BrushNameRole] = "brushName";
    roles[BrushFilePathRole] = "brushFilePath";
    roles[BrushValidRole] = "brushValid";
    return roles;
}

void BrushListModel::updateFromManager(const BrushManager &manager) {
    beginResetModel();
    
    // Filtrer uniquement les brushes valides
    m_brushes.clear();
    const auto &allBrushes = manager.brushes();
    for (const auto &brush : allBrushes) {
        if (brush.valid) {
            m_brushes.push_back(brush);
        }
    }
    
    endResetModel();
}
