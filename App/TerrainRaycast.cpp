#include "TerrainRaycast.h"
#include <QOpenGLShader>
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

void TerrainRaycast::initialize(QOpenGLExtraFunctions *gl) {
    ensureProgram(gl);
    ensureSSBO(gl);
}

void TerrainRaycast::cleanup(QOpenGLExtraFunctions *gl) {
    if (m_ssbo) {
        gl->glDeleteBuffers(1, &m_ssbo);
        m_ssbo = 0;
    }
    m_computeProgram.reset();
}

void TerrainRaycast::ensureProgram(QOpenGLExtraFunctions *gl) {
    if (m_computeProgram) return;

    m_computeProgram.reset(new QOpenGLShaderProgram);

    QString shaderSource = loadShaderSource("terrain_raycast.comp");
    if (shaderSource.isEmpty()) {
        LOG_ERROR() << "Impossible de charger le shader terrain_raycast.comp";
        m_computeProgram.reset();
        return;
    }

    if (!m_computeProgram->addShaderFromSourceCode(QOpenGLShader::Compute, shaderSource)) {
        LOG_ERROR() << "Erreur de compilation du compute shader: " << m_computeProgram->log().toStdString();
        m_computeProgram.reset();
        return;
    }

    if (!m_computeProgram->link()) {
        LOG_ERROR() << "Erreur de linkage du compute shader: " << m_computeProgram->log().toStdString();
        m_computeProgram.reset();
        return;
    }

    LOG_INFO() << "Compute shader de raycast compilé et lié";
}

void TerrainRaycast::ensureSSBO(QOpenGLExtraFunctions *gl) {
    if (m_ssbo) return;

    gl->glGenBuffers(1, &m_ssbo);
    gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);

    // Allouer le buffer (taille = sizeof(RaycastResult))
    RaycastResult initialData = {};
    gl->glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(RaycastResult), &initialData, GL_DYNAMIC_READ);

    gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    LOG_INFO() << "SSBO pour raycast créé, taille=" << sizeof(RaycastResult) << " octets";
}

void TerrainRaycast::performRaycast(QOpenGLExtraFunctions *gl,
                                    const QVector2D &mouseNDC,
                                    const QMatrix4x4 &projectionMatrix,
                                    const QMatrix4x4 &viewMatrix,
                                    GLuint heightmapTexture,
                                    int heightmapResolution,
                                    float heightScale) {
    if (!m_computeProgram || !m_ssbo) return;

    m_computeProgram->bind();

    // Calculer la matrice inverse ViewProjection
    QMatrix4x4 viewProj = projectionMatrix * viewMatrix;
    QMatrix4x4 invViewProj = viewProj.inverted();

    // Uniforms
    m_computeProgram->setUniformValue("uMouseNDC", mouseNDC);
    m_computeProgram->setUniformValue("uInvViewProj", invViewProj);
    m_computeProgram->setUniformValue("uHeightScale", heightScale);
    m_computeProgram->setUniformValue("uHeightmapRes", heightmapResolution);
    m_computeProgram->setUniformValue("uTerrainBounds",
                                     QVector4D(m_terrainMinX, m_terrainMaxX,
                                              m_terrainMinZ, m_terrainMaxZ));

    // Bind heightmap texture
    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, heightmapTexture);
    m_computeProgram->setUniformValue("uHeightmap", 0);

    // Bind SSBO
    gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ssbo);

    // Dispatch compute shader (1 invocation)
    gl->glDispatchCompute(1, 1, 1);

    // Attendre la fin du compute shader
    gl->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    m_computeProgram->release();
    gl->glBindTexture(GL_TEXTURE_2D, 0);
}

RaycastResult TerrainRaycast::readResult(QOpenGLExtraFunctions *gl) {
    RaycastResult result = {};

    if (!m_ssbo) return result;

    gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);

    void *ptr = gl->glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(RaycastResult), GL_MAP_READ_BIT);
    if (ptr) {
        memcpy(&result, ptr, sizeof(RaycastResult));
        gl->glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    return result;
}

