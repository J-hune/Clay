#include "TerrainBrushOp.h"
#include "Log.h"
#include <QOpenGLShader>
#include <QCoreApplication>
#include <QDir>

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

void TerrainBrushOp::initialize(QOpenGLExtraFunctions *gl) {
    ensureProgram(gl);
}

void TerrainBrushOp::cleanup(QOpenGLExtraFunctions *gl) {
    Q_UNUSED(gl);
    m_computeProgram.reset();
}

void TerrainBrushOp::ensureProgram(QOpenGLExtraFunctions *gl) {
    if (m_computeProgram) return;

    m_computeProgram.reset(new QOpenGLShaderProgram);

    QString shaderSource = loadShaderSource("brush_op.comp");
    if (shaderSource.isEmpty()) {
        LOG_ERROR() << "Impossible de charger le shader brush_op.comp";
        m_computeProgram.reset();
        return;
    }

    if (!m_computeProgram->addShaderFromSourceCode(QOpenGLShader::Compute, shaderSource)) {
        LOG_ERROR() << "Erreur compilation compute shader (BrushOp): " << m_computeProgram->log().toStdString();
        m_computeProgram.reset();
        return;
    }

    if (!m_computeProgram->link()) {
        LOG_ERROR() << "Erreur linkage compute shader (BrushOp): " << m_computeProgram->log().toStdString();
        m_computeProgram.reset();
        return;
    }

    LOG_INFO() << "Compute shader BrushOp compilé";
}

void TerrainBrushOp::applyBrush(QOpenGLExtraFunctions *gl,
                                BrushOpType opType,
                                GLuint heightmapTexture,
                                int heightmapResolution,
                                const QVector3D &worldPos,
                                float brushSize,
                                float brushStrength,
                                GLuint brushTextureArray,
                                int brushIndex,
                                float terrainMinX,
                                float terrainMaxX,
                                float terrainMinZ,
                                float terrainMaxZ,
                                float heightScale,
                                const QVector3D &cameraForward) {
    if (!m_computeProgram) {
        ensureProgram(gl);
        if (!m_computeProgram) {
            LOG_ERROR() << "Failed to create brush compute program";
            return;
        }
    }

    m_computeProgram->bind();

    m_computeProgram->setUniformValue("uOpType", static_cast<int>(opType));
    m_computeProgram->setUniformValue("uWorldPos", worldPos);
    m_computeProgram->setUniformValue("uBrushSize", brushSize);
    m_computeProgram->setUniformValue("uBrushStrength", brushStrength);
    m_computeProgram->setUniformValue("uBrushIndex", brushIndex);
    m_computeProgram->setUniformValue("uTerrainBounds", QVector4D(terrainMinX, terrainMaxX, terrainMinZ, terrainMaxZ));
    m_computeProgram->setUniformValue("uHeightScale", heightScale);
    m_computeProgram->setUniformValue("uHeightmapRes", heightmapResolution);
    m_computeProgram->setUniformValue("uCameraForward", cameraForward);

    gl->glBindImageTexture(0, heightmapTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);

    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D_ARRAY, brushTextureArray);
    m_computeProgram->setUniformValue("uBrushArray", 0);

    const int groupsX = (heightmapResolution + 7) / 8;
    const int groupsY = (heightmapResolution + 7) / 8;
    gl->glDispatchCompute(groupsX, groupsY, 1);

    gl->glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    m_computeProgram->release();
    gl->glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}
