#ifndef FLIGHTROUTE_H
#define FLIGHTROUTE_H

#include <QObject>
#include <QVector>
#include <QPointF>
#include <QColor>
#include <QString>
#include <QDateTime>

/**
 * @brief FlightRoute class represents a flight path between waypoints
 * 
 * This class handles flight route visualization, waypoint management,
 * and route calculations for aircraft navigation.
 */
class FlightRoute : public QObject
{
    Q_OBJECT

public:
    enum RouteType {
        Departure,      // Đường bay cất cánh
        Arrival,        // Đường bay hạ cánh
        Transit,        // Đường bay quá cảnh
        Emergency       // Đường bay khẩn cấp
    };

    struct Waypoint {
        QPointF position;       // Lat/Lon coordinates
        QString name;           // Waypoint identifier (e.g., "VTUD", "WP001")
        double altitude;        // Altitude in meters
        QDateTime estimatedTime; // ETA at this waypoint
        QString description;    // Optional description
    };

    explicit FlightRoute(QObject *parent = nullptr);
    explicit FlightRoute(const QString& routeId, RouteType type, QObject *parent = nullptr);

    // Route identification
    QString getRouteId() const { return m_routeId; }
    void setRouteId(const QString& id) { m_routeId = id; }

    RouteType getRouteType() const { return m_routeType; }
    void setRouteType(RouteType type) { m_routeType = type; }

    // Waypoint management
    void addWaypoint(const Waypoint& waypoint);
    void addWaypoint(const QPointF& position, const QString& name = QString());
    void insertWaypoint(int index, const Waypoint& waypoint);
    void removeWaypoint(int index);
    void clearWaypoints();

    QVector<Waypoint> getWaypoints() const { return m_waypoints; }
    Waypoint getWaypoint(int index) const;
    int waypointCount() const { return m_waypoints.size(); }

    // Route properties
    QVector<QPointF> getRoutePoints() const;
    double getTotalDistance() const;
    QDateTime getEstimatedDuration() const;

    // Visual properties
    QColor getRouteColor() const { return m_color; }
    void setRouteColor(const QColor& color) { m_color = color; }

    int getRouteWidth() const { return m_width; }
    void setRouteWidth(int width) { m_width = width; }

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }

    // Route status
    bool isActive() const { return m_active; }
    void setActive(bool active) { m_active = active; }

    QString getDescription() const { return m_description; }
    void setDescription(const QString& description) { m_description = description; }

    // Database operations
    void saveToDatabase();
    void loadFromDatabase(const QString& routeId);
    void deleteFromDatabase();

    // Utility functions
    static double calculateDistance(const QPointF& point1, const QPointF& point2);
    static QPointF interpolatePoint(const QPointF& start, const QPointF& end, double ratio);

signals:
    void routeChanged();
    void waypointAdded(int index);
    void waypointRemoved(int index);
    void routePropertiesChanged();

private:
    QString m_routeId;
    RouteType m_routeType;
    QVector<Waypoint> m_waypoints;
    
    // Visual properties
    QColor m_color;
    int m_width;
    bool m_visible;
    bool m_active;
    QString m_description;

    void updateRouteMetrics();
    void createDefaultRoute();
};

Q_DECLARE_METATYPE(FlightRoute::RouteType)
Q_DECLARE_METATYPE(FlightRoute::Waypoint)

#endif // FLIGHTROUTE_H 