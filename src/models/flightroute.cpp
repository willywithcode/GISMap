#include "flightroute.h"
#include "../core/configmanager.h"
#include <QtMath>
#include <QDebug>
#include <pqxx/pqxx>

FlightRoute::FlightRoute(QObject *parent)
    : QObject(parent)
    , m_routeType(Transit)
    , m_color(Qt::blue)
    , m_width(2)
    , m_visible(true)
    , m_active(false)
{
    m_routeId = QString("ROUTE_%1").arg(QDateTime::currentMSecsSinceEpoch());
    createDefaultRoute();
}

FlightRoute::FlightRoute(const QString& routeId, RouteType type, QObject *parent)
    : QObject(parent)
    , m_routeId(routeId)
    , m_routeType(type)
    , m_color(Qt::blue)
    , m_width(2)
    , m_visible(true)
    , m_active(false)
{
    // Set default colors based on route type
    switch (type) {
        case Departure:
            m_color = Qt::green;
            break;
        case Arrival:
            m_color = Qt::red;
            break;
        case Transit:
            m_color = Qt::blue;
            break;
        case Emergency:
            m_color = Qt::magenta;
            m_width = 3;
            break;
    }
}

void FlightRoute::addWaypoint(const Waypoint& waypoint)
{
    m_waypoints.append(waypoint);
    updateRouteMetrics();
    emit waypointAdded(m_waypoints.size() - 1);
    emit routeChanged();
}

void FlightRoute::addWaypoint(const QPointF& position, const QString& name)
{
    Waypoint waypoint;
    waypoint.position = position;
    waypoint.name = name.isEmpty() ? QString("WP%1").arg(m_waypoints.size() + 1) : name;
    waypoint.altitude = 10000; // Default 10km altitude
    waypoint.estimatedTime = QDateTime::currentDateTime().addSecs(m_waypoints.size() * 600); // 10 min intervals
    
    addWaypoint(waypoint);
}

void FlightRoute::insertWaypoint(int index, const Waypoint& waypoint)
{
    if (index >= 0 && index <= m_waypoints.size()) {
        m_waypoints.insert(index, waypoint);
        updateRouteMetrics();
        emit waypointAdded(index);
        emit routeChanged();
    }
}

void FlightRoute::removeWaypoint(int index)
{
    if (index >= 0 && index < m_waypoints.size()) {
        m_waypoints.removeAt(index);
        updateRouteMetrics();
        emit waypointRemoved(index);
        emit routeChanged();
    }
}

void FlightRoute::clearWaypoints()
{
    m_waypoints.clear();
    emit routeChanged();
}

FlightRoute::Waypoint FlightRoute::getWaypoint(int index) const
{
    if (index >= 0 && index < m_waypoints.size()) {
        return m_waypoints[index];
    }
    return Waypoint();
}

QVector<QPointF> FlightRoute::getRoutePoints() const
{
    QVector<QPointF> points;
    for (const auto& waypoint : m_waypoints) {
        points.append(waypoint.position);
    }
    return points;
}

double FlightRoute::getTotalDistance() const
{
    double totalDistance = 0.0;
    for (int i = 1; i < m_waypoints.size(); ++i) {
        totalDistance += calculateDistance(m_waypoints[i-1].position, m_waypoints[i].position);
    }
    return totalDistance;
}

QDateTime FlightRoute::getEstimatedDuration() const
{
    if (m_waypoints.isEmpty()) {
        return QDateTime::currentDateTime();
    }
    
    QDateTime start = m_waypoints.first().estimatedTime;
    QDateTime end = m_waypoints.last().estimatedTime;
    
    return end.isValid() ? end : start.addSecs(getTotalDistance() * 0.1); // Rough estimate
}

void FlightRoute::saveToDatabase()
{
    try {
        ConfigManager& config = ConfigManager::instance();
        
        QString connString = QString("host=%1 port=%2 dbname=%3 user=%4 password=%5 connect_timeout=%6")
            .arg(config.getDatabaseHost())
            .arg(config.getDatabasePort())
            .arg(config.getDatabaseName())
            .arg(config.getDatabaseUsername())
            .arg(config.getDatabasePassword())
            .arg(config.getDatabaseConnectionTimeout());
        
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        // Create flight_routes table if not exists
        QString createTableQuery = R"(
            CREATE TABLE IF NOT EXISTS flight_routes (
                id SERIAL PRIMARY KEY,
                route_id VARCHAR(255) UNIQUE NOT NULL,
                route_type INTEGER NOT NULL,
                description TEXT,
                color VARCHAR(20),
                width INTEGER DEFAULT 2,
                visible BOOLEAN DEFAULT true,
                active BOOLEAN DEFAULT false,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        )";
        
        txn.exec(createTableQuery.toStdString());
        
        // Create waypoints table if not exists
        QString createWaypointsQuery = R"(
            CREATE TABLE IF NOT EXISTS route_waypoints (
                id SERIAL PRIMARY KEY,
                route_id VARCHAR(255) REFERENCES flight_routes(route_id) ON DELETE CASCADE,
                waypoint_order INTEGER NOT NULL,
                name VARCHAR(255),
                longitude DOUBLE PRECISION NOT NULL,
                latitude DOUBLE PRECISION NOT NULL,
                altitude DOUBLE PRECISION DEFAULT 0,
                estimated_time TIMESTAMP,
                description TEXT,
                UNIQUE(route_id, waypoint_order)
            )
        )";
        
        txn.exec(createWaypointsQuery.toStdString());
        
        // Insert or update route
        QString upsertRouteQuery = R"(
            INSERT INTO flight_routes (route_id, route_type, description, color, width, visible, active)
            VALUES ($1, $2, $3, $4, $5, $6, $7)
            ON CONFLICT (route_id) 
            DO UPDATE SET 
                route_type = EXCLUDED.route_type,
                description = EXCLUDED.description,
                color = EXCLUDED.color,
                width = EXCLUDED.width,
                visible = EXCLUDED.visible,
                active = EXCLUDED.active,
                updated_at = CURRENT_TIMESTAMP
        )";
        
        txn.exec_params(upsertRouteQuery.toStdString(),
            m_routeId.toStdString(),
            static_cast<int>(m_routeType),
            m_description.toStdString(),
            m_color.name().toStdString(),
            m_width,
            m_visible,
            m_active
        );
        
        // Delete existing waypoints for this route
        QString deleteWaypointsQuery = "DELETE FROM route_waypoints WHERE route_id = $1";
        txn.exec_params(deleteWaypointsQuery.toStdString(), m_routeId.toStdString());
        
        // Insert waypoints
        for (int i = 0; i < m_waypoints.size(); ++i) {
            const auto& waypoint = m_waypoints[i];
            
            QString insertWaypointQuery = R"(
                INSERT INTO route_waypoints 
                (route_id, waypoint_order, name, longitude, latitude, altitude, estimated_time, description)
                VALUES ($1, $2, $3, $4, $5, $6, $7, $8)
            )";
            
            txn.exec_params(insertWaypointQuery.toStdString(),
                m_routeId.toStdString(),
                i,
                waypoint.name.toStdString(),
                waypoint.position.x(),
                waypoint.position.y(),
                waypoint.altitude,
                waypoint.estimatedTime.toString(Qt::ISODate).toStdString(),
                waypoint.description.toStdString()
            );
        }
        
        txn.commit();
        
        qDebug() << "Successfully saved flight route to database:" << m_routeId;
        
    } catch (const std::exception &e) {
        qDebug() << "Error saving flight route to database:" << e.what();
    }
}

void FlightRoute::loadFromDatabase(const QString& routeId)
{
    try {
        ConfigManager& config = ConfigManager::instance();
        
        QString connString = QString("host=%1 port=%2 dbname=%3 user=%4 password=%5 connect_timeout=%6")
            .arg(config.getDatabaseHost())
            .arg(config.getDatabasePort())
            .arg(config.getDatabaseName())
            .arg(config.getDatabaseUsername())
            .arg(config.getDatabasePassword())
            .arg(config.getDatabaseConnectionTimeout());
        
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        // Load route details
        QString routeQuery = R"(
            SELECT route_type, description, color, width, visible, active
            FROM flight_routes 
            WHERE route_id = $1
        )";
        
        pqxx::result routeResult = txn.exec_params(routeQuery.toStdString(), routeId.toStdString());
        
        if (routeResult.empty()) {
            qDebug() << "Route not found in database:" << routeId;
            return;
        }
        
        auto row = routeResult[0];
        m_routeId = routeId;
        m_routeType = static_cast<RouteType>(row["route_type"].as<int>());
        m_description = QString::fromStdString(row["description"].as<std::string>());
        m_color = QColor(QString::fromStdString(row["color"].as<std::string>()));
        m_width = row["width"].as<int>();
        m_visible = row["visible"].as<bool>();
        m_active = row["active"].as<bool>();
        
        // Load waypoints
        QString waypointsQuery = R"(
            SELECT name, longitude, latitude, altitude, estimated_time, description
            FROM route_waypoints 
            WHERE route_id = $1 
            ORDER BY waypoint_order
        )";
        
        pqxx::result waypointsResult = txn.exec_params(waypointsQuery.toStdString(), routeId.toStdString());
        
        m_waypoints.clear();
        for (const auto& waypointRow : waypointsResult) {
            Waypoint waypoint;
            waypoint.name = QString::fromStdString(waypointRow["name"].as<std::string>());
            waypoint.position = QPointF(
                waypointRow["longitude"].as<double>(),
                waypointRow["latitude"].as<double>()
            );
            waypoint.altitude = waypointRow["altitude"].as<double>();
            waypoint.estimatedTime = QDateTime::fromString(
                QString::fromStdString(waypointRow["estimated_time"].as<std::string>()), 
                Qt::ISODate
            );
            waypoint.description = QString::fromStdString(waypointRow["description"].as<std::string>());
            
            m_waypoints.append(waypoint);
        }
        
        qDebug() << "Successfully loaded flight route from database:" << routeId;
        qDebug() << "Route has" << m_waypoints.size() << "waypoints";
        
        emit routeChanged();
        
    } catch (const std::exception &e) {
        qDebug() << "Error loading flight route from database:" << e.what();
    }
}

void FlightRoute::deleteFromDatabase()
{
    try {
        ConfigManager& config = ConfigManager::instance();
        
        QString connString = QString("host=%1 port=%2 dbname=%3 user=%4 password=%5 connect_timeout=%6")
            .arg(config.getDatabaseHost())
            .arg(config.getDatabasePort())
            .arg(config.getDatabaseName())
            .arg(config.getDatabaseUsername())
            .arg(config.getDatabasePassword())
            .arg(config.getDatabaseConnectionTimeout());
        
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        // Delete route (waypoints will be cascade deleted)
        QString deleteQuery = "DELETE FROM flight_routes WHERE route_id = $1";
        txn.exec_params(deleteQuery.toStdString(), m_routeId.toStdString());
        
        txn.commit();
        
        qDebug() << "Successfully deleted flight route from database:" << m_routeId;
        
    } catch (const std::exception &e) {
        qDebug() << "Error deleting flight route from database:" << e.what();
    }
}

double FlightRoute::calculateDistance(const QPointF& point1, const QPointF& point2)
{
    // Haversine formula for great-circle distance
    const double R = 6371000; // Earth radius in meters
    
    double lat1 = qDegreesToRadians(point1.y());
    double lat2 = qDegreesToRadians(point2.y());
    double deltaLat = qDegreesToRadians(point2.y() - point1.y());
    double deltaLon = qDegreesToRadians(point2.x() - point1.x());
    
    double a = qSin(deltaLat/2) * qSin(deltaLat/2) +
               qCos(lat1) * qCos(lat2) *
               qSin(deltaLon/2) * qSin(deltaLon/2);
    double c = 2 * qAtan2(qSqrt(a), qSqrt(1-a));
    
    return R * c; // Distance in meters
}

QPointF FlightRoute::interpolatePoint(const QPointF& start, const QPointF& end, double ratio)
{
    double lat = start.y() + (end.y() - start.y()) * ratio;
    double lon = start.x() + (end.x() - start.x()) * ratio;
    return QPointF(lon, lat);
}

void FlightRoute::updateRouteMetrics()
{
    // Update estimated times based on distance and speed
    if (m_waypoints.size() > 1) {
        double totalDistance = 0;
        QDateTime currentTime = QDateTime::currentDateTime();
        
        for (int i = 0; i < m_waypoints.size(); ++i) {
            if (i == 0) {
                m_waypoints[i].estimatedTime = currentTime;
            } else {
                double segmentDistance = calculateDistance(
                    m_waypoints[i-1].position, 
                    m_waypoints[i].position
                );
                totalDistance += segmentDistance;
                
                // Assume average speed of 250 m/s (900 km/h)
                int flightTimeSeconds = static_cast<int>(segmentDistance / 250.0);
                m_waypoints[i].estimatedTime = m_waypoints[i-1].estimatedTime.addSecs(flightTimeSeconds);
            }
        }
    }
}

void FlightRoute::createDefaultRoute()
{
    // Create a sample route around Hanoi
    addWaypoint(QPointF(105.8, 21.0), "VTUD"); // Departure: Hanoi
    addWaypoint(QPointF(105.9, 21.1), "WP001"); // Waypoint 1
    addWaypoint(QPointF(106.0, 21.2), "WP002"); // Waypoint 2
    addWaypoint(QPointF(105.7, 21.3), "VTUU"); // Arrival: Another airport
    
    m_description = "Default route around Hanoi area";
} 