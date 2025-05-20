#ifndef TYPES_H
#define TYPES_H

#include <QPointF>
#include <QString>
#include <QVector>

namespace GISMap {

/**
 * Common types used throughout the application
 */
namespace Types {

/**
 * Geographic coordinate (latitude, longitude)
 */
struct GeoPoint {
    double latitude;
    double longitude;
    
    GeoPoint() : latitude(0.0), longitude(0.0) {}
    GeoPoint(double lat, double lon) : latitude(lat), longitude(lon) {}
    
    // Convert to QPointF (lat, lon)
    QPointF toQPointF() const { return QPointF(latitude, longitude); }
    
    // Create from QPointF (lat, lon)
    static GeoPoint fromQPointF(const QPointF& point) {
        return GeoPoint(point.x(), point.y());
    }
};

/**
 * Tile coordinates (x, y, zoom)
 */
struct TileCoord {
    int x;
    int y;
    int zoom;
    
    TileCoord() : x(0), y(0), zoom(0) {}
    TileCoord(int _x, int _y, int _zoom) : x(_x), y(_y), zoom(_zoom) {}
    
    // Generate a unique string key for this tile
    QString toKey() const {
        return QString("%1/%2/%3").arg(zoom).arg(x).arg(y);
    }
};

/**
 * Feature types that can be displayed on the map
 */
enum class FeatureType {
    Point,
    Line,
    Polygon,
    Aircraft
};

/**
 * Map feature base information
 */
struct FeatureInfo {
    QString id;
    QString name;
    QString description;
    FeatureType type;
    
    FeatureInfo() : type(FeatureType::Point) {}
    FeatureInfo(const QString& _id, const QString& _name, FeatureType _type)
        : id(_id), name(_name), type(_type) {}
};

} // namespace Types

} // namespace GISMap

#endif // TYPES_H 