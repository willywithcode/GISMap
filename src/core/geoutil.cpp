#include "core/geoutil.h"
#include <QtMath>

namespace GISMap {
namespace Core {

QPointF GeoUtil::geoToPixel(double lat, double lon, int zoom, int tileSize)
{
    const double pi = M_PI;
    
    double latRad = lat * pi / 180.0;
    double n = qPow(2.0, zoom);
    
    double x = ((lon + 180.0) / 360.0) * n * tileSize;
    double y = (1.0 - qLn(qTan(latRad) + 1.0 / qCos(latRad)) / pi) * n * tileSize / 2.0;
    
    return QPointF(x, y);
}

QPointF GeoUtil::pixelToGeo(double x, double y, int zoom, int tileSize)
{
    const double pi = M_PI;
    
    double n = qPow(2.0, zoom);
    
    double lon = x / (n * tileSize) * 360.0 - 180.0;
    double latRad = qAtan(qSinh(pi * (1 - 2 * y / (n * tileSize))));
    double lat = latRad * 180.0 / pi;
    
    return QPointF(lat, lon);
}

double GeoUtil::getScaleFactor(double lat, int zoom)
{
    return qCos(lat * M_PI / 180.0) * 2 * M_PI * 6378137 / (256 * qPow(2.0, zoom));
}

double GeoUtil::distanceGeo(double lat1, double lon1, double lat2, double lon2)
{
    const double earthRadius = 6371.0;
    
    double lat1Rad = lat1 * M_PI / 180.0;
    double lon1Rad = lon1 * M_PI / 180.0;
    double lat2Rad = lat2 * M_PI / 180.0;
    double lon2Rad = lon2 * M_PI / 180.0;
    
    double dLat = lat2Rad - lat1Rad;
    double dLon = lon2Rad - lon1Rad;
    double a = qSin(dLat / 2) * qSin(dLat / 2) +
               qCos(lat1Rad) * qCos(lat2Rad) *
               qSin(dLon / 2) * qSin(dLon / 2);
    double c = 2 * qAtan2(qSqrt(a), qSqrt(1 - a));
    double distance = earthRadius * c;
    
    return distance;
}

bool GeoUtil::isPointInPolygon(const QPointF& point, const QVector<QPointF>& polygon)
{
    if (polygon.size() < 3) {
        return false;
    }
    
    bool inside = false;
    int i, j;
    for (i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        if (((polygon[i].y() > point.y()) != (polygon[j].y() > point.y())) &&
            (point.x() < (polygon[j].x() - polygon[i].x()) * (point.y() - polygon[i].y()) / 
             (polygon[j].y() - polygon[i].y()) + polygon[i].x())) {
            inside = !inside;
        }
    }
    
    return inside;
}

}
} 