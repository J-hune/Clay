#include "TerrainManagerQml.h"
#include "../TerrainBuilder.h"
#include "../Log.h"

TerrainManagerQml::TerrainManagerQml(QObject *parent)
    : QObject(parent) {
}

int TerrainManagerQml::triangleCount() const {
    if (!m_ready) return 0;
    return (m_resolution - 1) * (m_resolution - 1) * 2;
}

void TerrainManagerQml::setResolution(int r) {
    if (r < 2) r = 2;
    if (r == m_resolution) return;
    m_resolution = r;
    emit resolutionChanged();
}

void TerrainManagerQml::setHeightmapResolution(int r) {
    constexpr int allowed[4] = {512, 1024, 2048, 4096};
    int chosen = 512;
    for (const int v: allowed) {
        if (r == v) {
            chosen = v;
            break;
        }
    }
    if (chosen == m_heightmapResolution) return;
    m_heightmapResolution = chosen;
    ++m_revision;
    emit heightmapResolutionChanged();
}

void TerrainManagerQml::setHeightmapSource(const QUrl &url) {
    if (url == m_heightmapSource) return;
    m_heightmapSource = url;
    emit heightmapSourceChanged();
}

void TerrainManagerQml::setHeightScale(float s) {
    if (s < 0.f) s = 0.f;
    if (qFuzzyCompare(s, m_heightScale)) return;
    m_heightScale = s;
    emit heightScaleChanged();
}

void TerrainManagerQml::setMode(int m) {
    if (m == m_mode) return;
    if (m < 0 || m > 1) return;
    m_mode = m;
    emit modeChanged();
}

void TerrainManagerQml::generate() {
    LOG_INFO() << "Génération du terrain - Mode:" << m_mode << " Resolution:" << m_resolution;

    bool wasReady = m_ready;
    m_ready = false;

    if (m_mode == 0) {
        // Mode flat
        TerrainBuilder::buildFlat(m_mesh, m_resolution, m_resolution);
    } else if (m_mode == 1) {
        // Mode heightmap
        const QImage img = TerrainBuilder::loadHeightmapFromUrl(m_heightmapSource);
        if (img.isNull()) {
            LOG_WARN() << "Impossible de charger l'image heightmap:" << m_heightmapSource.toString().toStdString();
            if (wasReady != m_ready) {
                emit readyChanged();
            }
            return;
        }
        TerrainBuilder::buildFromHeightmap(m_mesh, img, m_resolution, m_resolution, m_heightScale);
    }

    m_ready = m_mesh.isValid();
    ++m_revision;
    m_needsUpload = true;

    if (wasReady != m_ready) {
        emit readyChanged();
    }
    emit triangleCountChanged();
    emit terrainGenerated();

    LOG_INFO() << "Terrain généré - Ready:" << m_ready << " Triangles:" << triangleCount();
}
