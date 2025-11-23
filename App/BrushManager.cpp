#include "BrushManager.h"



#include "BrushManager.h"
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QVector3D>

#include "Camera.h"

BrushManager::BrushManager(QObject *parent) : QObject(parent) {
    scanBrushFolder();
    if (!m_brushPaths.isEmpty()) {
        loadBrushTexture(m_brushPaths.first());
    }
}

void BrushManager::refresh() {
    scanBrushFolder();
}

void BrushManager::scanBrushFolder() {
    // dossier relatif à l'exe : <appdir>/brushes
    const QString base = QCoreApplication::applicationDirPath();
    QDir d(base + "/brushes");
    if (!d.exists()) {
        d.mkpath(".");
    }
    const QStringList nameFilters = { "*.png", "*.jpg", "*.jpeg", "*.bmp" };
    const QFileInfoList files = d.entryInfoList(nameFilters, QDir::Files, QDir::Name);
    QStringList paths;
    for (const QFileInfo &fi : files) paths << fi.absoluteFilePath();
    m_brushPaths = paths;
    emit brushesChanged();
}

void BrushManager::updateAlphaFromLuminance() {
    if (m_brushImage.isNull()) return;

    for (int y = 0; y < m_brushImage.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(m_brushImage.scanLine(y));
        for (int x = 0; x < m_brushImage.width(); ++x) {
            QColor c = QColor::fromRgba(line[x]);

            // luminance (0 = noir, 255 = blanc)
            float lum = 0.2126f * c.red() + 0.7152f * c.green() + 0.0722f * c.blue();
            float alpha = lum / 255.0f; // noir = 0, blanc = 1

            alpha *= (m_brushStrength / 100.f);
            if (alpha > 1.f) alpha = 1.f;
            else if (alpha < 0.f) alpha = 0.f;
            c.setAlphaF(alpha); // on applique la force de la brosse

            line[x] = c.rgba();
        }
    }
    // on marque qu'il faut uploader la texture dans le thread GL
    m_needsUpload.store(true, std::memory_order_release);
}

void BrushManager::loadBrushTexture(const QString &newPath) {
    if (newPath == m_selectedBrushPath) return;
    m_selectedBrushPath = newPath;

    // détruit ancienne image CPU
    m_brushImage = QImage();

    if (!m_selectedBrushPath.isEmpty()) {
        QImage img(m_selectedBrushPath);
        if (!img.isNull()) {
            // convertir pour format RGBA
            QImage conv = img.convertToFormat(QImage::Format_RGBA8888);
            if (conv.size() != QSize(128, 128))
                conv = conv.scaled(128, 128, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

            m_brushImage = conv;
            // on convertit en masque alpha selon la luminance (noir = transparent, blanc = opaque)
            for (int y = 0; y < m_brushImage.height(); ++y) {
                QRgb *line = reinterpret_cast<QRgb*>(m_brushImage.scanLine(y));
                for (int x = 0; x < m_brushImage.width(); ++x) {
                    QColor c = QColor::fromRgba(line[x]);

                    // luminance (0 = noir, 255 = blanc)
                    float lum = 0.2126f * c.red() + 0.7152f * c.green() + 0.0722f * c.blue();
                    float alpha = lum / 255.0f; // noir = 0, blanc = 1
                    alpha *= (m_brushStrength / 100.f);
                    if (alpha > 1.f) alpha = 1.f;
                    else if (alpha < 0.f) alpha = 0.f;
                    c.setAlphaF(alpha); // on applique la force de la brosse
                    line[x] = c.rgba();
                }
            }
            // on marque qu'il faut uploader la texture dans le thread GL
            m_needsUpload.store(true, std::memory_order_release);
        } else {
            qWarning() << "[BrushManager] Failed to load image:" << m_selectedBrushPath;
        }
    } else {
        // clear selection: supprimer texture GL existante lors du prochain upload
        m_needsUpload.store(true, std::memory_order_release);
    }
}


void BrushManager::uploadTextureIfNeeded() {
    // check et reset atomique
    bool expected = true;
    if (!m_needsUpload.compare_exchange_strong(expected, false, std::memory_order_acq_rel)) {
        // rien à faire
        return;
    }

    // si on doit supprimer l'ancienne texture (selection vide), on le fait
    if (m_selectedBrushPath.isEmpty()) {
        if (m_brushTex) {
            glDeleteTextures(1, &m_brushTex);
            m_brushTex = 0;
        }
        m_brushImage = QImage();
        return;
    }

    // on a une image CPU prête -> upload en GL
    if (m_brushImage.isNull()) {
        qWarning() << "[BrushManager] upload requested but m_brushImage is null for" << m_selectedBrushPath;
        return;
    }

    // supprime texture précédente si existante
    if (m_brushTex) {
        glDeleteTextures(1, &m_brushTex);
        m_brushTex = 0;
    }

    glGenTextures(1, &m_brushTex);
    glBindTexture(GL_TEXTURE_2D, m_brushTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // QImage::constBits() renvoie les pixels (Format_RGBA8888)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_brushImage.width(), m_brushImage.height(), 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, m_brushImage.constBits());

    glBindTexture(GL_TEXTURE_2D, 0);
}

void BrushManager::drawPlaceholder(QVector3D hitPoint, const Camera &camera) {
    const float eps = 0.001f;
    const float y = hitPoint.y() + eps;
    const float radius = m_brushSize;

    // Si on a une texture GL, on l'affiche sur un quad orienté selon la caméra
    if (m_brushTex) {
        // Calcul d'axes pour le quad : on veut que le quad reste sur la surface (plan horizontal)
        // mais tourne pour suivre la direction de la caméra projetée sur XZ.
        const QVector3D worldUp(0.f, 1.f, 0.f);

        QVector3D camForward = camera.frontVector();
        // projeter sur XZ
        camForward.setY(0.f);
        if (qFuzzyIsNull(camForward.lengthSquared())) {
            // fallback si la projection est nulle (cam exactement vertical)
            camForward = QVector3D(0.f, 0.f, -1.f);
        } else {
            camForward.normalize();
        }

        QVector3D right = QVector3D::crossProduct(camForward, worldUp).normalized();
        QVector3D forward = QVector3D::crossProduct(worldUp, right).normalized();
        // forward et right forment une base sur le plan XZ orientée vers la caméra

        // coin bas-gauche, bas-droite, haut-droite, haut-gauche (dans l'ordre pour GL_QUADS)
        const QVector3D v0 = hitPoint - right * radius - forward * radius; // -x -z
        const QVector3D v1 = hitPoint + right * radius - forward * radius; // +x -z
        const QVector3D v2 = hitPoint + right * radius + forward * radius; // +x +z
        const QVector3D v3 = hitPoint - right * radius + forward * radius; // -x +z

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, m_brushTex);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBegin(GL_QUADS);
            glColor4f(1,1,1,1);
            glTexCoord2f(0.f, 1.f); glVertex3f(v0.x(), y, v0.z());
            glTexCoord2f(1.f, 1.f); glVertex3f(v1.x(), y, v1.z());
            glTexCoord2f(1.f, 0.f); glVertex3f(v2.x(), y, v2.z());
            glTexCoord2f(0.f, 0.f); glVertex3f(v3.x(), y, v3.z());
        glEnd();

        glDisable(GL_BLEND);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    } else {
        // fallback circle
        const int segs = 32;
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(0.529f, 0.808f, 0.922f, 0.5f);
        glVertex3f(hitPoint.x(), y, hitPoint.z());
        for (int i = 0; i <= segs; ++i) {
            const float a = (static_cast<float>(i) / static_cast<float>(segs)) * (M_PI * 2.0f);
            const float cx = hitPoint.x() + qCos(a) * radius;
            const float cz = hitPoint.z() + qSin(a) * radius;
            glVertex3f(cx, y, cz);
        }
        glEnd();
        glDisable(GL_BLEND);
    }
}


bool BrushManager::screenToPlane(const QPoint &screenPos, int viewW, int viewH,
                                   const Camera &camera, float fovDeg, const Grid *grid,
                                   QVector3D &outHit) const{
    if (viewW <= 0 || viewH <= 0) return false;

    const QVector3D camPos = camera.position();
    const QVector3D forward = camera.frontVector().normalized();
    const QVector3D worldUp(0.f, 1.f, 0.f);
    QVector3D right = QVector3D::crossProduct(forward, worldUp).normalized();
    QVector3D up = QVector3D::crossProduct(right, forward).normalized();

    const float nx = (2.0f * static_cast<float>(screenPos.x()) / static_cast<float>(viewW) - 1.0f);
    const float ny = (1.0f - 2.0f * static_cast<float>(screenPos.y()) / static_cast<float>(viewH));
    const float aspect = static_cast<float>(viewW) / static_cast<float>(viewH);
    const float tanHalf = qTan(qDegreesToRadians(fovDeg * 0.5f));

    QVector3D rayDir = (right * (nx * aspect * tanHalf) +
                        up    * (ny * tanHalf) +
                        forward).normalized();

    // Si pas de Grid fourni, fallback sur plan horizontal constant
    if (grid == nullptr) {
        if (qFuzzyIsNull(rayDir.y())) return false;
        float t = (m_planeY - camPos.y()) / rayDir.y();
        if (t <= 0.f) return false;
        outHit = camPos + rayDir * t;
        return true;
    }

    // --- Avec Grid : on fait un ray-marching simple pour trouver intersection avec heightAt(x,z)
    if (rayDir.y() >= 0.0f && camPos.y() < grid->heightAt(camPos.x(), camPos.z())) {
        // caméra en-dessous du terrain et rayon monte -> possible gros cas, mais on peut continuer
    }
    const float tMin = 0.01f;
    const float  tMax = 1000.0f; // distance max de recherche
    const float sampleStep = 0.5f; // pas initial

    float t = tMin;
    float prevDiff = (camPos.y() + rayDir.y() * t) - grid->heightAt(camPos.x() + rayDir.x()*t, camPos.z() + rayDir.z()*t);
    bool found = false;
    float tLow = tMin, tHigh = tMin;

    while (t <= tMax) {
        const float px = camPos.x() + rayDir.x() * t;
        const float pz = camPos.z() + rayDir.z() * t;

        // si hors grille, on abandonne (on veut que le placeholder n'apparaisse pas en dehors)
        if (!grid->contains(px, pz)) {
            return false;
        }

        const float terrainY = grid->heightAt(px, pz);
        const float diff = (camPos.y() + rayDir.y() * t) - terrainY;

        // sign change => intersection entre previous t and current t
        if (diff == 0.0f) {
            outHit = QVector3D(px, terrainY, pz);
            return true;
        }
        if (prevDiff * diff < 0.0f) {
            // crossing
            found = true;
            tLow = t - sampleStep;
            tHigh = t;
            break;
        }
        prevDiff = diff;
        t += sampleStep;
    }

    if (!found) return false;

    // refinement binaire pour précision
    for (int iter = 0; iter < 12; ++iter) {
        const float tm = 0.5f * (tLow + tHigh);
        const float mx = camPos.x() + rayDir.x() * tm;
        const float mz = camPos.z() + rayDir.z() * tm;

        if (!grid->contains(mx, mz)) return false;

        const float my = grid->heightAt(mx, mz);
        const float diff = (camPos.y() + rayDir.y() * tm) - my;
        if (diff > 0.0f) tLow = tm; else tHigh = tm;
    }

    const float tFinal = 0.5f * (tLow + tHigh);
    const float fx = camPos.x() + rayDir.x() * tFinal;
    const float fz = camPos.z() + rayDir.z() * tFinal;
    const float fy = grid->heightAt(fx, fz);
    outHit = QVector3D(fx, fy, fz);
    return true;
}