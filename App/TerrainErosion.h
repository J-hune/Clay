#ifndef CLAYAPP_TERRAINEROSION_H
#define CLAYAPP_TERRAINEROSION_H

#include <QOpenGLShaderProgram>
#include <QOpenGLExtraFunctions>
#include <memory>

/**
 * @brief Gère l'application d'érosion sur le terrain via compute shader en 2 passes
 * Pass 1 : Simulation des particules d'eau qui érodent le terrain
 * Pass 2 : Application des changements accumulés sur la heightmap
 */
class TerrainErosion {
public:
    TerrainErosion() = default;
    ~TerrainErosion();

    /**
     * @brief Lance l'érosion sur la heightmap (2 passes)
     * @param gl Fonctions OpenGL
     * @param heightmapTexture Texture heightmap (R32F) à éroder
     * @param heightmapSize Taille de la heightmap (largeur = hauteur)
     */
    void dispatch(QOpenGLExtraFunctions *gl, GLuint heightmapTexture, int heightmapSize);

    // Paramètres d'érosion (modifiables depuis QML)
    int iterations() const { return m_iterations; }
    void setIterations(int val) { m_iterations = val; }

    int numParticles() const { return m_numParticles; }
    void setNumParticles(int val) { m_numParticles = val; }

    float inertia() const { return m_inertia; }
    void setInertia(float val) { m_inertia = val; }

    float sedimentCapacity() const { return m_sedimentCapacity; }
    void setSedimentCapacity(float val) { m_sedimentCapacity = val; }

    float depositionPercentage() const { return m_depositionPercentage; }
    void setDepositionPercentage(float val) { m_depositionPercentage = val; }

    float erosionSpeed() const { return m_erosionSpeed; }
    void setErosionSpeed(float val) { m_erosionSpeed = val; }

    float evaporationSpeed() const { return m_evaporationSpeed; }
    void setEvaporationSpeed(float val) { m_evaporationSpeed = val; }

    float gravity() const { return m_gravity; }
    void setGravity(float val) { m_gravity = val; }

    float minSlope() const { return m_minSlope; }
    void setMinSlope(float val) { m_minSlope = val; }

    int maxLifetime() const { return m_maxLifetime; }
    void setMaxLifetime(int val) { m_maxLifetime = val; }

private:
    void ensurePrograms(QOpenGLExtraFunctions *gl);
    void ensureBuffers(QOpenGLExtraFunctions *gl, int heightmapSize);


    void createTempIntTexture(QOpenGLExtraFunctions *gl, int heightmapSize);
    void convertFloatToFixedTexture(QOpenGLExtraFunctions *gl, GLuint floatHeightmapTexture, int size) const;
    void convertFixedToFloatTexture(QOpenGLExtraFunctions *gl, GLuint floatHeightmapTexture, int size) const;

    // Programmes de compute shader
    std::unique_ptr<QOpenGLShaderProgram> m_firstPassProgram;  // Simulation des particules
    std::unique_ptr<QOpenGLShaderProgram> m_secondPassProgram; // Application des changements
    std::unique_ptr<QOpenGLShaderProgram> m_convertFloatToFixedProgram; // Conversion float -> int
    std::unique_ptr<QOpenGLShaderProgram> m_convertFixedToFloatProgram; // Conversion int -> float

    // Buffer pour les particules
    GLuint m_particleBuffer = 0;
    GLuint m_modBuffer = 0;
    GLuint m_tempIntTexture = 0;
    int m_lastHeightmapSize = 0;

    // Paramètres d'érosion (valeurs par défaut)
    int m_iterations = 10;          // Nombre d'itérations d'érosion [min = 1, max = 1000, step = 1, default = 10]
    int m_numParticles = 10000;      // Nombre de particules par itération
    float m_inertia = 0.1f;          // Inertie du mouvement [min = 0, max = 1, step = 0.01, default = 0.1] [Tooltype = Plus c'est proche de 0 et plus la particule suit la pente]
    float m_sedimentCapacity = 8.0f; // Capacité de transport de sédiment [min = 0, max = 32, step = 0.1, default = 8.0] [Tooltype = Plus c'est élevé et plus la particule peut transporter de sédiment. Et donc créer plus de ravins]
    float m_depositionPercentage = 0.1f;  // Pourcentage de dépôt [min = 0, max = 1, step = 0.01, default = 0.1] [Tooltype = Plus c'est élevé et plus la particule dépose du sédiment rapidement. Attention si trop haut peut casser l'érosion (tout déposer en cas de changement brutal de vitesse)]
    float m_erosionSpeed = 0.3f;     // Vitesse d'érosion [min = 0, max = 1, step = 0.01, default = 0.1] [Tooltype = Plus c'est bas et plus l'érosion formera des ravins, en dessous de 0.1 quasiment rien est érodé]
    float m_evaporationSpeed = 0.01f;// Vitesse d'évaporation [min = 0, max = 0.5, step = 0.001, default = 0.01] [Tooltype = Plus c'est élevé et plus la particule perdra rapidement son eau et donc sédiment transporté, au-dessus de 0.5 sert plus à rien]
    float m_gravity = 4.0f;          // Gravité [Pas besoin de changer, car selon l'article ca change rien a l'apparence du terrain]
    float m_minSlope = 0.001f;        // Pente minimale [min = 0.0001, max = 0.05, step = 0.0001, default = 0.001] [Tooltype = Plus c'est élevé et plus la particule s'arrêtera rapidement sur des pentes faibles]
    int m_maxLifetime = 30;          // Durée de vie max d'une particule
    float m_initVelocity = 10.0f;
    float m_initWater = 10.0f;
    int m_erosionRadius = 1; // Rayon d'érosion autour de la particule [min = 0, max = 6, step = 1, default = 1] [Tooltype = Plus c'est grand et plus l'érosion sera douce et étalée sur une large zone]
    int m_fixedPointScale = 100000;   // Échelle pour conversion float -> int
};




#endif //CLAYAPP_TERRAINEROSION_H