#ifndef CLAYAPP_RENDERSTATE_H
#define CLAYAPP_RENDERSTATE_H

#include <QVector2D>

/**
 * @brief État du rendu - gère les raisons de redraw et l'état du frame
 */
enum class RedrawReason {
    None            = 0,
    TerrainChanged  = 1 << 0,
    GridChanged     = 1 << 1,
    CameraMoved     = 1 << 2,
    RaycastChanged  = 1 << 3,
    BrushChanged    = 1 << 4
};

inline RedrawReason operator|(RedrawReason a, RedrawReason b) {
    return static_cast<RedrawReason>(static_cast<int>(a) | static_cast<int>(b));
}

inline RedrawReason& operator|=(RedrawReason &a, RedrawReason b) {
    a = a | b;
    return a;
}

inline bool operator&(RedrawReason a, RedrawReason b) {
    return (static_cast<int>(a) & static_cast<int>(b)) != 0;
}

/**
 * @brief État global du rendu pour un frame
 */
struct RenderState {
    // Flags d'affichage
    bool drawGrid = true;
    bool drawAxes = true;

    // État du terrain
    bool terrainReady = false;
    int lastTerrainRevision = -1;

    // État de la grille (pour détecter les changements)
    int prevGridResolution = -1;
    bool prevDrawGrid = true;
    bool prevDrawAxes = true;

    // État de la souris
    bool mouseMoved = false;
    QVector2D lastMouseNDC{0.f, 0.f};

    // État du brush (pour détecter le début/fin d'application)
    bool wasBrushActive = false;

    // FPS
    float fpsAccum = -1.f;

    // Raisons de redraw
    RedrawReason redrawReasons = RedrawReason::None;

    void requestRedraw(RedrawReason reason) {
        redrawReasons |= reason;
    }

    void clearRedrawReasons() {
        redrawReasons = RedrawReason::None;
    }

    bool shouldRedraw() const {
        return redrawReasons != RedrawReason::None;
    }
};

#endif // CLAYAPP_RENDERSTATE_H
