#pragma once
#include <QObject>
#include <QPointF>
#include <QSize>
#include <QRectF>
#include <cmath>

/**
 * @brief Handles coordinate transformations between geographic and screen coordinates
 */
class ViewTransform : public QObject {
    Q_OBJECT
public:
    explicit ViewTransform(QObject* parent = nullptr);
    ViewTransform(const QPointF& center, int zoom, const QSize& viewSize, QObject* parent = nullptr);

    // Coordinate transformation methods
    QPointF geoToScreen(const QPointF& geoPoint) const;
    QPointF screenToGeo(const QPointF& screenPoint) const;
    QRectF visibleBounds() const;
    
    // Navigation methods
    void setCenter(const QPointF& center);
    void setZoom(int zoom);
    void setViewSize(const QSize& size);
    void pan(const QPointF& deltaPx);
    void zoomIn();
    void zoomOut();
    
    // Getters
    QPointF center() const { return m_center; }
    int zoom() const { return m_zoom; }
    QSize viewSize() const { return m_viewSize; }
    
    // Utility methods
    double metersPerPixel() const;
    double pixelsPerMeter() const;

signals:
    void transformChanged();
    void centerChanged(const QPointF& center);
    void zoomChanged(int zoom);

private:
    QPointF geoToPixel(const QPointF& geoPoint) const;
    QPointF pixelToGeo(const QPointF& pixelPoint) const;
    
    QPointF m_center = QPointF(105.85, 21.03); // Default to Hanoi
    int m_zoom = 12;
    QSize m_viewSize = QSize(800, 600);
    
    static constexpr int TILE_SIZE = 256;
    static constexpr int MIN_ZOOM = 3;
    static constexpr int MAX_ZOOM = 18;
};
