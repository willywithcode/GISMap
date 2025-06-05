#pragma once
#include "../core/geometryobject.h"
#include <QPolygonF>
#include <QColor>
#include <QPen>
#include <QBrush>

/**
 * @brief Represents a polygon geometry object
 */
class PolygonObject : public GeometryObject {
    Q_OBJECT
public:
    explicit PolygonObject(const QPolygonF& polygon, QObject* parent = nullptr);
    explicit PolygonObject(QObject* parent = nullptr);

    // GeometryObject interface
    void render(QPainter& painter, const ViewTransform& transform) override;
    bool containsPoint(const QPointF& geoPoint) override;
    QRectF boundingBox() const override;
    QString getInfo() const override;

    // Polygon-specific methods
    void setPolygon(const QPolygonF& polygon);
    QPolygonF polygon() const { return m_polygon; }
    
    void setFillColor(const QColor& color);
    void setBorderColor(const QColor& color);
    void setBorderWidth(int width);
    
    QColor fillColor() const { return m_fillColor; }
    QColor borderColor() const { return m_borderColor; }
    int borderWidth() const { return m_borderWidth; }

protected:
    virtual void updateStyle();

private:
    QPolygonF m_polygon;
    QColor m_fillColor = QColor(255, 0, 0, 100); // Semi-transparent red
    QColor m_borderColor = Qt::red;
    int m_borderWidth = 2;
};
