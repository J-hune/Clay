#include "BrushManagerQml.h"

#include <QUrl>
#include <QDir>
#include <QDesktopServices>

#include "App/Log.h"

BrushManagerQml::BrushManagerQml(QObject *parent)
    : QObject(parent) {
}

void BrushManagerQml::setSharedManager(BrushManager *manager) {
    m_sharedManager = manager;
}

int BrushManagerQml::brushIndex() const {
    return m_sharedManager ? m_sharedManager->currentBrushIndex() : 0;
}

float BrushManagerQml::brushSize() const {
    return m_sharedManager ? m_sharedManager->brushSize() : 2.0f;
}

float BrushManagerQml::brushStrength() const {
    return m_sharedManager ? m_sharedManager->brushStrength() : 1.0f;
}

int BrushManagerQml::brushOperation() const {
    return m_sharedManager ? static_cast<int>(m_sharedManager->operation()) : 0;
}

void BrushManagerQml::setBrushIndex(int index) {
    if (!m_sharedManager) return;
    if (m_sharedManager->currentBrushIndex() == index) return;
    m_sharedManager->setCurrentBrushIndex(index);
    emit brushIndexChanged();
}

void BrushManagerQml::setBrushSize(float size) {
    if (!m_sharedManager) return;
    if (qFuzzyCompare(m_sharedManager->brushSize(), size)) return;
    m_sharedManager->setBrushSize(size);
    emit brushSizeChanged();
}

void BrushManagerQml::setBrushStrength(float strength) {
    if (!m_sharedManager) return;
    if (qFuzzyCompare(m_sharedManager->brushStrength(), strength)) return;
    m_sharedManager->setBrushStrength(strength);
    emit brushStrengthChanged();
}

void BrushManagerQml::setBrushOperation(int op) {
    if (!m_sharedManager) return;
    if (static_cast<int>(m_sharedManager->operation()) == op) return;
    m_sharedManager->setOperation(static_cast<BrushOpType>(op));
    emit brushOperationChanged();
}

void BrushManagerQml::enqueueStroke(const QVector3D &worldPos) {
    if (!m_sharedManager) return;
    m_sharedManager->enqueueStroke(worldPos);
}

void BrushManagerQml::requestAddBrush(const QString &filePath) {
    if (!m_sharedManager || filePath.isEmpty()) return;

    QString fp = filePath;
    // Si QML renvoie un URL file://, on convertit en chemin local
    if (fp.startsWith("file://")) {
        fp = QUrl(fp).toLocalFile();
    }

    int newIndex = m_sharedManager->addBrushFromExternalFile(fp);
    if (newIndex >= 0) {
        // Mise à jour du modèle pour refléter le nouveau brush
        m_brushModel.updateFromManager(*m_sharedManager);

        // Sélectionner le nouveau brush
        m_sharedManager->setCurrentBrushIndex(newIndex);

        // Émettre les signaux pour mettre à jour l'interface
        emit brushCountChanged();
        emit brushModelChanged();
        emit brushIndexChanged();

        LOG_INFO() << "Brush ajouté avec succès à l'index " << newIndex;
    } else {
        LOG_ERROR() << "Échec de l'ajout du brush: " << fp.toStdString();
    }
}

void BrushManagerQml::requestRemoveBrush(int index) {
    if (!m_sharedManager || !m_sharedManager->isValidBrushIndex(index)) return;

    // Supprimer le brush et son fichier
    m_sharedManager->removeBrushAndFile(index);

    // Mise à jour du modèle
    m_brushModel.updateFromManager(*m_sharedManager);
    emit brushCountChanged();
    emit brushModelChanged();

    // Si le brush supprimé était sélectionné, sélectionner le premier brush valide
    if (m_sharedManager->currentBrushIndex() == index) {
        int newIndex = 0;
        if (m_sharedManager->brushCount() > 0 && !m_sharedManager->isValidBrushIndex(0)) {
            // Chercher le premier brush valide
            for (int i = 0; i < m_sharedManager->brushCount(); ++i) {
                if (m_sharedManager->isValidBrushIndex(i)) {
                    newIndex = i;
                    break;
                }
            }
        }
        m_sharedManager->setCurrentBrushIndex(newIndex);
        emit brushIndexChanged();
    }
}

void BrushManagerQml::requestShowBrushInFolder(int index) {
    if (!m_sharedManager || !m_sharedManager->isValidBrushIndex(index)) {
        LOG_ERROR() << "Index de brush invalide: " << index;
        return;
    }

    const auto &brushes = m_sharedManager->brushes();
    const QString filePath = brushes[static_cast<size_t>(index)].filePath;

    if (filePath.isEmpty() || !QFile::exists(filePath)) {
        LOG_ERROR() << "Le fichier de brush n'existe pas: " << filePath.toStdString();
        return;
    }

    // Ouvrir le dossier contenant le fichier dans l'explorateur système
    QFileInfo fileInfo(filePath);
    QString dirPath = fileInfo.absolutePath();

    QUrl url = QUrl::fromLocalFile(dirPath);
    QDesktopServices::openUrl(url);
}

void BrushManagerQml::refreshBrushModel() {
    if (!m_sharedManager) return;

    m_brushModel.updateFromManager(*m_sharedManager);
    emit brushCountChanged();
    emit brushModelChanged();
}

void BrushManagerQml::notifyLoadingProgress() {
    emit loadingProgressChanged();
}
