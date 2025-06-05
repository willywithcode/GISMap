#include "maplayer.h"

MapLayer::MapLayer(const QString& name, QObject* parent)
    : QObject(parent)
    , m_name(name)
    , m_visible(true)
    , m_opacity(1.0)
{
}

void MapLayer::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        emit layerChanged();
    }
}

void MapLayer::setOpacity(double opacity)
{
    opacity = qBound(0.0, opacity, 1.0);
    if (qAbs(m_opacity - opacity) > 0.001) {
        m_opacity = opacity;
        emit layerChanged();
    }
}

void MapLayer::setName(const QString& name)
{
    if (m_name != name) {
        m_name = name;
        emit layerChanged();
    }
}
