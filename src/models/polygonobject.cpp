#include "polygonobject.h"
#include "../core/viewtransform.h"
#include <QPainter>
#include <QPen>
#include <QBrush>

PolygonObject::PolygonObject(const QPolygonF& polygon, QObject* parent)
    : GeometryObject(parent), m_polygon(polygon) {
}

PolygonObject::PolygonObject(QObject* parent)
    : GeometryObject(parent) {
}

void PolygonObject::render(QPainter& painter, const ViewTransform& transform) {
    if (!isVisible() || m_polygon.isEmpty()) {
        return;
    }

    // Transform polygon to screen coordinates
    QPolygonF screenPolygon;
    for (const QPointF& geoPoint : m_polygon) {
        screenPolygon << transform.geoToScreen(geoPoint);
    }

    // Set up painter
    painter.setRenderHint(QPainter::Antialiasing);
    
    QColor fillColor = m_fillColor;
    QColor borderColor = m_borderColor;
    int borderWidth = m_borderWidth;
    
    // Modify appearance if selected
    if (isSelected()) {
        borderWidth += 2;
        borderColor = borderColor.lighter(150);
    }
    
    painter.setPen(QPen(borderColor, borderWidth));
    painter.setBrush(QBrush(fillColor));
    
    // Draw the polygon
    painter.drawPolygon(screenPolygon);
}

bool PolygonObject::containsPoint(const QPointF& geoPoint) {
    return m_polygon.containsPoint(geoPoint, Qt::OddEvenFill);
}

QRectF PolygonObject::boundingBox() const {
    return m_polygon.boundingRect();
}

QString PolygonObject::getInfo() const {
    QRectF bounds = boundingBox();
    return QString("Polygon (ID: %1)\nPoints: %2\nBounds: %3, %4 to %5, %6")
           .arg(getId())
           .arg(m_polygon.size())
           .arg(bounds.left(), 0, 'f', 4)
           .arg(bounds.top(), 0, 'f', 4)
           .arg(bounds.right(), 0, 'f', 4)
           .arg(bounds.bottom(), 0, 'f', 4);
}

void PolygonObject::setPolygon(const QPolygonF& polygon) {
    if (m_polygon != polygon) {
        m_polygon = polygon;
        emit objectChanged();
    }
}

void PolygonObject::setFillColor(const QColor& color) {
    if (m_fillColor != color) {
        m_fillColor = color;
        updateStyle();
        emit objectChanged();
    }
}

void PolygonObject::setBorderColor(const QColor& color) {
    if (m_borderColor != color) {
        m_borderColor = color;
        updateStyle();
        emit objectChanged();
    }
}

void PolygonObject::setBorderWidth(int width) {
    if (m_borderWidth != width) {
        m_borderWidth = width;
        updateStyle();
        emit objectChanged();
    }
}

void PolygonObject::updateStyle() {
    // Can be overridden by subclasses for custom styling
}
