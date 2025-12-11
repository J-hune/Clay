#include "TerrainGpu.h"
#include <QRgba64>
#include <algorithm>
#include <QFile>
#include <QCoreApplication>
#include <QDir>
#include "Log.h"

static QString loadShaderSource(const QString &filename) {
    const QString exeDir = QCoreApplication::applicationDirPath();
    const QString shaderPath = QDir(exeDir).filePath(QStringLiteral("shaders/%1").arg(filename));

    QFile file(shaderPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR() << "Impossible d'ouvrir le fichier shader: " << shaderPath.toStdString();
        return QString();
    }

    QString source = QString::fromUtf8(file.readAll());
    LOG_INFO() << "Shader chargé: " << shaderPath.toStdString();
    return source;
}

void TerrainGpu::ensureProgram(const QOpenGLFunctions *gl) {
    Q_UNUSED(gl);
    if (m_program && m_program->isLinked()) return;

    m_program.reset(new QOpenGLShaderProgram());

    // Charger le vertex shader
    QString vertexSource = loadShaderSource("terrain.vert");
    if (vertexSource.isEmpty()) {
        LOG_ERROR() << "Impossible de charger terrain.vert";
        m_program.reset();
        return;
    }

    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexSource)) {
        LOG_ERROR() << "Échec compilation Vertex Shader terrain: " << m_program->log().toStdString();
        m_program.reset();
        return;
    }

    // Charger le fragment shader
    QString fragmentSource = loadShaderSource("terrain.frag");
    if (fragmentSource.isEmpty()) {
        LOG_ERROR() << "Impossible de charger terrain.frag";
        m_program.reset();
        return;
    }

    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentSource)) {
        LOG_ERROR() << "Échec compilation Fragment Shader terrain: " << m_program->log().toStdString();
        m_program.reset();
        return;
    }

    m_program->bindAttributeLocation("aUV", 0);

    if (!m_program->link()) {
        LOG_ERROR() << "Échec link du programme shader terrain: " << m_program->log().toStdString();
        m_program.reset();
        return;
    }

    LOG_INFO() << "Shaders terrain compilés et liés";
}

void TerrainGpu::ensureMesh(QOpenGLFunctions *gl) {
    if (!m_dirtyMesh && m_vbo && m_ibo) return;

    // Génère une grille de m_gridResX * m_gridResZ avec UV (0..1)
    const int rx = m_gridResX;
    const int rz = m_gridResZ;
    std::vector<float> uvs; uvs.reserve(rx * rz * 2);
    for (int z = 0; z < rz; ++z) {
        float v = (rz <= 1) ? 0.f : float(z) / float(rz - 1);
        for (int x = 0; x < rx; ++x) {
            float u = (rx <= 1) ? 0.f : float(x) / float(rx - 1);
            uvs.push_back(u);
            uvs.push_back(v);
        }
    }
    m_indices.clear();
    m_indices.reserve((rx - 1) * (rz - 1) * 6);
    for (int z = 0; z < rz - 1; ++z) {
        for (int x = 0; x < rx - 1; ++x) {
            unsigned int i0 = z * rx + x;
            unsigned int i1 = i0 + 1;
            unsigned int i2 = i0 + rx;
            unsigned int i3 = i2 + 1;
            m_indices.push_back(i0);
            m_indices.push_back(i2);
            m_indices.push_back(i1);
            m_indices.push_back(i1);
            m_indices.push_back(i2);
            m_indices.push_back(i3);
        }
    }

    if (!m_vbo) gl->glGenBuffers(1, &m_vbo);
    if (!m_ibo) gl->glGenBuffers(1, &m_ibo);

    gl->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    gl->glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(uvs.size() * sizeof(float)), uvs.data(), GL_STATIC_DRAW);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);

    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(m_indices.size() * sizeof(unsigned int)), m_indices.data(), GL_STATIC_DRAW);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    m_dirtyMesh = false;
    LOG_DEBUG() << "Maillage terrain (VBO/IBO) prêt: resX=" << m_gridResX << " resZ=" << m_gridResZ
                << " indices=" << m_indices.size();
}

void TerrainGpu::ensureTexture(QOpenGLFunctions *gl) {
    if (!m_tex) {
        gl->glGenTextures(1, &m_tex);
        gl->glBindTexture(GL_TEXTURE_2D, m_tex);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl->glBindTexture(GL_TEXTURE_2D, 0);
        m_dirtyTexture = true;
        LOG_INFO() << "Texture heightmap allouée (id=" << m_tex << ")";
    }
    if (m_dirtyTexture) {
        gl->glBindTexture(GL_TEXTURE_2D, m_tex);
        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_texRes, m_texRes, 0, GL_RED, GL_FLOAT, nullptr);
        gl->glBindTexture(GL_TEXTURE_2D, 0);
        m_dirtyTexture = false;
        LOG_INFO() << "Texture heightmap (R32F) initialisée: " << m_texRes << "x" << m_texRes;
    }
}

void TerrainGpu::uploadHeightData(QOpenGLFunctions *gl, const float *data, int width, int height) {
    ensureTexture(gl);
    gl->glBindTexture(GL_TEXTURE_2D, m_tex);
    gl->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_FLOAT, data);
    gl->glBindTexture(GL_TEXTURE_2D, 0);
    m_haveTextureData = true;
    LOG_INFO() << "Heightmap uploadée: " << width << "x" << height;
}

void TerrainGpu::initialize(QOpenGLFunctions *gl) {
    ensureProgram(gl);
    ensureMesh(gl);
    ensureTexture(gl);
}

static float grayFromImageAt(const QImage &img, int x, int y) {
    const int w = img.width();
    const int h = img.height();
    if (x < 0) x = 0; else if (x >= w) x = w - 1;
    if (y < 0) y = 0; else if (y >= h) y = h - 1;
    switch (img.format()) {
    case QImage::Format_Grayscale16: {
        const quint16 *row = reinterpret_cast<const quint16*>(img.constScanLine(y));
        const quint16 v = row[x];
        return float(v) / 65535.0f;
    }
    case QImage::Format_RGBX64:
    case QImage::Format_RGBA64: {
        const QRgba64 *row = reinterpret_cast<const QRgba64*>(img.constScanLine(y));
        const QRgba64 p = row[x];
        const float r = float(p.red())   / 65535.0f;
        const float g = float(p.green()) / 65535.0f;
        const float b = float(p.blue())  / 65535.0f;
        return r * 0.299f + g * 0.587f + b * 0.114f;
    }
    default: {
        // fallback 8-bit
        const QRgb p = img.pixel(x, y);
        const int r = qRed(p), g = qGreen(p), b = qBlue(p);
        return (r*0.299f + g*0.587f + b*0.114f) / 255.f;
    }
    }
}

static float sampleBilinearF32(const QImage &img, float u, float v) {
    const int w = img.width();
    const int h = img.height();
    if (w <= 0 || h <= 0) return 0.f;
    u = std::min(std::max(u, 0.f), 0.999999f);
    v = std::min(std::max(v, 0.f), 0.999999f);
    const float x = u * (w - 1);
    const float y = v * (h - 1);
    const int x0 = int(std::floor(x));
    const int y0 = int(std::floor(y));
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

void TerrainGpu::rebuild(QOpenGLFunctions *gl, const QImage &sourceImg, float heightScale) {
    ensureProgram(gl);
    ensureMesh(gl);
    ensureTexture(gl);
    m_heightScale = heightScale;

    // Conserver la profondeur si fournie (EXR via plugin peut donner Grayscale16 ou RGBA64)
    const QImage &img = sourceImg;

    std::vector<float> data; data.resize(size_t(m_texRes) * size_t(m_texRes));
    for (int y = 0; y < m_texRes; ++y) {
        const float v = (m_texRes <= 1) ? 0.f : float(y) / float(m_texRes - 1);
        for (int x = 0; x < m_texRes; ++x) {
            const float u = (m_texRes <= 1) ? 0.f : float(x) / float(m_texRes - 1);
            data[size_t(y) * size_t(m_texRes) + size_t(x)] = sampleBilinearF32(img, u, v);
        }
    }
    uploadHeightData(gl, data.data(), m_texRes, m_texRes);
    LOG_INFO() << "Terrain reconstruit depuis heightmap, scale=" << m_heightScale;
}

void TerrainGpu::rebuildFlat(QOpenGLFunctions *gl, float heightScale) {
    ensureProgram(gl);
    ensureMesh(gl);
    ensureTexture(gl);
    m_heightScale = heightScale;
    std::vector<float> data; data.assign(size_t(m_texRes) * size_t(m_texRes), 0.f);
    uploadHeightData(gl, data.data(), m_texRes, m_texRes);
    LOG_INFO() << "Terrain plat reconstruit, scale=" << m_heightScale;
}

void TerrainGpu::draw(QOpenGLFunctions *gl, const QMatrix4x4 &proj, const QMatrix4x4 &view, const QVector3D &cameraForward) {
    if (!m_program || !m_program->isLinked() || !m_haveTextureData) return;
    ensureMesh(gl);

    m_program->bind();
    const QMatrix4x4 mvp = proj * view;
    m_program->setUniformValue("uMVP", mvp);
    m_program->setUniformValue("uHeightScale", m_heightScale);
    m_program->setUniformValue("uTexSize", static_cast<float>(m_texRes));

    m_program->setUniformValue("uHitPos", m_hitPos);
    m_program->setUniformValue("uHitValid", m_hitValid ? 1.0f : 0.0f);
    m_program->setUniformValue("uCameraForward", cameraForward);

    // Uniforms de brush
    m_program->setUniformValue("uBrushIndex", m_brushIndex);
    m_program->setUniformValue("uBrushSize", m_brushSize);

    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, m_tex);
    m_program->setUniformValue("uHeight", 0);

    if (m_brushArray != 0) {
        gl->glActiveTexture(GL_TEXTURE1);
        gl->glBindTexture(GL_TEXTURE_2D_ARRAY, m_brushArray);
        m_program->setUniformValue("uBrushArray", 1);
    }

    // Erosion mask overlay
    m_program->setUniformValue("uMaskEnabled", m_erosionMaskTex != 0 ? (m_showMaskOverlay ? 1.0f : 0.0f) : 0.0f);
    if (m_erosionMaskTex != 0) {
        gl->glActiveTexture(GL_TEXTURE2);
        gl->glBindTexture(GL_TEXTURE_2D, m_erosionMaskTex);
        m_program->setUniformValue("uErosionMask", 2);
    }

    gl->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    gl->glDrawElements(GL_TRIANGLES, GLsizei(m_indices.size()), GL_UNSIGNED_INT, nullptr);

    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    gl->glDisableVertexAttribArray(0);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);

    gl->glBindTexture(GL_TEXTURE_2D, 0);
    if (m_brushArray != 0) {
        gl->glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    }
    if (m_erosionMaskTex != 0) {
        gl->glBindTexture(GL_TEXTURE_2D, 0);
    }
    m_program->release();
}

QImage TerrainGpu::exportHeightmap16(QOpenGLFunctions *gl) const {
    if (!gl || !m_tex) {
        LOG_WARN() << "exportHeightmap16: paramètres invalides";
        return QImage();
    }

    const int texSize = m_texRes;
    const size_t dataSize = static_cast<size_t>(texSize) * static_cast<size_t>(texSize);
    std::vector<float> data(dataSize);

    // On crée un FBO temporaire pour lire la texture
    GLuint fbo = 0;
    gl->glGenFramebuffers(1, &fbo);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_tex, 0);

    if (gl->glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR() << "FBO incomplet pour lecture de texture";
        gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
        gl->glDeleteFramebuffers(1, &fbo);
        return QImage();
    }

    gl->glReadPixels(0, 0, texSize, texSize, GL_RED, GL_FLOAT, data.data());
    gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gl->glDeleteFramebuffers(1, &fbo);

    LOG_INFO() << "Texture heightmap lue depuis GPU: " << texSize << "x" << texSize;

    // Créer l'image en 16-bit grayscale
    QImage img(texSize, texSize, QImage::Format_Grayscale16);
    if (img.isNull()) {
        LOG_ERROR() << "Échec création QImage Grayscale16";
        return QImage();
    }

    for (int y = 0; y < texSize; ++y) {
        quint16 *scanline = reinterpret_cast<quint16*>(img.scanLine(y));
        for (int x = 0; x < texSize; ++x) {
            const float height = data[y * texSize + x];
            scanline[x] = static_cast<quint16>(height * 65535.0f);
        }
    }

    LOG_INFO() << "Heightmap 16-bit exportée: " << texSize << "x" << texSize;
    return img;
}