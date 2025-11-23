#ifndef CLAYAPP_GRID_H
#define CLAYAPP_GRID_H

#include <QVector>
#include <QOpenGLFunctions>
#include <QtGlobal>

// Grille de référence au sol (Y=0)
class Grid {
public:
    void setResolution(int r) {
        r = qBound(1, r, 10000);
        if (m_resolution == r) return;
        m_resolution = r;
        m_dirty = true; // On marque le besoin de reconstruire
    }

    int resolution() const { return m_resolution; }

    // Ajout de paramètres pour activer/désactiver l'affichage
    void draw(QOpenGLFunctions *f, const bool drawGrid, const bool drawAxes) {
        if (m_dirty) rebuildVertices();
        const int N = m_resolution;
        const float half = static_cast<float>(N);

        glEnableClientState(GL_VERTEX_ARRAY);

        if (drawGrid) {
            // Lignes parallèles à X
            glColor3f(0.35f, 0.35f, 0.35f);
            glVertexPointer(3, GL_FLOAT, 0, m_linesX.data());
            glDrawArrays(GL_LINES, 0, m_linesX.size() / 3);

            // Lignes parallèles à Z
            glColor3f(0.30f, 0.30f, 0.30f);
            glVertexPointer(3, GL_FLOAT, 0, m_linesZ.data());
            glDrawArrays(GL_LINES, 0, m_linesZ.size() / 3);
        }

        if (drawAxes) {
            // Axes
            f->glLineWidth(2.0f);
            const GLfloat axes[] = {
                -half, 0.0f, 0.0f, half, 0.0f, 0.0f,
                0.0f, 0.0f, -half, 0.0f, 0.0f, half,
                0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f
            };
            // On dessine X
            glColor3f(1.0f, 0.2f, 0.2f);
            glVertexPointer(3, GL_FLOAT, 0, axes);
            f->glDrawArrays(GL_LINES, 0, 2);

            // On dessine Z
            glColor3f(0.2f, 0.4f, 1.0f);
            glVertexPointer(3, GL_FLOAT, 0, axes + 6);
            f->glDrawArrays(GL_LINES, 0, 2);

            // On dessine Y
            glColor3f(0.3f, 1.0f, 0.3f);
            glVertexPointer(3, GL_FLOAT, 0, axes + 12);
            f->glDrawArrays(GL_LINES, 0, 2);
            f->glLineWidth(1.0f);
        }

        glDisableClientState(GL_VERTEX_ARRAY);
    }

    // Vérifie si une position (x,z) est dans la grille
    bool contains(float x, float z) const {
        const float half = static_cast<float>(m_resolution) * m_step;
        return (x >= -half && x <= half && z >= -half && z <= half);
    }

    // Par défaut planaire à 0.0f. Si présence d'une heightmap, on pourrait ajuster.
    float heightAt(float /*x*/, float /*z*/) const {
        return 0.0f;
    }

private:
    void rebuildVertices() {
        // On reconstruit les deux ensembles de lignes
        const int N = m_resolution;
        constexpr float step = 1.0f;
        m_step = step;
        const int lineCount = 2 * N + 1;

        m_linesX.clear(); m_linesX.reserve(lineCount * 6);
        for (int i = -N; i <= N; ++i) {
            float z = static_cast<float>(i) * step;
            m_linesX.push_back(-N * step); m_linesX.push_back(0.0f); m_linesX.push_back(z);
            m_linesX.push_back( N * step); m_linesX.push_back(0.0f); m_linesX.push_back(z);
        }

        m_linesZ.clear(); m_linesZ.reserve(lineCount * 6);
        for (int i = -N; i <= N; ++i) {
            float x = static_cast<float>(i) * step;
            m_linesZ.push_back(x); m_linesZ.push_back(0.0f); m_linesZ.push_back(-N * step);
            m_linesZ.push_back(x); m_linesZ.push_back(0.0f); m_linesZ.push_back( N * step);
        }

        m_dirty = false;
    }

    int m_resolution = 100;
    bool m_dirty = true;
    QVector<float> m_linesX; // On stocke les sommets des lignes X
    QVector<float> m_linesZ; // On stocke les sommets des lignes Z
    // step (distance entre lignes)
    float m_step = 1.0f;
};

#endif // CLAYAPP_GRID_H
