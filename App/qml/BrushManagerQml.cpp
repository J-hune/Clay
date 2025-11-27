#include "BrushManagerQml.h"

BrushManagerQml::BrushManagerQml(QObject *parent)
    : QObject(parent) {
}

void BrushManagerQml::setBrushIndex(int index) {
    if (m_manager.currentBrushIndex() == index) return;
    m_manager.setCurrentBrushIndex(index);
    emit brushIndexChanged();
}

void BrushManagerQml::setBrushSize(float size) {
    if (qFuzzyCompare(m_manager.brushSize(), size)) return;
    m_manager.setBrushSize(size);
    emit brushSizeChanged();
}

void BrushManagerQml::setBrushStrength(float strength) {
    if (qFuzzyCompare(m_manager.brushStrength(), strength)) return;
    m_manager.setBrushStrength(strength);
    emit brushStrengthChanged();
}

void BrushManagerQml::enqueueStroke(const QVector3D &worldPos) {
    m_manager.enqueueStroke(worldPos);
}
