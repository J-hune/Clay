#include "TerrainMesh.h"
#include <cmath>
#include <QImage>
#include <QRgba64>
#include "Log.h"

void TerrainMesh::clear() {
    m_positions.clear();
    m_normals.clear();
    m_indices.clear();
    m_uploaded = false; // on forcera un nouvel upload
}

void TerrainMesh::buildFlat(int rx, int rz) {
    clear();
    if (rx < 2 || rz < 2) return;
    LOG_INFO() << "Construction terrain plat: res=" << rx << "x" << rz;
    m_positions.reserve(rx * rz);
    m_normals.reserve(rx * rz);

    const float halfX = static_cast<float>(rx - 1) * 0.5f;
    const float halfZ = static_cast<float>(rz - 1) * 0.5f;
    for (int z = 0; z < rz; ++z) {
        for (int x = 0; x < rx; ++x) {
            const float px = static_cast<float>(x) - halfX;
            const float pz = static_cast<float>(z) - halfZ;
            m_positions.emplace_back(px, 0.0f, pz);
            m_normals.emplace_back(0.0f, 1.0f, 0.0f);
        }
    }

    // Indices
    const int quadX = rx - 1;
    const int quadZ = rz - 1;
    m_indices.reserve(quadX * quadZ * 6);
    for (int z = 0; z < quadZ; ++z) {
        for (int x = 0; x < quadX; ++x) {
            const unsigned int i0 = static_cast<unsigned int>(z * rx + x);
            const unsigned int i1 = i0 + 1;
            const unsigned int i2 = i0 + rx;
            const unsigned int i3 = i2 + 1;
            m_indices.push_back(i0); m_indices.push_back(i2); m_indices.push_back(i1);
            m_indices.push_back(i1); m_indices.push_back(i2); m_indices.push_back(i3);
        }
    }
}

static float grayFromImageAt(const QImage &img, int x, int y) {
    const int w = img.width();
    const int h = img.height();
    if (x < 0) x = 0; else if (x >= w) x = w - 1;
    if (y < 0) y = 0; else if (y >= h) y = h - 1;
    switch (img.format()) {
        case QImage::Format_Grayscale16: {
            const quint16 *row = reinterpret_cast<const quint16 *>(img.constScanLine(y));
            const quint16 v = row[x];
            return float(v) / 65535.0f;
        }
        case QImage::Format_RGBX64:
        case QImage::Format_RGBA64: {
            const QRgba64 *row = reinterpret_cast<const QRgba64 *>(img.constScanLine(y));
            const QRgba64 p = row[x];
            const float r = float(p.red()) / 65535.0f;
            const float g = float(p.green()) / 65535.0f;
            const float b = float(p.blue()) / 65535.0f;
            return r * 0.299f + g * 0.587f + b * 0.114f;
        }
        default: {
            const QRgb p = img.pixel(x, y);
            const int r = qRed(p), g = qGreen(p), b = qBlue(p);
            return (r * 0.299f + g * 0.587f + b * 0.114f) / 255.f;
        }
    }
}

static float sampleBilinearF32(const QImage &img, float u, float v) {
    const int w = img.width();
    const int h = img.height();
    if (w <= 0 || h <= 0) return 0.f;
    u = std::clamp(u, 0.f, 0.999999f);
    v = std::clamp(v, 0.f, 0.999999f);
    const float x = u * (w - 1);
    const float y = v * (h - 1);
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const int x1 = std::min(x0 + 1, w - 1);
    const int y1 = std::min(y0 + 1, h - 1);
    const float tx = x - x0;
    const float ty = y - y0;
    const float c00 = grayFromImageAt(img, x0, y0);
    const float c10 = grayFromImageAt(img, x1, y0);
    const float c01 = grayFromImageAt(img, x0, y1);
    const float c11 = grayFromImageAt(img, x1, y1);
    const float cx0 = c00 * (1 - tx) + c10 * tx;
    const float cx1 = c01 * (1 - tx) + c11 * tx;
    return cx0 * (1 - ty) + cx1 * ty;
}

void TerrainMesh::buildHeightmap(const QImage &sourceImg, const int rx, const int rz, const float heightScale) {
    LOG_INFO() << "Construction terrain depuis heightmap: res=" << rx << "x" << rz << " scale=" << heightScale;
    clear();
    if (rx < 2 || rz < 2 || sourceImg.isNull()) {
        LOG_WARN() << "Paramètres invalides pour buildHeightmap ou image nulle";
        return;
    }

    const QImage &img = sourceImg; // conserver profondeur si fournie (EXR -> 16-bit)

    m_positions.reserve(rx * rz);
    m_normals.reserve(rx * rz);

    for (int z = 0; z < rz; ++z) {
        const float v = static_cast<float>(z) / static_cast<float>(rz - 1);
        for (int x = 0; x < rx; ++x) {
            const float u = static_cast<float>(x) / static_cast<float>(rx - 1);
            const float hSample = sampleBilinearF32(img, u, v) * heightScale;
            const float px = (u - 0.5f);
            const float pz = (v - 0.5f);
            m_positions.emplace_back(px, hSample, pz);
            m_normals.emplace_back(0.f, 0.f, 0.f);
        }
    }

    // Indices
    const int quadX = rx - 1;
    const int quadZ = rz - 1;
    m_indices.reserve(quadX * quadZ * 6);
    for (int z = 0; z < quadZ; ++z) {
        for (int x = 0; x < quadX; ++x) {
            const unsigned int i0 = static_cast<unsigned int>(z * rx + x);
            const unsigned int i1 = i0 + 1;
            const unsigned int i2 = i0 + rx;
            const unsigned int i3 = i2 + 1;
            m_indices.push_back(i0); m_indices.push_back(i2); m_indices.push_back(i1);
            m_indices.push_back(i1); m_indices.push_back(i2); m_indices.push_back(i3);
        }
    }

    // Normales (gradient centré)
    auto heightAt = [&](int x, int z) -> float {
        x = std::clamp(x, 0, rx - 1);
        z = std::clamp(z, 0, rz - 1);
        return m_positions[z * rx + x].y();
    };

    for (int z = 0; z < rz; ++z) {
        for (int x = 0; x < rx; ++x) {
            const float dx = heightAt(x + 1, z) - heightAt(x - 1, z);
            const float dz = heightAt(x, z + 1) - heightAt(x, z - 1);
            QVector3D n(-dx, 2.0f, -dz);
            n.normalize();
            m_normals[z * rx + x] = n;
        }
    }
}

void TerrainMesh::uploadGL(QOpenGLFunctions *gl) {
    if (!gl || !isValid()) return;
    if (m_uploaded) return;

    if (!m_vboPositions) gl->glGenBuffers(1, &m_vboPositions);
    if (!m_vboNormals) gl->glGenBuffers(1, &m_vboNormals);
    if (!m_ibo) gl->glGenBuffers(1, &m_ibo);

    gl->glBindBuffer(GL_ARRAY_BUFFER, m_vboPositions);
    gl->glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_positions.size() * sizeof(QVector3D)), m_positions.data(), GL_STATIC_DRAW);

    gl->glBindBuffer(GL_ARRAY_BUFFER, m_vboNormals);
    gl->glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_normals.size() * sizeof(QVector3D)), m_normals.data(), GL_STATIC_DRAW);

    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_indices.size() * sizeof(unsigned int)), m_indices.data(), GL_STATIC_DRAW);

    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    m_uploaded = true;
    LOG_INFO() << "Mesh CPU uploadé vers GPU: vtx=" << m_positions.size() << " indices=" << m_indices.size();
}

void TerrainMesh::draw(QOpenGLFunctions *gl) const {
    if (!gl || !isValid() || !m_uploaded) return;

    // Positions -> attrib 0
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_vboPositions);
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Normales -> attrib 1
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_vboNormals);
    gl->glEnableVertexAttribArray(1);
    gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    gl->glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, nullptr);

    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    gl->glDisableVertexAttribArray(1);
    gl->glDisableVertexAttribArray(0);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
}