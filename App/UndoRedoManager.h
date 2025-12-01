#ifndef CLAYAPP_UNDOREDOMANAGER_H
#define CLAYAPP_UNDOREDOMANAGER_H

#include <vector>
#include <QOpenGLExtraFunctions>

/**
 * @brief Snapshot d'une heightmap pour undo/redo
 * Stocke les données de la heightmap au format float (R32F)
 */
struct HeightmapSnapshot {
    std::vector<float> data;
    int resolution = 0;

    HeightmapSnapshot() = default;
    HeightmapSnapshot(int res) : resolution(res) {
        data.resize(res * res);
    }

    bool isValid() const {
        return resolution > 0 && !data.empty();
    }

    size_t memorySize() const {
        return data.size() * sizeof(float);
    }
};

/**
 * @brief Gestionnaire d'undo/redo pour les opérations de terrain
 * Capture des snapshots de la heightmap avant chaque modification
 */
class UndoRedoManager {
public:
    UndoRedoManager() = default;
    ~UndoRedoManager() = default;

    /**
     * @brief Configure le nombre maximal d'étapes dans l'historique
     * Par défaut: 50 étapes
     */
    void setMaxHistorySize(int size);
    int maxHistorySize() const { return m_maxHistorySize; }

    /**
     * @brief Sauvegarde l'état actuel de la heightmap
     * @param gl Contexte OpenGL
     * @param heightmapTexture ID de la texture heightmap
     * @param resolution Résolution de la heightmap
     */
    void captureSnapshot(QOpenGLExtraFunctions *gl, GLuint heightmapTexture, int resolution);

    /**
     * @brief Restaure l'état précédent (undo)
     * @param gl Contexte OpenGL
     * @param heightmapTexture ID de la texture heightmap
     * @return true si l'opération a réussi, false sinon
     */
    bool undo(QOpenGLExtraFunctions *gl, GLuint heightmapTexture);

    /**
     * @brief Refait l'état annulé (redo)
     * @param gl Contexte OpenGL
     * @param heightmapTexture ID de la texture heightmap
     * @return true si l'opération a réussi, false sinon
     */
    bool redo(QOpenGLExtraFunctions *gl, GLuint heightmapTexture);

    /**
     * @brief Vérifie si un undo est possible
     */
    bool canUndo() const;

    /**
     * @brief Vérifie si un redo est possible
     */
    bool canRedo() const;

    /**
     * @brief Réinitialise l'historique complet
     */
    void clear();

    /**
     * @brief Retourne le nombre d'étapes dans l'historique
     */
    int historySize() const { return static_cast<int>(m_history.size()); }

    /**
     * @brief Retourne l'index actuel dans l'historique
     */
    int currentIndex() const { return m_currentIndex; }

    /**
     * @brief Retourne la mémoire utilisée par l'historique (en bytes)
     */
    size_t memoryUsage() const;

    /**
     * @brief Active/désactive le mode "batch" pour regrouper plusieurs opérations
     * Utile pour les opérations continues (brush drag)
     */
    void beginBatch();
    void endBatch();
    bool isBatching() const { return m_batching; }

private:
    /**
     * @brief Lit les données de la heightmap depuis le GPU
     */
    HeightmapSnapshot readHeightmap(QOpenGLExtraFunctions *gl, GLuint heightmapTexture, int resolution);

    /**
     * @brief Écrit les données de la heightmap vers le GPU
     */
    void writeHeightmap(QOpenGLExtraFunctions *gl, GLuint heightmapTexture, const HeightmapSnapshot &snapshot);

    /**
     * @brief Supprime les états "redo" après une nouvelle capture
     */
    void invalidateRedoHistory();

    std::vector<HeightmapSnapshot> m_history;
    int m_currentIndex = -1;  // Index de l'état actuel (-1 = aucun historique)
    int m_maxHistorySize = 50;
    bool m_batching = false;
    HeightmapSnapshot m_batchStartSnapshot;  // Snapshot au début du batch
};

#endif // CLAYAPP_UNDOREDOMANAGER_H
