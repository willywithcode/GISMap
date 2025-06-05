#include "viewtransform.h"
#include <QDebug>

ViewTransform::ViewTransform(QObject* parent) 
    : QObject(parent) {
}

ViewTransform::ViewTransform(const QPointF& center, int zoom, const QSize& viewSize, QObject* parent)
    : QObject(parent), m_center(center), m_zoom(zoom), m_viewSize(viewSize) {
}

QPointF ViewTransform::geoToScreen(const QPointF& geoPoint) const {
    QPointF pixelPoint = geoToPixel(geoPoint);
    QPointF centerPixel = geoToPixel(m_center);
    
    // Translate to screen coordinates relative to center
    QPointF screenPoint = pixelPoint - centerPixel + QPointF(m_viewSize.width()/2.0, m_viewSize.height()/2.0);
    return screenPoint;
}

QPointF ViewTransform::screenToGeo(const QPointF& screenPoint) const {
    QPointF centerPixel = geoToPixel(m_center);
    QPointF pixelPoint = screenPoint - QPointF(m_viewSize.width()/2.0, m_viewSize.height()/2.0) + centerPixel;
    return pixelToGeo(pixelPoint);
}

QRectF ViewTransform::visibleBounds() const {
    QPointF topLeft = screenToGeo(QPointF(0, 0));
    QPointF bottomRight = screenToGeo(QPointF(m_viewSize.width(), m_viewSize.height()));
    return QRectF(topLeft, bottomRight);
}

void ViewTransform::setCenter(const QPointF& center) {
    if (m_center != center) {
        m_center = center;
        emit centerChanged(center);
        emit transformChanged();
    }
}

void ViewTransform::setZoom(int zoom) {
    int clampedZoom = qBound(MIN_ZOOM, zoom, MAX_ZOOM);
    if (m_zoom != clampedZoom) {
        m_zoom = clampedZoom;
        emit zoomChanged(clampedZoom);
        emit transformChanged();
    }
}

void ViewTransform::setViewSize(const QSize& size) {
    if (m_viewSize != size) {
        m_viewSize = size;
        emit transformChanged();
    }
}

void ViewTransform::pan(const QPointF& deltaPx) {
    // Convert pixel delta to geographic delta
    QPointF startGeo = screenToGeo(QPointF(0, 0));
    QPointF endGeo = screenToGeo(deltaPx);
    QPointF geoOffset = endGeo - startGeo;
    
    setCenter(m_center - geoOffset);
}

void ViewTransform::zoomIn() {
    setZoom(m_zoom + 1);
}

void ViewTransform::zoomOut() {
    setZoom(m_zoom - 1);
}

double ViewTransform::metersPerPixel() const {
    // Approximation at current latitude
    double latRad = m_center.y() * M_PI / 180.0;
    double earthCircumference = 40075016.686; // meters
    double metersPerDegree = earthCircumference / 360.0;
    double metersPerDegreeLon = metersPerDegree * cos(latRad);
    
    double pixelsPerDegree = (1 << m_zoom) * TILE_SIZE / 360.0;
    return metersPerDegreeLon / pixelsPerDegree;
}

double ViewTransform::pixelsPerMeter() const {
    return 1.0 / metersPerPixel();
}

QPointF ViewTransform::geoToPixel(const QPointF& geoPoint) const {
    // Web Mercator projection
    double x = (geoPoint.x() + 180.0) / 360.0 * (1 << m_zoom) * TILE_SIZE;
    double latRad = geoPoint.y() * M_PI / 180.0;
    double y = (1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / M_PI) / 2.0 * (1 << m_zoom) * TILE_SIZE;
    return QPointF(x, y);
}

QPointF ViewTransform::pixelToGeo(const QPointF& pixelPoint) const {
    // Inverse Web Mercator projection
    double lon = (pixelPoint.x() / TILE_SIZE) / (1 << m_zoom) * 360.0 - 180.0;
    double latRad = atan(sinh(M_PI * (1 - 2 * (pixelPoint.y() / TILE_SIZE) / (1 << m_zoom))));
    double lat = latRad * 180.0 / M_PI;
    return QPointF(lon, lat);
}
