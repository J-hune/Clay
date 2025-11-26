#include "TerrainRaycast.h"
#include <QOpenGLShader>
#include "Log.h"

static const char *computeShaderSource = R"(
#version 430 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

// SSBO pour le résultat
layout(std430, binding = 0) buffer ResultBuffer {
    vec4 worldPos;       // position mondiale (w non utilisé)
    vec4 worldNormal;    // normale (w non utilisé)
    float hitFlag;       // 1.0 si hit, 0.0 sinon
    float pad1, pad2, pad3;
};

// Heightmap
uniform sampler2D uHeightmap;

// Paramètres
uniform vec2 uMouseNDC;          // Position souris en NDC [-1,1]
uniform mat4 uInvViewProj;       // Inverse(Projection * View)
uniform float uHeightScale;
uniform int uHeightmapRes;
uniform vec4 uTerrainBounds;     // (minX, maxX, minZ, maxZ)

// Fonction pour obtenir la hauteur du terrain à une position XZ donnée
float getTerrainHeight(vec2 xz) {
    vec2 terrainMin = uTerrainBounds.xz;
    vec2 terrainMax = uTerrainBounds.yw;

    vec2 uv = (xz - terrainMin) / (terrainMax - terrainMin);

    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
        return -1e8;

    float h = texture(uHeightmap, uv).r;
    return h * uHeightScale;
}

// Calcule la normale au point XZ
vec3 computeNormal(vec2 xz) {
    // delta basé sur la résolution → stable
    float terrainSizeX = uTerrainBounds.y - uTerrainBounds.x;
    float terrainSizeZ = uTerrainBounds.w - uTerrainBounds.z;
    float delta = (terrainSizeX + terrainSizeZ) * 0.5 / float(uHeightmapRes);

    float hL = getTerrainHeight(xz + vec2(-delta, 0.0));
    float hR = getTerrainHeight(xz + vec2(delta, 0.0));
    float hD = getTerrainHeight(xz + vec2(0.0, -delta));
    float hU = getTerrainHeight(xz + vec2(0.0, delta));

    vec3 tangentX = vec3(2.0 * delta, hR - hL, 0.0);
    vec3 tangentZ = vec3(0.0, hU - hD, 2.0 * delta);

    return normalize(cross(tangentZ, tangentX));
}

void main() {
    // On reconstruit le rayon depuis la position NDC de la souris
    vec4 nearPoint = uInvViewProj * vec4(uMouseNDC, -1.0, 1.0);
    nearPoint /= nearPoint.w;

    vec4 farPoint = uInvViewProj * vec4(uMouseNDC, 1.0, 1.0);
    farPoint /= farPoint.w;

    vec3 rayOrigin = nearPoint.xyz;
    vec3 rayDir = normalize(farPoint.xyz - nearPoint.xyz);

    const int maxSteps = 512;
    const float maxDist = 1000.0;
    const int binarySteps = 10;

    float stepSize = maxDist / float(maxSteps);
    float t = 0.0;
    bool hit = false;
    vec3 hitPos = vec3(0.0);

    // early check si le rayon démarre dans le terrain
    float h0 = getTerrainHeight(rayOrigin.xz);
    if (rayOrigin.y <= h0) {
        hit = true;
        hitPos = rayOrigin;
    }

    // Raymarch
    if (!hit) {
        for (int i = 0; i < maxSteps; i++) {
            vec3 p = rayOrigin + rayDir * t;
            float terrainH = getTerrainHeight(p.xz);

            if (terrainH < -9e7) { // sentinel
                t += stepSize;
                if (t > maxDist) break;
                continue;
            }

            if (p.y <= terrainH) {
                float tMin = t - stepSize;
                float tMax = t;

                for (int j = 0; j < binarySteps; j++) {
                    float tMid = 0.5 * (tMin + tMax);
                    vec3 pMid = rayOrigin + rayDir * tMid;
                    float hMid = getTerrainHeight(pMid.xz);

                    if (pMid.y > hMid)
                        tMin = tMid;
                    else
                        tMax = tMid;
                }

                t = 0.5 * (tMin + tMax);
                hitPos = rayOrigin + rayDir * t;
                hit = true;
                break;
            }

            t += stepSize;
            if (t > maxDist) break;
        }
    }

    // Écrire le résultat dans le SSBO
    if (hit) {
        worldPos = vec4(hitPos, 1.0);
        worldNormal = vec4(computeNormal(hitPos.xz), 0.0);
        hitFlag = 1.0;
    } else {
        worldPos = vec4(0.0);
        worldNormal = vec4(0.0, 1.0, 0.0, 0.0);
        hitFlag = 0.0;
    }
}
)";

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

    if (!m_computeProgram->addShaderFromSourceCode(QOpenGLShader::Compute, computeShaderSource)) {
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

