#include "UndoRedoManager.h"
#include "Log.h"
#include <algorithm>

void UndoRedoManager::setMaxHistorySize(int size) {
    if (size < 1) size = 1;
    m_maxHistorySize = size;

    // Tronque l'historique si nécessaire
    while (static_cast<int>(m_history.size()) > m_maxHistorySize) {
        m_history.erase(m_history.begin());
        if (m_currentIndex >= 0) {
            m_currentIndex--;
        }
    }
}

HeightmapSnapshot UndoRedoManager::readHeightmap(QOpenGLExtraFunctions *gl, GLuint heightmapTexture, int resolution) {
    if (!gl || heightmapTexture == 0 || resolution <= 0) {
        LOG_ERROR() << "Paramètres invalides pour la lecture de heightmap";
        return HeightmapSnapshot();
    }

    HeightmapSnapshot snapshot(resolution);

    // Crée un framebuffer temporaire pour lire la texture
    GLuint fbo = 0;
    gl->glGenFramebuffers(1, &fbo);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Attache la texture au framebuffer
    gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, heightmapTexture, 0);

    // Vérifie que le framebuffer est complet
    if (gl->glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR() << "Framebuffer incomplet pour la lecture de heightmap";
        gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
        gl->glDeleteFramebuffers(1, &fbo);
        return HeightmapSnapshot();
    }

    // Lit les pixels
    gl->glReadPixels(0, 0, resolution, resolution, GL_RED, GL_FLOAT, snapshot.data.data());

    // Nettoie
    gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gl->glDeleteFramebuffers(1, &fbo);

    return snapshot;
}

void UndoRedoManager::writeHeightmap(QOpenGLExtraFunctions *gl, GLuint heightmapTexture, const HeightmapSnapshot &snapshot) {
    if (!gl || heightmapTexture == 0 || !snapshot.isValid()) {
        LOG_ERROR() << "Paramètres invalides pour l'écriture de heightmap";
        return;
    }

    // Lie la texture et écrit les données
    gl->glBindTexture(GL_TEXTURE_2D, heightmapTexture);
    gl->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        snapshot.resolution, snapshot.resolution,
                        GL_RED, GL_FLOAT, snapshot.data.data());
    gl->glBindTexture(GL_TEXTURE_2D, 0);

    // Force la synchronisation pour être sûr que la texture est mise à jour
    gl->glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
}

void UndoRedoManager::captureSnapshot(QOpenGLExtraFunctions *gl, GLuint heightmapTexture, int resolution) {
    // En mode batch, on ne capture qu'au début
    if (m_batching) {
        return;
    }

    // Lit l'état actuel de la heightmap
    HeightmapSnapshot snapshot = readHeightmap(gl, heightmapTexture, resolution);
    if (!snapshot.isValid()) {
        LOG_ERROR() << "Échec de la capture du snapshot";
        return;
    }

    // Supprime les états "redo" si on crée une nouvelle branche
    invalidateRedoHistory();

    // Ajoute le nouveau snapshot
    m_history.push_back(std::move(snapshot));
    m_currentIndex = static_cast<int>(m_history.size()) - 1;

    // Limite la taille de l'historique
    while (static_cast<int>(m_history.size()) > m_maxHistorySize) {
        m_history.erase(m_history.begin());
        m_currentIndex--;
    }

    LOG_DEBUG() << "Snapshot capturé - Index: " << m_currentIndex
                << ", Historique: " << m_history.size()
                << ", Mémoire: " << (memoryUsage() / 1024 / 1024) << " MB";
}

void UndoRedoManager::invalidateRedoHistory() {
    if (m_currentIndex >= 0 && m_currentIndex < static_cast<int>(m_history.size()) - 1) {
        // Supprime tous les états après l'index actuel
        m_history.erase(m_history.begin() + m_currentIndex + 1, m_history.end());
    }
}

bool UndoRedoManager::undo(QOpenGLExtraFunctions *gl, GLuint heightmapTexture) {
    if (!canUndo()) {
        LOG_WARN() << "Undo impossible - pas d'historique disponible";
        return false;
    }

    // Retourne à l'état précédent
    m_currentIndex--;

    const HeightmapSnapshot &snapshot = m_history[m_currentIndex];
    writeHeightmap(gl, heightmapTexture, snapshot);

    LOG_DEBUG() << "Undo effectué - Index: " << m_currentIndex;
    return true;
}

bool UndoRedoManager::redo(QOpenGLExtraFunctions *gl, GLuint heightmapTexture) {
    if (!canRedo()) {
        LOG_WARN() << "Redo impossible - pas d'état futur disponible";
        return false;
    }

    // Avance vers l'état suivant
    m_currentIndex++;

    const HeightmapSnapshot &snapshot = m_history[m_currentIndex];
    writeHeightmap(gl, heightmapTexture, snapshot);

    LOG_DEBUG() << "Redo effectué - Index: " << m_currentIndex;
    return true;
}

bool UndoRedoManager::canUndo() const {
    // On peut undo si on a au moins 2 états et qu'on n'est pas au début
    return m_currentIndex > 0 && !m_history.empty();
}

bool UndoRedoManager::canRedo() const {
    // On peut redo si on n'est pas au dernier état
    return m_currentIndex >= 0 && m_currentIndex < static_cast<int>(m_history.size()) - 1;
}

void UndoRedoManager::clear() {
    m_history.clear();
    m_currentIndex = -1;
    m_batching = false;
    m_batchStartSnapshot = HeightmapSnapshot();
    LOG_INFO() << "Historique undo/redo effacé";
}

size_t UndoRedoManager::memoryUsage() const {
    size_t total = 0;
    for (const auto &snapshot : m_history) {
        total += snapshot.memorySize();
    }
    if (m_batchStartSnapshot.isValid()) {
        total += m_batchStartSnapshot.memorySize();
    }
    return total;
}

void UndoRedoManager::beginBatch() {
    if (m_batching) {
        LOG_WARN() << "beginBatch appelé alors qu'un batch est déjà en cours";
        return;
    }

    m_batching = true;

    // Le snapshot de début sera capturé lors du premier appel à captureSnapshot
    // qui sera intercepté par le mode batching
    LOG_DEBUG() << "Début du mode batch pour les opérations groupées";
}

void UndoRedoManager::endBatch() {
    if (!m_batching) {
        LOG_WARN() << "endBatch appelé sans beginBatch préalable";
        return;
    }

    m_batching = false;
    m_batchStartSnapshot = HeightmapSnapshot();

    LOG_DEBUG() << "Fin du mode batch";
}
