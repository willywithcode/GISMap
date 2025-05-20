#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include <QObject>
#include <QPointF>
#include <QColor>
#include <QTimer>
#include <QString>
#include "core/types.h"

namespace GISMap {
namespace Model {

/**
 * Class representing an aircraft on the map
 */
class Aircraft : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor
     * @param id Unique identifier for the aircraft
     * @param parent Parent object for memory management
     */
    explicit Aircraft(const QString &id, QObject *parent = nullptr);
    
    /**
     * Destructor
     */
    ~Aircraft();
    
    /**
     * Get the aircraft's current position
     * @return Position as QPointF (latitude, longitude)
     */
    QPointF position() const { return m_position; }
    
    /**
     * Get the aircraft's heading in degrees (0-360)
     * @return Heading in degrees
     */
    double heading() const { return m_heading; }
    
    /**
     * Get the aircraft's current color
     * @return Color as QColor
     */
    QColor color() const { return m_color; }
    
    /**
     * Check if the aircraft is currently selected
     * @return True if selected
     */
    bool isSelected() const { return m_isSelected; }
    
    /**
     * Get the aircraft's unique identifier
     * @return ID string
     */
    QString id() const { return m_id; }
    
    /**
     * Set the aircraft's position
     * @param position New position as QPointF (latitude, longitude)
     */
    void setPosition(const QPointF &position);
    
    /**
     * Set the aircraft's heading
     * @param heading New heading in degrees
     */
    void setHeading(double heading);
    
    /**
     * Set the aircraft's color
     * @param color New color
     */
    void setColor(const QColor &color);
    
    /**
     * Set the aircraft's selection state
     * @param selected New selection state
     */
    void setSelected(bool selected);
    
    /**
     * Update the aircraft's position based on current heading and speed
     */
    void updatePosition();
    
    /**
     * Set the aircraft's speed
     * @param speed Speed in degrees per second
     */
    void setSpeed(double speed);
    
    /**
     * Set the target position for the aircraft to move toward
     * @param target Target position as QPointF (latitude, longitude)
     */
    void setTargetPosition(const QPointF &target);
    
    /**
     * Start automatic movement to target
     * @param intervalMs Update interval in milliseconds
     */
    void startMovement(int intervalMs = 1000);
    
    /**
     * Stop automatic movement
     */
    void stopMovement();
    
    /**
     * Check if a point (in screen coordinates) is inside the aircraft
     * @param screenPoint Point to check in screen coordinates
     * @param screenPosition Aircraft's position in screen coordinates
     * @param size Size of the aircraft in pixels
     * @return True if the point is inside the aircraft
     */
    bool containsScreenPoint(const QPoint &screenPoint, 
                             const QPoint &screenPosition,
                             int size = 20) const;

signals:
    /**
     * Signal emitted when the aircraft's position changes
     * @param position New position
     */
    void positionChanged(const QPointF &position);
    
    /**
     * Signal emitted when the aircraft's color changes
     * @param color New color
     */
    void colorChanged(const QColor &color);
    
    /**
     * Signal emitted when the aircraft's selection state changes
     * @param selected New selection state
     */
    void selectionChanged(bool selected);

private slots:
    /**
     * Handle the movement timer timeout
     */
    void onMovementTimerTimeout();

private:
    QString m_id;
    QPointF m_position;      // Current position (lat, lon)
    QPointF m_targetPosition;// Target position (lat, lon)
    double m_heading;        // Heading in degrees
    QColor m_color;          // Aircraft color
    bool m_isSelected;       // Selection state
    double m_speed;          // Speed in degrees per second
    QTimer *m_movementTimer; // Timer for movement updates
    
    /**
     * Calculate heading to target position
     * @return Heading in degrees
     */
    double calculateHeadingToTarget();
};

} // namespace Model
} // namespace GISMap

#endif // AIRCRAFT_H 