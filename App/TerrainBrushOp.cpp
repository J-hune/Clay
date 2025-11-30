#include "TerrainBrushOp.h"
#include "Log.h"
#include <QOpenGLShader>

static const char *computeShaderSource = R"(
#version 430 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(r32f, binding = 0) uniform image2D uHeightmap;
uniform sampler2DArray uBrushArray;

uniform int uOpType;
uniform vec3 uWorldPos;
uniform float uBrushSize;
uniform float uBrushStrength;
uniform int uBrushIndex;
uniform vec4 uTerrainBounds;
uniform float uHeightScale;
uniform int uHeightmapRes;
uniform vec3 uCameraForward;

vec2 worldToUV(vec2 worldXZ) {
    vec2 terrainMin = uTerrainBounds.xz;
    vec2 terrainMax = uTerrainBounds.yw;
    return (worldXZ - terrainMin) / (terrainMax - terrainMin);
}

vec2 uvToWorld(vec2 uv) {
    vec2 terrainMin = uTerrainBounds.xz;
    vec2 terrainMax = uTerrainBounds.yw;
    return mix(terrainMin, terrainMax, uv);
}

float sampleHeight(ivec2 coord) {
    if (coord.x < 0 || coord.x >= uHeightmapRes || coord.y < 0 || coord.y >= uHeightmapRes)
        return 0.0;
    return imageLoad(uHeightmap, coord).r;
}

void main() {
    ivec2 texCoord = ivec2(gl_GlobalInvocationID.xy);

    if (texCoord.x >= uHeightmapRes || texCoord.y >= uHeightmapRes)
        return;

    vec2 uv = (vec2(texCoord) + 0.5) / float(uHeightmapRes);
    vec2 worldXZ = uvToWorld(uv);

    vec2 brushCenter = uWorldPos.xz;
    vec2 delta = worldXZ - brushCenter;

    vec2 cameraDir = normalize(uCameraForward.xz);
    float angle = atan(cameraDir.y, cameraDir.x);

    // Matrice de rotation 2D
    float c = cos(angle);
    float s = sin(angle);
    mat2 rotation = mat2(c, -s, s, c);

    vec2 rotatedDelta = rotation * delta;

    vec2 brushUV = (rotatedDelta / uBrushSize) * 0.5 + 0.5;
    float brushMask = 0.0;
    if (brushUV.x >= 0.0 && brushUV.x <= 1.0 && brushUV.y >= 0.0 && brushUV.y <= 1.0) {
        brushMask = texture(uBrushArray, vec3(brushUV, float(uBrushIndex))).r;
    }

    float intensity = brushMask * uBrushStrength * 0.1;

    float currentHeight = sampleHeight(texCoord);
    float newHeight = currentHeight;

    if (uOpType == 0) {
        newHeight = currentHeight + intensity * 0.005;
    }
    else if (uOpType == 1) {
        newHeight = currentHeight - intensity * 0.005;
    }
    else if (uOpType == 2) {
        float avg = 0.0;
        int count = 0;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                avg += sampleHeight(texCoord + ivec2(dx, dy));
                count++;
            }
        }
        avg /= float(count);
        newHeight = mix(currentHeight, avg, intensity);
    }

    newHeight = clamp(newHeight, 0.0, 1.0);
    imageStore(uHeightmap, texCoord, vec4(newHeight, 0.0, 0.0, 0.0));
}
)";

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

    if (!m_computeProgram->addShaderFromSourceCode(QOpenGLShader::Compute, computeShaderSource)) {
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
