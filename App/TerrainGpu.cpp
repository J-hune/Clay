#include "TerrainGpu.h"
#include <QRgba64>
#include <algorithm>
#include "Log.h"

static const char *kVS = R"GLSL(
#version 440 core
layout(location = 0) in vec2 aUV;

uniform sampler2D uHeight;
uniform mat4 uMVP;
uniform float uHeightScale;

out float vHeight;
out vec2 vUV;
out vec3 vWorldPos;

void main() {
    vec2 worldSize = vec2(100.0, 100.0); // taille fixe
    vUV = aUV;
    float h = texture(uHeight, aUV).r * uHeightScale;
    vHeight = h;
    vec2 posXZ = (aUV - vec2(0.5)) * worldSize;
    vWorldPos = vec3(posXZ.x, h, posXZ.y);
    gl_Position = uMVP * vec4(posXZ.x, h, posXZ.y, 1.0);
}
)GLSL";

static const char *kFS = R"GLSL(
#version 440 core

uniform sampler2D uHeight;
uniform float uHeightScale;
uniform float uTexSize; // résolution de la heightmap
uniform vec3 uHitPos; // Position du hit du raycast
uniform float uHitValid; // 1.0 si hit valide, 0.0 sinon
uniform vec3 uCameraForward; // direction avant de la caméra

// Brush preview
uniform sampler2DArray uBrushArray;
uniform int uBrushIndex;
uniform float uBrushSize;

in float vHeight;
in vec2 vUV;
in vec3 vWorldPos;

out vec4 fragColor;

// Calcule une normale approchée depuis la texture height (centrée)
vec3 computeNormal(vec2 uv) {
    float texel = 1.0 / uTexSize;
    float hL = texture(uHeight, uv + vec2(-texel, 0.0)).r * uHeightScale;
    float hR = texture(uHeight, uv + vec2( texel, 0.0)).r * uHeightScale;
    float hD = texture(uHeight, uv + vec2(0.0, -texel)).r * uHeightScale;
    float hU = texture(uHeight, uv + vec2(0.0,  texel)).r * uHeightScale;
    // Gradient
    float dx = hR - hL;
    float dz = hU - hD;
    vec3 n = normalize(vec3(-dx, 2.0, -dz));
    return n;
}

vec3 gradientColor(float hNorm) {
    // Couleurs clés
    vec3 low  = vec3(0.08, 0.25, 0.10);   // sombre
    vec3 mid  = vec3(0.15, 0.50, 0.20);   // vert moyen
    vec3 high = vec3(0.80, 0.80, 0.75);   // sommet clair (neige)
    if (hNorm < 0.5) {
        float t = hNorm / 0.5;
        return mix(low, mid, t);
    } else {
        float t = (hNorm - 0.5) / 0.5;
        return mix(mid, high, t);
    }
}

void main() {
    float safeScale = max(uHeightScale, 0.0001);
    float hNorm = clamp(vHeight / safeScale, 0.0, 1.0); // normalisation (0..1)
    vec3 baseCol = gradientColor(hNorm);

    // Normal + lumière directionnelle simple
    vec3 N = computeNormal(vUV);
    vec3 L = normalize(vec3(0.4, 1.0, 0.3));
    float diff = max(dot(N, L), 0.0);
    float ambient = 0.35;
    vec3 lit = baseCol * (ambient + diff * 0.65);

    // Aperçu du brush : on utilise la texture du brush
    if (uHitValid > 0.5 && uBrushSize > 0.0 && uBrushIndex >= 0) {
        // Coordonnées locales [0,1] dans l'espace du brush (plan XZ)
        vec2 delta = vWorldPos.xz - uHitPos.xz;

        // On calcule l'angle de rotation basé sur la direction de la caméra (yaw)
        vec2 cameraDir2D = normalize(uCameraForward.xz);
        float angle = atan(cameraDir2D.y, cameraDir2D.x);

        // Matrice de rotation 2D
        float c = cos(angle);
        float s = sin(angle);
        mat2 rotation = mat2(c, -s, s, c);

        vec2 rotatedDelta = rotation * delta;
        vec2 local = rotatedDelta / (uBrushSize * 2.0) + 0.5;

        // si on est dans le carré
        if (local.x >= 0.0 && local.x <= 1.0 &&
            local.y >= 0.0 && local.y <= 1.0) {

            float texVal = texture(uBrushArray, vec3(local, float(uBrushIndex))).r;
            lit = mix(lit, vec3(0.627, 0.796, 0.835), texVal);
        }
    }

    fragColor = vec4(lit, 1.0);
}
)GLSL";

void TerrainGpu::ensureProgram(const QOpenGLFunctions *gl) {
    Q_UNUSED(gl);
    if (m_program && m_program->isLinked()) return;
    m_program.reset(new QOpenGLShaderProgram());
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, kVS)) {
        LOG_ERROR() << "Échec compilation Vertex Shader terrain: " << m_program->log().toStdString();
        return;
    }
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, kFS)) {
        LOG_ERROR() << "Échec compilation Fragment Shader terrain: " << m_program->log().toStdString();
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
    m_program->release();
}
