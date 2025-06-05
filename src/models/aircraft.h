#pragma once
#include "../core/geometryobject.h"
#include <QTimer>
#include <QPointF>
#include <QColor>
#include <QPixmap>

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

    // GeometryObject interface
    void render(QPainter& painter, const ViewTransform& transform) override;
    bool containsPoint(const QPointF& geoPoint) override;
    QRectF boundingBox() const override;
    QString getInfo() const override;

    // Aircraft-specific methods
    void setPosition(const QPointF& position);
    QPointF position() const { return m_position; }
    
    void setVelocity(const QPointF& velocity);
    QPointF velocity() const { return m_velocity; }
    
    void setHeading(double heading);
    double heading() const { return m_heading; }
    
    void setState(State state);
    State state() const { return m_state; }
    
    void startMovement();
    void stopMovement();
    bool isMoving() const { return m_isMoving; }
    
    void setUpdateInterval(int milliseconds);
    int updateInterval() const { return m_updateInterval; }

    // Selection handling
    void setSelected(bool selected) override;

signals:
    void positionChanged(const QPointF& newPosition);
    void stateChanged(Aircraft::State newState);
    void headingChanged(double newHeading);

private slots:
    void updatePosition();

private:
    void updateHeadingFromVelocity();
    QPixmap createAircraftIcon(const QColor& color, bool highlighted = false);
    QColor getStateColor() const;
    
    QPointF m_position = QPointF(106.0, 20.5); // Default: Gulf of Tonkin
    QPointF m_velocity = QPointF(-0.001, 0.001); // Default velocity (degrees per second)
    double m_heading = 0.0; // Heading in degrees (0 = North)
    State m_state = Normal;
    
    QTimer* m_updateTimer;
    bool m_isMoving = false;
    int m_updateInterval = 1000; // 1 second
    
    // Visual properties
    static constexpr double AIRCRAFT_SIZE = 20.0; // Size in pixels
    static constexpr double SELECTION_RADIUS = 15.0; // Click detection radius in pixels
};
