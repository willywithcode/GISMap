#pragma once
#include "../core/geometryobject.h"
#include <QTimer>
#include <QPointF>
#include <QColor>
#include <QPixmap>
#include <QDateTime>

/**
 * @brief Represents an aircraft object that moves on the map
 */
class Aircraft : public GeometryObject {
    Q_OBJECT
public:
    enum State {
        Normal,      // Outside region, blue color
        InRegion,    // Inside polygon region, red color  
        Selected     // Selected by user, highlighted
    };

    explicit Aircraft(const QPointF& startPosition, QObject* parent = nullptr);
    explicit Aircraft(QObject* parent = nullptr);
    explicit Aircraft(const QString& aircraftId, QObject* parent = nullptr);

    // GeometryObject interface
    void render(QPainter& painter, const ViewTransform& transform) override;
    bool containsPoint(const QPointF& geoPoint) override;
    QRectF boundingBox() const override;
    QString getInfo() const override;

    // Aircraft identification
    QString getAircraftId() const { return m_aircraftId; }
    void setAircraftId(const QString& id) { m_aircraftId = id; }

    QString getCallSign() const { return m_callSign; }
    void setCallSign(const QString& callSign) { m_callSign = callSign; }

    QString getAircraftType() const { return m_aircraftType; }
    void setAircraftType(const QString& type) { m_aircraftType = type; }

    // Aircraft-specific methods
    void setPosition(const QPointF& position);
    QPointF position() const { return m_position; }
    
    void setVelocity(const QPointF& velocity);
    QPointF velocity() const { return m_velocity; }
    
    void setHeading(double heading);
    double heading() const { return m_heading; }

    void setAltitude(double altitude);
    double altitude() const { return m_altitude; }

    void setSpeed(double speed);
    double speed() const { return m_speed; }
    
    void setState(State state);
    State state() const { return m_state; }
    
    void startMovement();
    void stopMovement();
    bool isMoving() const { return m_isMoving; }
    
    void setUpdateInterval(int milliseconds);
    int updateInterval() const { return m_updateInterval; }

    // Flight route
    QString getFlightRouteId() const { return m_flightRouteId; }
    void setFlightRouteId(const QString& routeId) { m_flightRouteId = routeId; }

    // Timestamps
    QDateTime getCreatedAt() const { return m_createdAt; }
    QDateTime getUpdatedAt() const { return m_updatedAt; }

    // Selection handling
    void setSelected(bool selected) override;

    // Flight trail tracking
    void setTrailEnabled(bool enabled) { m_trailEnabled = enabled; }
    bool isTrailEnabled() const { return m_trailEnabled; }
    void setMaxTrailPoints(int maxPoints) { m_maxTrailPoints = maxPoints; }
    int maxTrailPoints() const { return m_maxTrailPoints; }
    QVector<QPointF> getTrail() const { return m_flightTrail; }
    void clearTrail() { m_flightTrail.clear(); }

    // Database operations
    void saveToDatabase();
    void loadFromDatabase(const QString& aircraftId);
    void updateInDatabase();
    void deleteFromDatabase();
    
    static QVector<Aircraft*> loadAllFromDatabase(QObject* parent = nullptr);
    static bool existsInDatabase(const QString& aircraftId);

signals:
    void positionChanged(const QPointF& newPosition);
    void stateChanged(Aircraft::State newState);
    void headingChanged(double newHeading);
    void altitudeChanged(double newAltitude);
    void speedChanged(double newSpeed);
    void databaseOperationCompleted(bool success, const QString& message);

private slots:
    void updatePosition();

private:
    void updateHeadingFromVelocity();
    QPixmap createAircraftIcon(const QColor& color, bool highlighted = false);
    QColor getStateColor() const;
    void generateAircraftId();
    void updateTimestamp();
    void addTrailPoint(const QPointF& position);
    
    // Aircraft identification
    QString m_aircraftId;
    QString m_callSign;
    QString m_aircraftType = "Unknown";
    
    // Position and movement
    QPointF m_position = QPointF(106.0, 20.5); // Default: Gulf of Tonkin
    QPointF m_velocity = QPointF(-0.001, 0.001); // Default velocity (degrees per second)
    double m_heading = 0.0; // Heading in degrees (0 = North)
    double m_altitude = 10000.0; // Altitude in meters
    double m_speed = 250.0; // Speed in m/s
    State m_state = Normal;
    
    // Flight planning
    QString m_flightRouteId;
    
    // Timestamps
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
    
    QTimer* m_updateTimer;
    bool m_isMoving = false;
    int m_updateInterval = 1000; // 1 second
    
    // Visual properties
    static constexpr double AIRCRAFT_SIZE = 20.0; // Size in pixels
    static constexpr double SELECTION_RADIUS = 15.0; // Click detection radius in pixels

    // Flight trail tracking
    bool m_trailEnabled = false;
    int m_maxTrailPoints = 100;
    QVector<QPointF> m_flightTrail;
};
