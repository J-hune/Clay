#include "ErosionControllerQml.h"
#include "../Log.h"

ErosionControllerQml::ErosionControllerQml(QObject *parent)
    : QObject(parent)
{
    LOG_INFO() << "ErosionControllerQml créé";
}

void ErosionControllerQml::setIterations(int val) {
    if (m_erosion.iterations() != val) {
        m_erosion.setIterations(val);
        emit iterationsChanged();
    }
}

void ErosionControllerQml::setNumParticles(int val) {
    if (m_erosion.numParticles() != val) {
        m_erosion.setNumParticles(val);
        emit numParticlesChanged();
    }
}

void ErosionControllerQml::setInertia(float val) {
    if (m_erosion.inertia() != val) {
        m_erosion.setInertia(val);
        emit inertiaChanged();
    }
}

void ErosionControllerQml::setSedimentCapacity(float val) {
    if (m_erosion.sedimentCapacity() != val) {
        m_erosion.setSedimentCapacity(val);
        emit sedimentCapacityChanged();
    }
}

void ErosionControllerQml::setDepositionPercentage(float val) {
    if (m_erosion.depositionPercentage() != val) {
        m_erosion.setDepositionPercentage(val);
        emit depositionPercentageChanged();
    }
}

void ErosionControllerQml::setErosionSpeed(float val) {
    if (m_erosion.erosionSpeed() != val) {
        m_erosion.setErosionSpeed(val);
        emit erosionSpeedChanged();
    }
}

void ErosionControllerQml::setEvaporationSpeed(float val) {
    if (m_erosion.evaporationSpeed() != val) {
        m_erosion.setEvaporationSpeed(val);
        emit evaporationSpeedChanged();
    }
}

void ErosionControllerQml::setGravity(float val) {
    if (m_erosion.gravity() != val) {
        m_erosion.setGravity(val);
        emit gravityChanged();
    }
}

void ErosionControllerQml::setMinSlope(float val) {
    if (m_erosion.minSlope() != val) {
        m_erosion.setMinSlope(val);
        emit minSlopeChanged();
    }
}

void ErosionControllerQml::setMaxLifetime(int val) {
    if (m_erosion.maxLifetime() != val) {
        m_erosion.setMaxLifetime(val);
        emit maxLifetimeChanged();
    }
}

void ErosionControllerQml::setErosionRadius(int val) {
    if (m_erosion.erosionRadius() != val) {
        m_erosion.setErosionRadius(val);
        emit erosionRadiusChanged();
    }
}

void ErosionControllerQml::setIsErosionRunning(bool val) {
    if (m_isErosionRunning != val) {
        m_isErosionRunning = val;
        emit isErosionRunningChanged();
    }
}

void ErosionControllerQml::applyErosion() {
    LOG_DEBUG() << "Érosion demandée depuis QML";
    emit erosionRequested();
}

void ErosionControllerQml::toggleErosion() {
    setIsErosionRunning(!m_isErosionRunning);
    LOG_DEBUG() << "Érosion continue " << (m_isErosionRunning ? "démarrée" : "arrêtée");
}

