#include "model/aircraft.h"
#include "core/geoutil.h"
#include <QtMath>
#include <QRectF>

namespace GISMap {
namespace Model {

Aircraft::Aircraft(const QString &id, QObject *parent)
    : QObject(parent)
    , m_id(id)
    , m_position(21.0278, 105.8342) // Default position (Hanoi)
    , m_targetPosition(m_position)
    , m_heading(0.0)
    , m_color(Qt::blue)
    , m_isSelected(false)
    , m_speed(0.01) // degrees per update
    , m_movementTimer(new QTimer(this))
{
    // Connect the movement timer to the update slot
    connect(m_movementTimer, &QTimer::timeout,
            this, &Aircraft::onMovementTimerTimeout);
}

Aircraft::~Aircraft()
{
    // Stop the movement timer if it's active
    if (m_movementTimer->isActive()) {
        m_movementTimer->stop();
    }
}

void Aircraft::setPosition(const QPointF &position)
{
    if (m_position != position) {
        m_position = position;
        emit positionChanged(m_position);
    }
}

void Aircraft::setHeading(double heading)
{
    // Normalize the heading to 0-360 range
    while (heading < 0.0) {
        heading += 360.0;
    }
    
    while (heading >= 360.0) {
        heading -= 360.0;
    }
    
    m_heading = heading;
}

void Aircraft::setColor(const QColor &color)
{
    if (m_color != color) {
        m_color = color;
        emit colorChanged(m_color);
    }
}

void Aircraft::setSelected(bool selected)
{
    if (m_isSelected != selected) {
        m_isSelected = selected;
        emit selectionChanged(m_isSelected);
    }
}

void Aircraft::updatePosition()
{
    // Convert headings to radians
    double headingRad = m_heading * M_PI / 180.0;
    
    // Calculate new position
    double lat = m_position.x() + m_speed * qCos(headingRad);
    double lon = m_position.y() + m_speed * qSin(headingRad);
    
    // Set the new position
    setPosition(QPointF(lat, lon));
}

void Aircraft::setSpeed(double speed)
{
    m_speed = speed;
}

void Aircraft::setTargetPosition(const QPointF &target)
{
    m_targetPosition = target;
    
    // Update the heading to point toward the target
    setHeading(calculateHeadingToTarget());
}

void Aircraft::startMovement(int intervalMs)
{
    // Start the movement timer
    m_movementTimer->start(intervalMs);
}

void Aircraft::stopMovement()
{
    // Stop the movement timer
    m_movementTimer->stop();
}

bool Aircraft::containsScreenPoint(const QPoint &screenPoint, 
                                  const QPoint &screenPosition,
                                  int size) const
{
    // Create a rectangle around the aircraft's screen position
    QRect rect(screenPosition.x() - size / 2,
               screenPosition.y() - size / 2,
               size, size);
               
    // Check if the point is inside the rectangle
    return rect.contains(screenPoint);
}

void Aircraft::onMovementTimerTimeout()
{
    // Calculate distance to target
    double distance = Core::GeoUtil::distanceGeo(
        m_position.x(), m_position.y(),
        m_targetPosition.x(), m_targetPosition.y());
        
    // If we're close to the target, set a new random target
    const double minDistance = 0.1; // km
    if (distance < minDistance) {
        // Get current position bounds
        double minLat = 17.0; // Southern Vietnam
        double maxLat = 23.5; // Northern Vietnam
        double minLon = 102.0; // Western Vietnam
        double maxLon = 109.5; // Eastern Vietnam
        
        // Generate a random position within Vietnam's bounds
        double newLat = minLat + (maxLat - minLat) * (double)qrand() / RAND_MAX;
        double newLon = minLon + (maxLon - minLon) * (double)qrand() / RAND_MAX;
        
        // Set the new target
        m_targetPosition = QPointF(newLat, newLon);
        
        // Update heading
        setHeading(calculateHeadingToTarget());
    }
    
    // Update the position
    updatePosition();
}

double Aircraft::calculateHeadingToTarget()
{
    // Get the differences in coordinates
    double dLat = m_targetPosition.x() - m_position.x();
    double dLon = m_targetPosition.y() - m_position.y();
    
    // Calculate the heading in radians
    double heading = qAtan2(dLon, dLat);
    
    // Convert to degrees
    heading = heading * 180.0 / M_PI;
    
    // Normalize to 0-360
    if (heading < 0) {
        heading += 360.0;
    }
    
    return heading;
}

} // namespace Model
} // namespace GISMap 