#include "aircraft.h"
#include "../core/viewtransform.h"
#include "polygonobject.h"
#include "../core/configmanager.h"
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QDebug>
#include <QtMath>

Aircraft::Aircraft(const QPointF& startPosition, QObject* parent)
    : GeometryObject(parent)
    , m_position(startPosition)
    , m_updateTimer(new QTimer(this))
{
    // Get configuration from ConfigManager
    auto& config = ConfigManager::instance();
    m_updateInterval = config.getAircraftUpdateInterval();
    
    connect(m_updateTimer, &QTimer::timeout, this, &Aircraft::updatePosition);
    m_updateTimer->setInterval(m_updateInterval);
    
    // Set default velocity for movement using config speed
    double speed = config.getDefaultAircraftSpeed();
    m_velocity = QPointF(-speed, speed); // Moving northwest slowly
    updateHeadingFromVelocity();
}

Aircraft::Aircraft(QObject* parent)
    : Aircraft(QPointF(106.0, 20.5), parent) // Default position in Gulf of Tonkin
{
}

void Aircraft::render(QPainter& painter, const ViewTransform& transform)
{
    QPointF screenPos = transform.geoToScreen(m_position);
    
    // Get aircraft size from configuration
    auto& config = ConfigManager::instance();
    int aircraftSize = config.getAircraftIconSize();
    
    // Create aircraft icon
    QColor color = getStateColor();
    QPixmap icon = createAircraftIcon(color, isSelected());
    
    // Draw aircraft icon centered on position
    QRectF iconRect = QRectF(
        screenPos.x() - aircraftSize/2,
        screenPos.y() - aircraftSize/2,
        aircraftSize,
        aircraftSize
    );
    
    painter.save();
    
    // Rotate based on heading
    painter.translate(screenPos);
    painter.rotate(m_heading);
    painter.translate(-aircraftSize/2, -aircraftSize/2);
    
    painter.drawPixmap(QRect(0, 0, aircraftSize, aircraftSize), icon);
    
    // Draw selection highlight if selected
    if (isSelected()) {
        painter.setPen(QPen(Qt::yellow, 3));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(-5, -5, aircraftSize + 10, aircraftSize + 10);
    }
    
    painter.restore();
}

bool Aircraft::containsPoint(const QPointF& geoPoint)
{
    // Simple distance check for selection using config
    auto& config = ConfigManager::instance();
    int selectionRadius = config.getAircraftSelectionRadius();
    
    double dx = geoPoint.x() - m_position.x();
    double dy = geoPoint.y() - m_position.y();
    double distance = qSqrt(dx * dx + dy * dy);
    
    // Convert to approximate pixel distance (rough approximation)
    // At zoom level 12, 1 degree ≈ 111000 meters ≈ 3000 pixels
    double pixelDistance = distance * 3000;
    
    return pixelDistance <= selectionRadius;
}

QRectF Aircraft::boundingBox() const
{
    // Small bounding box around aircraft position
    double margin = 0.001; // About 100 meters
    return QRectF(
        m_position.x() - margin,
        m_position.y() - margin,
        2 * margin,
        2 * margin
    );
}

QString Aircraft::getInfo() const
{
    return QString("Aircraft at (%1, %2), Heading: %3°, State: %4")
        .arg(m_position.x(), 0, 'f', 6)
        .arg(m_position.y(), 0, 'f', 6)
        .arg(m_heading, 0, 'f', 1)
        .arg(m_state == Normal ? "Normal" : 
             m_state == InRegion ? "In Region" : "Selected");
}

void Aircraft::setPosition(const QPointF& position)
{
    if (m_position != position) {
        m_position = position;
        emit positionChanged(m_position);
        emit objectChanged();
    }
}

void Aircraft::setVelocity(const QPointF& velocity)
{
    m_velocity = velocity;
    updateHeadingFromVelocity();
}

void Aircraft::setHeading(double heading)
{
    if (qAbs(m_heading - heading) > 0.1) {
        m_heading = heading;
        emit headingChanged(m_heading);
        emit objectChanged();
    }
}

void Aircraft::setState(State state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(m_state);
        emit objectChanged();
    }
}

void Aircraft::startMovement()
{
    if (!m_isMoving) {
        m_isMoving = true;
        m_updateTimer->start();
        qDebug() << "Aircraft started moving from" << m_position;
    }
}

void Aircraft::stopMovement()
{
    if (m_isMoving) {
        m_isMoving = false;
        m_updateTimer->stop();
        qDebug() << "Aircraft stopped moving at" << m_position;
    }
}

void Aircraft::setUpdateInterval(int milliseconds)
{
    m_updateInterval = milliseconds;
    m_updateTimer->setInterval(m_updateInterval);
}

void Aircraft::setSelected(bool selected)
{
    GeometryObject::setSelected(selected);
    if (selected && m_state != Selected) {
        setState(Selected);
    } else if (!selected && m_state == Selected) {
        setState(Normal); // Reset to normal when deselected
    }
}

void Aircraft::updatePosition()
{
    if (!m_isMoving) return;
    
    // Update position based on velocity
    QPointF newPosition = m_position + m_velocity;
    
    // Simple boundary check - bounce off edges
    if (newPosition.x() < 105.0 || newPosition.x() > 107.0) {
        m_velocity.setX(-m_velocity.x());
        updateHeadingFromVelocity();
    }
    if (newPosition.y() < 20.0 || newPosition.y() > 22.0) {
        m_velocity.setY(-m_velocity.y());
        updateHeadingFromVelocity();
    }
    
    setPosition(m_position + m_velocity);
}

void Aircraft::updateHeadingFromVelocity()
{
    if (m_velocity.x() != 0 || m_velocity.y() != 0) {
        // Calculate heading from velocity vector
        // Note: atan2 gives angle from east, we want angle from north
        double angleRad = qAtan2(m_velocity.x(), m_velocity.y());
        double angleDeg = qRadiansToDegrees(angleRad);
        setHeading(angleDeg);
    }
}

QPixmap Aircraft::createAircraftIcon(const QColor& color, bool highlighted)
{
    auto& config = ConfigManager::instance();
    int aircraftSize = config.getAircraftIconSize();
    
    QPixmap pixmap(aircraftSize, aircraftSize);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw aircraft shape (simple triangle pointing up)
    QPolygonF aircraft;
    aircraft << QPointF(aircraftSize/2, 2)        // Nose
             << QPointF(aircraftSize*0.8, aircraftSize*0.8)  // Right wing
             << QPointF(aircraftSize/2, aircraftSize*0.6)    // Body center
             << QPointF(aircraftSize*0.2, aircraftSize*0.8)  // Left wing
             << QPointF(aircraftSize/2, 2);       // Close

    // Fill aircraft
    painter.setBrush(QBrush(color));
    painter.setPen(QPen(highlighted ? Qt::yellow : Qt::black, 2));
    painter.drawPolygon(aircraft);
    
    // Add some details
    painter.setPen(QPen(Qt::black, 1));
    painter.drawLine(QPointF(AIRCRAFT_SIZE/2, AIRCRAFT_SIZE*0.2), 
                    QPointF(AIRCRAFT_SIZE/2, AIRCRAFT_SIZE*0.6));

    return pixmap;
}

QColor Aircraft::getStateColor() const
{
    auto& config = ConfigManager::instance();
    
    switch (m_state) {
    case Normal:
        return config.getAircraftColor("normal");
    case InRegion:
        return config.getAircraftColor("in_region");
    case Selected:
        return config.getAircraftColor("selected");
    default:
        return config.getAircraftColor("normal");
    }
}
