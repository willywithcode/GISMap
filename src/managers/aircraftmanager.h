#pragma once
#include <QObject>
#include <QVector>
#include <QTimer>
#include <QPointF>

class Aircraft;
class PolygonObject;

/**
 * @brief Manages multiple aircraft objects
 * Handles creation, lifecycle, and coordination of aircraft
 */
class AircraftManager : public QObject {
    Q_OBJECT
public:
    explicit AircraftManager(QObject* parent = nullptr);
    ~AircraftManager();

    // Aircraft management
    Aircraft* createAircraft(const QPointF& startPosition = QPointF());
    void addExistingAircraft(Aircraft* aircraft);  // Add existing aircraft from database
    void removeAircraft(Aircraft* aircraft);
    void clearAllAircraft();
    
    QVector<Aircraft*> allAircraft() const { return m_aircrafts; }
    int aircraftCount() const { return m_aircrafts.size(); }
    
    // Region management
    void setPolygonRegion(PolygonObject* polygon);
    PolygonObject* polygonRegion() const { return m_polygonRegion; }
    
    // Batch operations
    void startAllMovement();
    void stopAllMovement();
    void setAllUpdateInterval(int milliseconds);

signals:
    void aircraftCreated(Aircraft* aircraft);
    void aircraftRemoved(Aircraft* aircraft);
    void aircraftCountChanged(int count);

private slots:
    void onAircraftDestroyed();

private:
    QVector<Aircraft*> m_aircrafts;
    PolygonObject* m_polygonRegion = nullptr;
    
    // Default properties for new aircraft
    int m_defaultUpdateInterval = 1000;
    
    // Helper methods
    QPointF generateRandomPosition();
    QPointF generateRandomVelocity();
};
