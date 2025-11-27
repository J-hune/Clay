#ifndef CLAYAPP_TERRAINBUILDER_H
#define CLAYAPP_TERRAINBUILDER_H

#include <QUrl>
#include <QImage>

class TerrainMesh;

class TerrainBuilder {
public:
    static void buildFlat(TerrainMesh &mesh, int resX, int resZ);
    static void buildFromHeightmap(TerrainMesh &mesh, const QImage &heightmap, int resX, int resZ, float heightScale);
    static QImage loadHeightmapFromUrl(const QUrl &url);
};

#endif // CLAYAPP_TERRAINBUILDER_H
