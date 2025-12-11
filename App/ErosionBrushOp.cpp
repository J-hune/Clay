#include "ErosionBrushOp.h"
#include "Log.h"
#include <QOpenGLShader>
#include <QCoreApplication>
#include <QDir>

static QString loadShaderSourceErosionMask(const QString &filename) {
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

void ErosionBrushOp::initialize(QOpenGLExtraFunctions *gl) {
    ensureProgram(gl);
}

void ErosionBrushOp::cleanup(QOpenGLExtraFunctions *gl) {
    Q_UNUSED(gl);
    m_program.reset();
}

void ErosionBrushOp::ensureProgram(QOpenGLExtraFunctions *gl) {
    Q_UNUSED(gl);
    if (m_program) return;

    m_program.reset(new QOpenGLShaderProgram);
    QString shaderSource = loadShaderSourceErosionMask("erosion_mask_op.comp");
    if (shaderSource.isEmpty()) {
        LOG_ERROR() << "Impossible de charger le shader erosion_mask_op.comp";
        m_program.reset();
        return;
    }

    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Compute, shaderSource)) {
        LOG_ERROR() << "Erreur compilation compute shader (ErosionMaskOp): " << m_program->log().toStdString();
        m_program.reset();
        return;
    }
    if (!m_program->link()) {
        LOG_ERROR() << "Erreur linkage compute shader (ErosionMaskOp): " << m_program->log().toStdString();
        m_program.reset();
        return;
    }
    LOG_INFO() << "Compute shader ErosionMaskOp compilé";
}

void ErosionBrushOp::applyMask(QOpenGLExtraFunctions *gl,
                               GLuint maskTexture,
                               int maskResolution,
                               const QVector3D &worldPos,
                               float brushSize,
                               float brushStrength,
                               GLuint brushTextureArray,
                               int brushIndex,
                               float terrainMinX,
                               float terrainMaxX,
                               float terrainMinZ,
                               float terrainMaxZ,
                               const QVector3D &cameraForward,
                               bool erase) {
    if (!m_program) {
        ensureProgram(gl);
        if (!m_program) return;
    }

    m_program->bind();
    m_program->setUniformValue("uWorldPos", worldPos);
    m_program->setUniformValue("uBrushSize", brushSize);
    m_program->setUniformValue("uBrushStrength", brushStrength);
    m_program->setUniformValue("uBrushIndex", brushIndex);
    m_program->setUniformValue("uTerrainBounds", QVector4D(terrainMinX, terrainMaxX, terrainMinZ, terrainMaxZ));
    m_program->setUniformValue("uMaskRes", maskResolution);
    m_program->setUniformValue("uCameraForward", cameraForward);
    m_program->setUniformValue("uErase", erase ? 1 : 0);

    gl->glBindImageTexture(0, maskTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8);
    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D_ARRAY, brushTextureArray);
    m_program->setUniformValue("uBrushArray", 0);

    const int groupsX = (maskResolution + 7) / 8;
    const int groupsY = (maskResolution + 7) / 8;
    gl->glDispatchCompute(groupsX, groupsY, 1);
    gl->glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    m_program->release();
    gl->glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}
