#ifndef CLAYAPP_TERRAINMESH_H
#define CLAYAPP_TERRAINMESH_H

#include <vector>
#include <QVector3D>
#include <QOpenGLFunctions>

// Maillage de terrain (MVP): génération d'un terrain plat
class TerrainMesh {
public:
    TerrainMesh() = default;

    void clear();
    bool isValid() const { return !m_positions.empty() && !m_indices.empty(); }

    // Construction d'un terrain plat de résolution (rx * rz)
    // Hauteur uniforme = 0, normales (0,1,0)
    void buildFlat(int rx, int rz);
    void buildHeightmap(const QImage &img, int rx, int rz, float heightScale); // nouvelle construction

    int vertexCount() const { return static_cast<int>(m_positions.size()); }
    int triangleCount() const { return static_cast<int>(m_indices.size() / 3); }

    // Chargement dans des VBO (positions + normales) + IBO indices
    void uploadGL(QOpenGLFunctions *gl);
    void draw(QOpenGLFunctions *gl) const;

private:
    std::vector<QVector3D> m_positions;
    std::vector<QVector3D> m_normals; // même taille que positions
    std::vector<unsigned int> m_indices; // triangles (3 indices)

    GLuint m_vboPositions = 0;
    GLuint m_vboNormals = 0;
    GLuint m_ibo = 0;
    bool m_uploaded = false;
};

#endif // CLAYAPP_TERRAINMESH_H