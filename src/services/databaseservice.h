#ifndef DATABASESERVICE_H
#define DATABASESERVICE_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QPointF>
#include <QPolygonF>
#include <QDateTime>

class Aircraft;
class FlightRoute;

/**
 * @brief Database service layer for centralized database operations
 * 
 * This service provides a clean interface for all database operations,
 * including polygon regions, aircraft, and flight routes.
 */
class DatabaseService : public QObject
{
    Q_OBJECT

public:
    static DatabaseService& instance();

    // Database connection management
    bool connectToDatabase();
    void disconnectFromDatabase();
    bool isConnected() const { return m_connected; }

    // Region/Polygon operations
    struct PolygonRegion {
        QString id;
        QString name;
        QPolygonF polygon;
        QString description;
        QDateTime createdAt;
        QDateTime updatedAt;
    };

    QVector<PolygonRegion> loadAllRegions();
    PolygonRegion loadRegion(const QString& regionId);
    bool saveRegion(const PolygonRegion& region);
    bool updateRegion(const PolygonRegion& region);
    bool deleteRegion(const QString& regionId);
    
    // Create default Hanoi polygon region
    void createDefaultHanoiRegion();

    // Aircraft operations
    QVector<Aircraft*> loadAllAircraft(QObject* parent = nullptr);
    bool saveAircraft(Aircraft* aircraft);
    bool updateAircraft(Aircraft* aircraft);
    bool deleteAircraft(const QString& aircraftId);
    bool aircraftExists(const QString& aircraftId);

    // Flight route operations
    QVector<FlightRoute*> loadAllFlightRoutes(QObject* parent = nullptr);
    bool saveFlightRoute(FlightRoute* route);
    bool updateFlightRoute(FlightRoute* route);
    bool deleteFlightRoute(const QString& routeId);
    bool flightRouteExists(const QString& routeId);

    // Database maintenance
    void createTables();
    void cleanupOldData(int daysOld = 30);
    QString getConnectionInfo() const;

signals:
    void databaseConnected();
    void databaseDisconnected();
    void databaseError(const QString& error);
    void operationCompleted(bool success, const QString& operation, const QString& message);

private:
    explicit DatabaseService(QObject* parent = nullptr);
    ~DatabaseService();

    QString buildConnectionString() const;
    void logError(const QString& operation, const QString& error);
    void logSuccess(const QString& operation, const QString& message);

    // WKT conversion helpers
    QPolygonF parseWKTPolygon(const QString& wkt);
    QString polygonToWKT(const QPolygonF& polygon);

    bool m_connected = false;
    static DatabaseService* s_instance;
};

#endif // DATABASESERVICE_H 