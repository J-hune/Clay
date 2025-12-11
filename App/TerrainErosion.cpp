
#include "TerrainErosion.h"
#include "Log.h"
#include <QFile>
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
    LOG_INFO() << "Shader érosion chargé: " << shaderPath.toStdString();
    return source;
}

void TerrainErosion::ensurePrograms(QOpenGLExtraFunctions *gl) {
    if (m_firstPassProgram && m_firstPassProgram->isLinked() &&
        m_secondPassProgram && m_secondPassProgram->isLinked() &&
        m_convertFloatToFixedProgram && m_convertFloatToFixedProgram->isLinked() &&
        m_convertFixedToFloatProgram && m_convertFixedToFloatProgram->isLinked()) {
        return;
    }

    // === PREMIÈRE PASSE : Simulation des particules ===
    m_firstPassProgram.reset(new QOpenGLShaderProgram());
    QString firstPassSource = loadShaderSource("erosion_first_pass.comp");
    if (firstPassSource.isEmpty()) {
        LOG_ERROR() << "Impossible de charger erosion_first_pass.comp";
        m_firstPassProgram.reset();
        return;
    }

    if (!m_firstPassProgram->addShaderFromSourceCode(QOpenGLShader::Compute, firstPassSource)) {
        LOG_ERROR() << "Échec compilation First Pass Compute Shader: " << m_firstPassProgram->log().toStdString();
        m_firstPassProgram.reset();
        return;
    }

    if (!m_firstPassProgram->link()) {
        LOG_ERROR() << "Échec link First Pass Program: " << m_firstPassProgram->log().toStdString();
        m_firstPassProgram.reset();
        return;
    }

    LOG_INFO() << "First Pass Erosion Shader compilé et lié";

    // === Convertion float -> fixed temporaire ===

    m_convertFloatToFixedProgram.reset(new QOpenGLShaderProgram());
    QString convertFloatSource = loadShaderSource("convert_float_to_fixed.comp");
    if (convertFloatSource.isEmpty()) {
        LOG_ERROR() << "Impossible de charger convert_float_to_fixed.comp";
        m_convertFloatToFixedProgram.reset();
        return;
    }

    if (!m_convertFloatToFixedProgram->addShaderFromSourceCode(QOpenGLShader::Compute, convertFloatSource)) {
        LOG_ERROR() << "Échec compilation Convert Float to Fixed Compute Shader: "
                    << m_convertFloatToFixedProgram->log().toStdString();
        m_convertFloatToFixedProgram.reset();
        return;
    }

    if (!m_convertFloatToFixedProgram->link()) {
        LOG_ERROR() << "Échec link Convert Float to Fixed Program: "
                    << m_convertFloatToFixedProgram->log().toStdString();
        m_convertFloatToFixedProgram.reset();
        return;
    }

    // === Convertion fixed -> float temporaire ===

    m_convertFixedToFloatProgram.reset(new QOpenGLShaderProgram());
    QString convertFixedSource = loadShaderSource("convert_fixed_to_float.comp");
    if (convertFixedSource.isEmpty()) {
        LOG_ERROR() << "Impossible de charger convert_fixed_to_float.comp";
        m_convertFixedToFloatProgram.reset();
        return;
    }

    if (!m_convertFixedToFloatProgram->addShaderFromSourceCode(QOpenGLShader::Compute, convertFixedSource)) {
        LOG_ERROR() << "Échec compilation Convert Fixed to Float Compute Shader: "
                    << m_convertFixedToFloatProgram->log().toStdString();
        m_convertFixedToFloatProgram.reset();
        return;
    }

    if (!m_convertFixedToFloatProgram->link()) {
        LOG_ERROR() << "Échec link Convert Fixed to Float Program: "
                    << m_convertFixedToFloatProgram->log().toStdString();
        m_convertFixedToFloatProgram.reset();
        return;
    }


    // === SECONDE PASSE : Application des changements ===
    m_secondPassProgram.reset(new QOpenGLShaderProgram());
    QString secondPassSource = loadShaderSource("erosion_second_pass.comp");
    if (secondPassSource.isEmpty()) {
        LOG_ERROR() << "Impossible de charger erosion_second_pass.comp";
        m_secondPassProgram.reset();
        return;
    }

    if (!m_secondPassProgram->addShaderFromSourceCode(QOpenGLShader::Compute, secondPassSource)) {
        LOG_ERROR() << "Échec compilation Second Pass Compute Shader: " << m_secondPassProgram->log().toStdString();
        m_secondPassProgram.reset();
        return;
    }

    if (!m_secondPassProgram->link()) {
        LOG_ERROR() << "Échec link Second Pass Program: " << m_secondPassProgram->log().toStdString();
        m_secondPassProgram.reset();
        return;
    }

    LOG_INFO() << "Second Pass Erosion Shader compilé et lié";
}

void TerrainErosion::createTempIntTexture(QOpenGLExtraFunctions *gl, int heightmapSize) {
    // Si la texture existe déjà, la détruire (sera recréée avec la bonne taille)
    if (m_tempIntTexture != 0) {
        gl->glDeleteTextures(1, &m_tempIntTexture);
        m_tempIntTexture = 0;
    }

    gl->glGenTextures(1, &m_tempIntTexture);
    gl->glBindTexture(GL_TEXTURE_2D, m_tempIntTexture);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl->glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32I, heightmapSize, heightmapSize);
    gl->glBindTexture(GL_TEXTURE_2D, 0);

    LOG_INFO() << "Texture temporaire d'érosion créée (taille: " << heightmapSize << "x" << heightmapSize << ")";
}

void TerrainErosion::ensureBuffers(QOpenGLExtraFunctions *gl, int heightmapSize) {
    // Si la taille n'a pas changé et que les buffers existent déjà, ne rien faire
    if (m_particleBuffer != 0 && m_lastHeightmapSize == heightmapSize) {
        return;
    }

    // Si la taille a changé, on doit TOUT recréer
    const bool sizeChanged = (m_lastHeightmapSize != heightmapSize && m_lastHeightmapSize != 0);
    if (sizeChanged) {
        LOG_INFO() << "Changement de résolution détecté (" << m_lastHeightmapSize
                   << " -> " << heightmapSize << "), recréation de tous les buffers d'érosion";

        // Détruire les anciens buffers
        if (m_particleBuffer != 0) {
            gl->glDeleteBuffers(1, &m_particleBuffer);
            m_particleBuffer = 0;
        }
        if (m_modBuffer != 0) {
            gl->glDeleteBuffers(1, &m_modBuffer);
            m_modBuffer = 0;
        }
        if (m_tempIntTexture != 0) {
            gl->glDeleteTextures(1, &m_tempIntTexture);
            m_tempIntTexture = 0;
        }
    }

    m_lastHeightmapSize = heightmapSize;

    // Buffer pour les particules (6 floats par particule : pos.xy, vel.xy, sediment, water)
    const int numParticles = m_numParticles;
    const int particleDataSize = numParticles * 6 * sizeof(float);

    if (m_particleBuffer == 0) {
        gl->glGenBuffers(1, &m_particleBuffer);
    }

    gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_particleBuffer);
    gl->glBufferData(GL_SHADER_STORAGE_BUFFER, particleDataSize, nullptr, GL_DYNAMIC_COPY);
    gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Modification buffer (STEP_SIZE=6 floats par step)
    const int STEP_SIZE = 6;
    const GLsizeiptr modCount = GLsizeiptr(m_numParticles) * GLsizeiptr(m_maxLifetime);
    const GLsizeiptr modBytes = modCount * STEP_SIZE * sizeof(float);

    if (m_modBuffer == 0) {
        gl->glGenBuffers(1, &m_modBuffer);
    }

    gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_modBuffer);
    gl->glBufferData(GL_SHADER_STORAGE_BUFFER, modBytes, nullptr, GL_DYNAMIC_COPY);
    // clear à 0
    void* p = gl->glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, modBytes, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    if (p) {
        std::memset(p, 0, size_t(modBytes));
        gl->glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }
    gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    createTempIntTexture(gl, heightmapSize);

    LOG_INFO() << "Buffers d'érosion créés/mis à jour pour résolution " << heightmapSize
               << " (" << numParticles << " particules)";
}

void TerrainErosion::dispatch(QOpenGLExtraFunctions *gl, GLuint heightmapTexture, int heightmapSize) {
    ensurePrograms(gl);
    if (!m_firstPassProgram || !m_secondPassProgram || !m_convertFloatToFixedProgram || !m_convertFixedToFloatProgram) {
        LOG_ERROR() << "Programmes d'érosion non disponibles";
        return;
    }

    ensureBuffers(gl, heightmapSize);

    LOG_INFO() << "=== Début érosion (2 passes) ===";
    LOG_INFO() << "Paramètres: particles=" << m_numParticles
               << ", inertia=" << m_inertia
               << ", capacity=" << m_sedimentCapacity
               << ", deposition=" << m_depositionPercentage
               << ", erosion=" << m_erosionSpeed
               << ", evaporation=" << m_evaporationSpeed
               << ", gravity=" << m_gravity
               << ", minSlope=" << m_minSlope;
    convertFloatToFixedTexture(gl, heightmapTexture, heightmapSize);

    // Vérifier si le masque d'érosion est actif
    const bool hasMask = (m_erosionMaskTexture != 0);

    // === PREMIÈRE PASSE : Simulation des particules ===
    m_firstPassProgram->bind();

    // Bind heightmap en lecture (DOIT être bindé AVANT le dispatch)
    gl->glBindImageTexture(0, m_tempIntTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32I);

    // Bind particle buffer
    gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_particleBuffer);
    gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_modBuffer);

    // Uniformes pour la première passe
    m_firstPassProgram->setUniformValue("mapSize", float(heightmapSize));
    m_firstPassProgram->setUniformValue("numParticles", m_numParticles);
    m_firstPassProgram->setUniformValue("maxLifetime", m_maxLifetime);
    m_firstPassProgram->setUniformValue("p_inertia", m_inertia);
    m_firstPassProgram->setUniformValue("p_capacity", m_sedimentCapacity);
    m_firstPassProgram->setUniformValue("p_deposition", m_depositionPercentage);
    m_firstPassProgram->setUniformValue("p_erosion", m_erosionSpeed);
    m_firstPassProgram->setUniformValue("p_evaporation", m_evaporationSpeed);
    m_firstPassProgram->setUniformValue("p_gravity", m_gravity);
    m_firstPassProgram->setUniformValue("p_minSlope", m_minSlope);
    m_firstPassProgram->setUniformValue("erosionRadius", m_erosionRadius);
    m_firstPassProgram->setUniformValue("init_vel", m_initVelocity);
    m_firstPassProgram->setUniformValue("init_water", m_initWater);
    m_firstPassProgram->setUniformValue("fixedPointScale", m_fixedPointScale); // int

    // Masque d'érosion (optionnel)
    // Si une texture de masque a été fournie, on la bind en lecture sur l'unité d'image 3
    // et on active le flag useMask côté shader. Sinon, on désactive le masque.
    m_firstPassProgram->setUniformValue("useMask", hasMask ? 1 : 0);
    if (hasMask) {
        gl->glBindImageTexture(3, m_erosionMaskTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);
    }

    // Box de spawn (4 coins)
    QVector2D corners[4] = {
        QVector2D(0.f, 0.f),
        QVector2D(float(heightmapSize-1), 0.f),
        QVector2D(0.f, float(heightmapSize-1)),
        QVector2D(float(heightmapSize-1), float(heightmapSize-1))
    };
    m_firstPassProgram->setUniformValueArray("cornersBox", corners, 4);
    m_firstPassProgram->release();

    // Set uniform de la seconde pass
    m_secondPassProgram->bind();
    m_secondPassProgram->setUniformValue("mapSize", heightmapSize);
    m_secondPassProgram->setUniformValue("numParticles", m_numParticles);
    m_secondPassProgram->setUniformValue("maxLifetime", m_maxLifetime);
    m_secondPassProgram->setUniformValue("fixedPointScale", m_fixedPointScale);
    m_secondPassProgram->release();

    // Dispatch première passe (nombre de particules / 256)
    const int workGroupSize = 256;
    const int numGroups = (m_numParticles + workGroupSize - 1) / workGroupSize;
    const int STEP_SIZE = 6;

    const GLuint totalSteps = GLuint(m_numParticles * m_maxLifetime);
    const GLuint groupsForMods = (totalSteps + workGroupSize - 1) / workGroupSize;

    // Clear modifications buffer
    GLsizeiptr modBytes = GLsizeiptr(m_numParticles) * GLsizeiptr(m_maxLifetime) * STEP_SIZE * sizeof(float);

    gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_modBuffer);
    void* p = gl->glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, modBytes,
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    if (p) { memset(p, 0, (size_t)modBytes); gl->glUnmapBuffer(GL_SHADER_STORAGE_BUFFER); }
    gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


    // Bind heightmap en lecture
    gl->glBindImageTexture(0, m_tempIntTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32I);

    // Re-bind masque d'érosion si actif
    if (hasMask) {
        gl->glBindImageTexture(3, m_erosionMaskTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);
    }

    // Lancer la première passe
    m_firstPassProgram->bind();
    // Choix d'une seed basé sur l'horloge interne de l'ordinateur
    int seed = int(QDateTime::currentMSecsSinceEpoch() & 0xFFFFFFFF);
    m_firstPassProgram->setUniformValue("seed", seed);
    gl->glDispatchCompute(numGroups, 1, 1);

    // Barrière mémoire pour s'assurer que la première passe est terminée
    gl->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    m_firstPassProgram->release();

    // Lancer la seconde passe
    m_secondPassProgram->bind();
    // Re-bind heightmap et particle buffer
    gl->glBindImageTexture(0, m_tempIntTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);
    gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_modBuffer);
    // Dispatch seconde passe en 1D
    gl->glDispatchCompute(groupsForMods, 1, 1);
    // Barrière mémoire pour s'assurer que la seconde passe est terminée
    gl->glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    m_secondPassProgram->release();

    convertFixedToFloatTexture(gl, heightmapTexture, heightmapSize);
    LOG_INFO() << "Seconde passe terminée - Érosion appliquée";
}

void TerrainErosion::convertFloatToFixedTexture(QOpenGLExtraFunctions* gl, GLuint floatHeightmapTexture, int heightmapSize) const{
    LOG_INFO() << "Conversion float -> fixed taille de texture: " << heightmapSize;
    m_convertFloatToFixedProgram->bind();
    gl->glBindImageTexture(0, floatHeightmapTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
    gl->glBindImageTexture(1, m_tempIntTexture,     0, GL_FALSE, 0, GL_WRITE_ONLY,GL_R32I);
    m_convertFloatToFixedProgram->setUniformValue("mapSize", heightmapSize);
    m_convertFloatToFixedProgram->setUniformValue("fixedPointScale", m_fixedPointScale);
    int groupsX = (heightmapSize + 15) / 16;
    int groupsY = (heightmapSize + 15) / 16;
    gl->glDispatchCompute(groupsX, groupsY, 1);
    gl->glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    m_convertFloatToFixedProgram->release();
}

void TerrainErosion::convertFixedToFloatTexture(QOpenGLExtraFunctions* gl, GLuint floatHeightmapTexture, int heightmapSize) const
{
    m_convertFixedToFloatProgram->bind();
    gl->glBindImageTexture(0, m_tempIntTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32I);
    gl->glBindImageTexture(1, floatHeightmapTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
    m_convertFixedToFloatProgram->setUniformValue("mapSize", heightmapSize);
    m_convertFixedToFloatProgram->setUniformValue("fixedPointScale", m_fixedPointScale);
    int groupsX = (heightmapSize + 15) / 16;
    int groupsY = (heightmapSize + 15) / 16;
    gl->glDispatchCompute(groupsX, groupsY, 1);
    gl->glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    m_convertFixedToFloatProgram->release();
}


TerrainErosion::~TerrainErosion() {
    // Les buffers seront détruits automatiquement par OpenGL lors de la destruction du contexte
    // Ou manuellement si nécessaire
}

void TerrainErosion::invalidateBuffers() {
    // Réinitialiser la taille mémorisée pour forcer la recréation
    // lors du prochain appel à ensureBuffers
    LOG_INFO() << "Invalidation des buffers d'érosion (seront recréés au prochain dispatch)";
    m_lastHeightmapSize = 0;
}

