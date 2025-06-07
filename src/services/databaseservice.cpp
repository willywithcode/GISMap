#include "databaseservice.h"
#include "../models/aircraft.h"
#include "../models/flightroute.h"
#include "../core/configmanager.h"
#include <QDebug>
#include <QUuid>
#include <pqxx/pqxx>

DatabaseService* DatabaseService::s_instance = nullptr;

DatabaseService& DatabaseService::instance()
{
    if (!s_instance) {
        s_instance = new DatabaseService();
    }
    return *s_instance;
}

DatabaseService::DatabaseService(QObject* parent)
    : QObject(parent)
{
    connectToDatabase();
}

DatabaseService::~DatabaseService()
{
    disconnectFromDatabase();
}

bool DatabaseService::connectToDatabase()
{
    try {
        // Test connection using ConfigManager
        ConfigManager& config = ConfigManager::instance();
        
        QString connString = buildConnectionString();
        pqxx::connection testConn(connString.toStdString());
        
        if (testConn.is_open()) {
            m_connected = true;
            createTables(); // Ensure all tables exist
            
            qDebug() << "Successfully connected to database";
            emit databaseConnected();
            return true;
        }
    } catch (const std::exception &e) {
        m_connected = false;
        QString error = QString("Database connection failed: %1").arg(e.what());
        logError("Database Connection", error);
        emit databaseError(error);
    }
    
    return false;
}

void DatabaseService::disconnectFromDatabase()
{
    if (m_connected) {
        m_connected = false;
        qDebug() << "Disconnected from database";
        emit databaseDisconnected();
    }
}

QVector<DatabaseService::PolygonRegion> DatabaseService::loadAllRegions()
{
    QVector<PolygonRegion> regions;
    
    try {
        QString connString = buildConnectionString();
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        QString query = R"(
            SELECT region_id, name, description, created_at, updated_at,
                   ST_AsText(geom) as wkt_geometry
            FROM polygon_regions 
            ORDER BY created_at
        )";
        
        pqxx::result result = txn.exec(query.toStdString());
        
        for (const auto& row : result) {
            PolygonRegion region;
            region.id = QString::fromStdString(row["region_id"].as<std::string>());
            region.name = QString::fromStdString(row["name"].as<std::string>());
            region.description = QString::fromStdString(row["description"].as<std::string>());
            region.createdAt = QDateTime::fromString(
                QString::fromStdString(row["created_at"].as<std::string>()), Qt::ISODate);
            region.updatedAt = QDateTime::fromString(
                QString::fromStdString(row["updated_at"].as<std::string>()), Qt::ISODate);
            
            // Parse WKT geometry to QPolygonF
            QString wkt = QString::fromStdString(row["wkt_geometry"].as<std::string>());
            region.polygon = parseWKTPolygon(wkt);
            
            regions.append(region);
        }
        
        logSuccess("Load Regions", QString("Loaded %1 polygon regions").arg(regions.size()));
        
    } catch (const std::exception &e) {
        logError("Load Regions", e.what());
    }
    
    return regions;
}

DatabaseService::PolygonRegion DatabaseService::loadRegion(const QString& regionId)
{
    PolygonRegion region;
    
    try {
        QString connString = buildConnectionString();
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        QString query = R"(
            SELECT name, description, created_at, updated_at,
                   ST_AsText(geom) as wkt_geometry
            FROM polygon_regions 
            WHERE region_id = $1
        )";
        
        pqxx::result result = txn.exec_params(query.toStdString(), regionId.toStdString());
        
        if (!result.empty()) {
            auto row = result[0];
            region.id = regionId;
            region.name = QString::fromStdString(row["name"].as<std::string>());
            region.description = QString::fromStdString(row["description"].as<std::string>());
            region.createdAt = QDateTime::fromString(
                QString::fromStdString(row["created_at"].as<std::string>()), Qt::ISODate);
            region.updatedAt = QDateTime::fromString(
                QString::fromStdString(row["updated_at"].as<std::string>()), Qt::ISODate);
            
            QString wkt = QString::fromStdString(row["wkt_geometry"].as<std::string>());
            region.polygon = parseWKTPolygon(wkt);
            
            logSuccess("Load Region", QString("Loaded region: %1").arg(regionId));
        }
        
    } catch (const std::exception &e) {
        logError("Load Region", e.what());
    }
    
    return region;
}

bool DatabaseService::saveRegion(const PolygonRegion& region)
{
    try {
        QString connString = buildConnectionString();
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        // Convert QPolygonF to WKT
        QString wkt = polygonToWKT(region.polygon);
        
        QString insertQuery = R"(
            INSERT INTO polygon_regions (region_id, name, description, geom, created_at, updated_at)
            VALUES ($1, $2, $3, ST_GeomFromText($4, 4326), $5, $6)
        )";
        
        txn.exec_params(insertQuery.toStdString(),
            region.id.toStdString(),
            region.name.toStdString(),
            region.description.toStdString(),
            wkt.toStdString(),
            region.createdAt.toString(Qt::ISODate).toStdString(),
            region.updatedAt.toString(Qt::ISODate).toStdString()
        );
        
        txn.commit();
        
        logSuccess("Save Region", QString("Saved region: %1").arg(region.name));
        emit operationCompleted(true, "Save Region", "Region saved successfully");
        return true;
        
    } catch (const std::exception &e) {
        logError("Save Region", e.what());
        emit operationCompleted(false, "Save Region", e.what());
        return false;
    }
}

bool DatabaseService::updateRegion(const PolygonRegion& region)
{
    try {
        QString connString = buildConnectionString();
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        QString wkt = polygonToWKT(region.polygon);
        
        QString updateQuery = R"(
            UPDATE polygon_regions SET 
                name = $2, description = $3, geom = ST_GeomFromText($4, 4326), updated_at = $5
            WHERE region_id = $1
        )";
        
        txn.exec_params(updateQuery.toStdString(),
            region.id.toStdString(),
            region.name.toStdString(),
            region.description.toStdString(),
            wkt.toStdString(),
            QDateTime::currentDateTime().toString(Qt::ISODate).toStdString()
        );
        
        txn.commit();
        
        logSuccess("Update Region", QString("Updated region: %1").arg(region.name));
        emit operationCompleted(true, "Update Region", "Region updated successfully");
        return true;
        
    } catch (const std::exception &e) {
        logError("Update Region", e.what());
        emit operationCompleted(false, "Update Region", e.what());
        return false;
    }
}

bool DatabaseService::deleteRegion(const QString& regionId)
{
    try {
        QString connString = buildConnectionString();
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        QString deleteQuery = "DELETE FROM polygon_regions WHERE region_id = $1";
        txn.exec_params(deleteQuery.toStdString(), regionId.toStdString());
        
        txn.commit();
        
        logSuccess("Delete Region", QString("Deleted region: %1").arg(regionId));
        emit operationCompleted(true, "Delete Region", "Region deleted successfully");
        return true;
        
    } catch (const std::exception &e) {
        logError("Delete Region", e.what());
        emit operationCompleted(false, "Delete Region", e.what());
        return false;
    }
}

void DatabaseService::createDefaultHanoiRegion()
{
    try {
        // Check if Hanoi region already exists
        QString connString = buildConnectionString();
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        QString checkQuery = "SELECT COUNT(*) FROM polygon_regions WHERE name = 'Hanoi Area'";
        pqxx::result checkResult = txn.exec(checkQuery.toStdString());
        
        if (checkResult[0][0].as<int>() == 0) {
            // Create default Hanoi polygon region
            PolygonRegion hanoiRegion;
            hanoiRegion.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            hanoiRegion.name = "Hanoi Area";
            hanoiRegion.description = "Default aircraft interaction region around Hanoi";
            hanoiRegion.createdAt = QDateTime::currentDateTime();
            hanoiRegion.updatedAt = QDateTime::currentDateTime();
            
            // Create polygon around Hanoi
            hanoiRegion.polygon << QPointF(105.7, 20.8)
                               << QPointF(105.7, 21.3)
                               << QPointF(106.1, 21.3)
                               << QPointF(106.1, 20.8)
                               << QPointF(105.7, 20.8); // Close polygon
            
            saveRegion(hanoiRegion);
            
            qDebug() << "Created default Hanoi region";
        } else {
            qDebug() << "Hanoi region already exists";
        }
        
    } catch (const std::exception &e) {
        logError("Create Default Hanoi Region", e.what());
    }
}

QVector<Aircraft*> DatabaseService::loadAllAircraft(QObject* parent)
{
    return Aircraft::loadAllFromDatabase(parent);
}

bool DatabaseService::saveAircraft(Aircraft* aircraft)
{
    if (!aircraft) return false;
    
    aircraft->saveToDatabase();
    return true; // Aircraft class handles its own error reporting
}

bool DatabaseService::updateAircraft(Aircraft* aircraft)
{
    if (!aircraft) return false;
    
    aircraft->updateInDatabase();
    return true;
}

bool DatabaseService::deleteAircraft(const QString& aircraftId)
{
    try {
        QString connString = buildConnectionString();
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        QString deleteQuery = "DELETE FROM aircraft WHERE aircraft_id = $1";
        txn.exec_params(deleteQuery.toStdString(), aircraftId.toStdString());
        
        txn.commit();
        
        logSuccess("Delete Aircraft", QString("Deleted aircraft: %1").arg(aircraftId));
        emit operationCompleted(true, "Delete Aircraft", "Aircraft deleted successfully");
        return true;
        
    } catch (const std::exception &e) {
        logError("Delete Aircraft", e.what());
        emit operationCompleted(false, "Delete Aircraft", e.what());
        return false;
    }
}

bool DatabaseService::aircraftExists(const QString& aircraftId)
{
    return Aircraft::existsInDatabase(aircraftId);
}

QVector<FlightRoute*> DatabaseService::loadAllFlightRoutes(QObject* parent)
{
    QVector<FlightRoute*> routes;
    
    try {
        QString connString = buildConnectionString();
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        QString query = "SELECT route_id FROM flight_routes ORDER BY created_at";
        pqxx::result result = txn.exec(query.toStdString());
        
        for (const auto& row : result) {
            QString routeId = QString::fromStdString(row["route_id"].as<std::string>());
            FlightRoute* route = new FlightRoute(parent);
            route->loadFromDatabase(routeId);
            routes.append(route);
        }
        
        logSuccess("Load Flight Routes", QString("Loaded %1 flight routes").arg(routes.size()));
        
    } catch (const std::exception &e) {
        logError("Load Flight Routes", e.what());
    }
    
    return routes;
}

bool DatabaseService::saveFlightRoute(FlightRoute* route)
{
    if (!route) return false;
    
    route->saveToDatabase();
    return true;
}

bool DatabaseService::updateFlightRoute(FlightRoute* route)
{
    if (!route) return false;
    
    route->saveToDatabase(); // FlightRoute uses upsert
    return true;
}

bool DatabaseService::deleteFlightRoute(const QString& routeId)
{
    try {
        QString connString = buildConnectionString();
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        QString deleteQuery = "DELETE FROM flight_routes WHERE route_id = $1";
        txn.exec_params(deleteQuery.toStdString(), routeId.toStdString());
        
        txn.commit();
        
        logSuccess("Delete Flight Route", QString("Deleted flight route: %1").arg(routeId));
        emit operationCompleted(true, "Delete Flight Route", "Flight route deleted successfully");
        return true;
        
    } catch (const std::exception &e) {
        logError("Delete Flight Route", e.what());
        emit operationCompleted(false, "Delete Flight Route", e.what());
        return false;
    }
}

bool DatabaseService::flightRouteExists(const QString& routeId)
{
    try {
        QString connString = buildConnectionString();
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        QString query = "SELECT COUNT(*) FROM flight_routes WHERE route_id = $1";
        pqxx::result result = txn.exec_params(query.toStdString(), routeId.toStdString());
        
        return result[0][0].as<int>() > 0;
        
    } catch (const std::exception &e) {
        logError("Check Flight Route Existence", e.what());
        return false;
    }
}

void DatabaseService::createTables()
{
    try {
        QString connString = buildConnectionString();
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        // Create aircraft table
        QString createAircraftTable = R"(
            CREATE TABLE IF NOT EXISTS aircraft (
                id SERIAL PRIMARY KEY,
                aircraft_id VARCHAR(255) UNIQUE NOT NULL,
                call_sign VARCHAR(50),
                aircraft_type VARCHAR(50),
                longitude DOUBLE PRECISION NOT NULL,
                latitude DOUBLE PRECISION NOT NULL,
                altitude DOUBLE PRECISION DEFAULT 0,
                speed DOUBLE PRECISION DEFAULT 0,
                heading DOUBLE PRECISION DEFAULT 0,
                velocity_x DOUBLE PRECISION DEFAULT 0,
                velocity_y DOUBLE PRECISION DEFAULT 0,
                state INTEGER DEFAULT 0,
                flight_route_id VARCHAR(255),
                is_moving BOOLEAN DEFAULT false,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        )";
        
        txn.exec(createAircraftTable.toStdString());
        qDebug() << "Aircraft table created/verified";
        
        // Create flight_routes table
        QString createFlightRoutesTable = R"(
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
        
        txn.exec(createFlightRoutesTable.toStdString());
        qDebug() << "Flight routes table created/verified";
        
        // Create route_waypoints table
        QString createWaypointsTable = R"(
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
        
        txn.exec(createWaypointsTable.toStdString());
        qDebug() << "Route waypoints table created/verified";
        
        // Create polygon_regions table
        QString createRegionsTable = R"(
            CREATE TABLE IF NOT EXISTS polygon_regions (
                id SERIAL PRIMARY KEY,
                region_id VARCHAR(255) UNIQUE NOT NULL,
                name VARCHAR(255) NOT NULL,
                description TEXT,
                geom GEOMETRY(POLYGON, 4326) NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        )";
        
        txn.exec(createRegionsTable.toStdString());
        qDebug() << "Polygon regions table created/verified";
        
        // Create spatial index
        QString createSpatialIndex = R"(
            CREATE INDEX IF NOT EXISTS idx_polygon_regions_geom 
            ON polygon_regions USING GIST (geom)
        )";
        
        txn.exec(createSpatialIndex.toStdString());
        qDebug() << "Spatial index created/verified";
        
        txn.commit();
        
        qDebug() << "All database tables created/verified successfully";
        
        // Create default Hanoi region after tables are created
        createDefaultHanoiRegion();
        
    } catch (const std::exception &e) {
        logError("Create Tables", e.what());
    }
}

void DatabaseService::cleanupOldData(int daysOld)
{
    try {
        QString connString = buildConnectionString();
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        // Cleanup old aircraft that haven't been updated
        QString cleanupAircraft = QString(
            "DELETE FROM aircraft WHERE updated_at < NOW() - INTERVAL '%1 days' AND is_moving = false"
        ).arg(daysOld);
        
        pqxx::result aircraftResult = txn.exec(cleanupAircraft.toStdString());
        
        // Cleanup old flight routes
        QString cleanupRoutes = QString(
            "DELETE FROM flight_routes WHERE updated_at < NOW() - INTERVAL '%1 days' AND active = false"
        ).arg(daysOld);
        
        pqxx::result routesResult = txn.exec(cleanupRoutes.toStdString());
        
        txn.commit();
        
        logSuccess("Cleanup Old Data", 
            QString("Cleaned up old aircraft and flight routes older than %1 days").arg(daysOld));
        
    } catch (const std::exception &e) {
        logError("Cleanup Old Data", e.what());
    }
}

QString DatabaseService::getConnectionInfo() const
{
    ConfigManager& config = ConfigManager::instance();
    return QString("Host: %1:%2, Database: %3, User: %4")
        .arg(config.getDatabaseHost())
        .arg(config.getDatabasePort())
        .arg(config.getDatabaseName())
        .arg(config.getDatabaseUsername());
}

QString DatabaseService::buildConnectionString() const
{
    ConfigManager& config = ConfigManager::instance();
    
    return QString("host=%1 port=%2 dbname=%3 user=%4 password=%5 connect_timeout=%6")
        .arg(config.getDatabaseHost())
        .arg(config.getDatabasePort())
        .arg(config.getDatabaseName())
        .arg(config.getDatabaseUsername())
        .arg(config.getDatabasePassword())
        .arg(config.getDatabaseConnectionTimeout());
}

void DatabaseService::logError(const QString& operation, const QString& error)
{
    qDebug() << "Database Error in" << operation << ":" << error;
    emit databaseError(QString("%1: %2").arg(operation, error));
}

void DatabaseService::logSuccess(const QString& operation, const QString& message)
{
    qDebug() << "Database Success in" << operation << ":" << message;
}

// Helper functions for WKT conversion
QPolygonF DatabaseService::parseWKTPolygon(const QString& wkt)
{
    QPolygonF polygon;
    
    // Simple WKT parser for POLYGON((x1 y1, x2 y2, ...))
    QString coords = wkt;
    coords.remove("POLYGON((");
    coords.remove("))");
    
    QStringList points = coords.split(",");
    for (const QString& point : points) {
        QStringList xy = point.trimmed().split(" ");
        if (xy.size() >= 2) {
            polygon << QPointF(xy[0].toDouble(), xy[1].toDouble());
        }
    }
    
    return polygon;
}

QString DatabaseService::polygonToWKT(const QPolygonF& polygon)
{
    QString wkt = "POLYGON((";
    
    for (int i = 0; i < polygon.size(); ++i) {
        const QPointF& point = polygon[i];
        if (i > 0) wkt += ", ";
        wkt += QString("%1 %2").arg(point.x()).arg(point.y());
    }
    
    wkt += "))";
    return wkt;
} 