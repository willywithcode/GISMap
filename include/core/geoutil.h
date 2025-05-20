#ifndef GEOUTIL_H
#define GEOUTIL_H

#include <QPointF>

namespace GISMap {
namespace Core {

/**
 * Geographic utility functions for coordinate conversion
 * and GIS operations
 */
class GeoUtil
{
public:
    /**
     * Converts latitude/longitude to pixel coordinates on the map
     * @param lat Latitude
     * @param lon Longitude
     * @param zoom Zoom level
     * @param tileSize Tile size in pixels
     * @return Pixel coordinates (x, y)
     */
    static QPointF geoToPixel(double lat, double lon, int zoom, int tileSize = 256);
    
    /**
     * Converts pixel coordinates to latitude/longitude
     * @param x X coordinate
     * @param y Y coordinate
     * @param zoom Zoom level
     * @param tileSize Tile size in pixels
     * @return Geographic coordinates (latitude, longitude)
     */
    static QPointF pixelToGeo(double x, double y, int zoom, int tileSize = 256);
    
    /**
     * Converts latitude and zoom level to scale factor
     * @param lat Latitude
     * @param zoom Zoom level
     * @return Scale factor
     */
    static double getScaleFactor(double lat, int zoom);
    
    /**
     * Calculates distance between two geographic points (Haversine formula)
     * @param lat1 Latitude of first point
     * @param lon1 Longitude of first point
     * @param lat2 Latitude of second point
     * @param lon2 Longitude of second point
     * @return Distance in kilometers
     */
    static double distanceGeo(double lat1, double lon1, double lat2, double lon2);
    
    /**
     * Checks if a point is inside a polygon
     * @param point The point to check
     * @param polygon Vector of polygon points
     * @return True if point is inside the polygon
     */
    static bool isPointInPolygon(const QPointF& point, const QVector<QPointF>& polygon);
};

} // namespace Core
} // namespace GISMap

#endif // GEOUTIL_H 