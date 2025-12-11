#ifndef CLAYAPP_EROSIONCONTROLLERQML_H
#define CLAYAPP_EROSIONCONTROLLERQML_H

#include <QObject>
#include "../TerrainErosion.h"

/**
 * @brief Bridge QML/C++ pour contrôler l'érosion du terrain
 * Expose les paramètres d'érosion à l'interface QML et déclenche l'érosion
 */
class ErosionControllerQml : public QObject {
    Q_OBJECT

    // Propriétés exposées à QML
    Q_PROPERTY(int iterations READ iterations WRITE setIterations NOTIFY iterationsChanged)
    Q_PROPERTY(int numParticles READ numParticles WRITE setNumParticles NOTIFY numParticlesChanged)
    Q_PROPERTY(float inertia READ inertia WRITE setInertia NOTIFY inertiaChanged)
    Q_PROPERTY(float sedimentCapacity READ sedimentCapacity WRITE setSedimentCapacity NOTIFY sedimentCapacityChanged)
    Q_PROPERTY(float depositionPercentage READ depositionPercentage WRITE setDepositionPercentage NOTIFY depositionPercentageChanged)
    Q_PROPERTY(float erosionSpeed READ erosionSpeed WRITE setErosionSpeed NOTIFY erosionSpeedChanged)
    Q_PROPERTY(float evaporationSpeed READ evaporationSpeed WRITE setEvaporationSpeed NOTIFY evaporationSpeedChanged)
    Q_PROPERTY(float gravity READ gravity WRITE setGravity NOTIFY gravityChanged)
    Q_PROPERTY(float minSlope READ minSlope WRITE setMinSlope NOTIFY minSlopeChanged)
    Q_PROPERTY(int maxLifetime READ maxLifetime WRITE setMaxLifetime NOTIFY maxLifetimeChanged)
    Q_PROPERTY(int erosionRadius READ erosionRadius WRITE setErosionRadius NOTIFY erosionRadiusChanged)
    Q_PROPERTY(bool isErosionRunning READ isErosionRunning WRITE setIsErosionRunning NOTIFY isErosionRunningChanged)

public:
    explicit ErosionControllerQml(QObject *parent = nullptr);

    // Accès au système d'érosion (pour GLRenderer)
    TerrainErosion& erosion() { return m_erosion; }
    const TerrainErosion& erosion() const { return m_erosion; }

    // Getters
    int iterations() const { return m_erosion.iterations(); }
    int numParticles() const { return m_erosion.numParticles(); }
    float inertia() const { return m_erosion.inertia(); }
    float sedimentCapacity() const { return m_erosion.sedimentCapacity(); }
    float depositionPercentage() const { return m_erosion.depositionPercentage(); }
    float erosionSpeed() const { return m_erosion.erosionSpeed(); }
    float evaporationSpeed() const { return m_erosion.evaporationSpeed(); }
    float gravity() const { return m_erosion.gravity(); }
    float minSlope() const { return m_erosion.minSlope(); }
    int maxLifetime() const { return m_erosion.maxLifetime(); }
    int erosionRadius() const { return m_erosion.erosionRadius(); }
    bool isErosionRunning() const { return m_isErosionRunning; }

    // Setters
    void setIterations(int val);
    void setNumParticles(int val);
    void setInertia(float val);
    void setSedimentCapacity(float val);
    void setDepositionPercentage(float val);
    void setErosionSpeed(float val);
    void setEvaporationSpeed(float val);
    void setGravity(float val);
    void setMinSlope(float val);
    void setMaxLifetime(int val);
    void setErosionRadius(int val);
    void setIsErosionRunning(bool val);

    // Méthodes invocables depuis QML
    Q_INVOKABLE void applyErosion();
    Q_INVOKABLE void toggleErosion();

signals:
    void iterationsChanged();
    void numParticlesChanged();
    void inertiaChanged();
    void sedimentCapacityChanged();
    void depositionPercentageChanged();
    void erosionSpeedChanged();
    void evaporationSpeedChanged();
    void gravityChanged();
    void minSlopeChanged();
    void maxLifetimeChanged();
    void erosionRadiusChanged();
    void isErosionRunningChanged();

    // Signal pour notifier le renderer qu'une érosion doit être appliquée
    void erosionRequested();

private:
    TerrainErosion m_erosion;
    bool m_isErosionRunning = false;
};

#endif // CLAYAPP_EROSIONCONTROLLERQML_H

