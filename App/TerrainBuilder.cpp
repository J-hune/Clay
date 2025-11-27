#include "TerrainBuilder.h"
#include "TerrainMesh.h"
#include "ImageUtils.h"
#include "Log.h"

void TerrainBuilder::buildFlat(TerrainMesh &mesh, int resX, int resZ) {
    mesh.clear();
    mesh.buildFlat(resX, resZ);
    LOG_INFO() << "Terrain plat créé - résolution: " << resX << "x" << resZ;
}

void TerrainBuilder::buildFromHeightmap(TerrainMesh &mesh, const QImage &heightmap, int resX, int resZ, float heightScale) {
    mesh.clear();

    if (heightmap.isNull()) {
        LOG_WARN() << "Heightmap invalide, création d'un terrain plat";
        mesh.buildFlat(resX, resZ);
        return;
    }

    mesh.buildHeightmap(heightmap, resX, resZ, heightScale);
    LOG_INFO() << "Terrain créé depuis heightmap - résolution: " << resX << "x" << resZ << ", scale: " << heightScale;
}

QImage TerrainBuilder::loadHeightmapFromUrl(const QUrl &url) {
    if (url.isEmpty()) {
        LOG_WARN() << "URL de heightmap vide";
        return QImage();
    }

    const QImage img = loadHeightImage(url);
    if (img.isNull()) {
        LOG_WARN() << "Impossible de charger la heightmap depuis: " << url.toString().toStdString();
    } else {
        LOG_INFO() << "Heightmap chargée avec succès: " << url.toString().toStdString();
    }

    return img;
}
