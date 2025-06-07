#include "aircraftmanager.h"
#include "../models/aircraft.h"
#include "../models/polygonobject.h"
#include "../core/configmanager.h"
#include <QRandomGenerator>
#include <QDebug>

AircraftManager::AircraftManager(QObject* parent)
    : QObject(parent)
    , m_polygonRegion(nullptr)
{
}

AircraftManager::~AircraftManager()
{
    clearAllAircraft();
}

Aircraft* AircraftManager::createAircraft(const QPointF& startPosition)
{
    auto& config = ConfigManager::instance();
    QPointF position = startPosition.isNull() ? generateRandomPosition() : startPosition;
    
    Aircraft* aircraft = new Aircraft(position, this);
    aircraft->setVelocity(generateRandomVelocity());
    aircraft->setUpdateInterval(config.getAircraftUpdateInterval());
    
    // Connect destruction signal
    connect(aircraft, &QObject::destroyed, 
            this, &AircraftManager::onAircraftDestroyed);
    
    m_aircrafts.append(aircraft);
    
    emit aircraftCreated(aircraft);
    emit aircraftCountChanged(m_aircrafts.size());
    
    qDebug() << "Created aircraft at" << position << "Total:" << m_aircrafts.size();
    
    return aircraft;
}

void AircraftManager::addExistingAircraft(Aircraft* aircraft)
{
    if (!aircraft) {
        qDebug() << "Cannot add null aircraft";
        return;
    }
    
    // Check if aircraft already exists in our list
    if (m_aircrafts.contains(aircraft)) {
        qDebug() << "Aircraft already exists in manager";
        return;
    }
    
    // Connect destruction signal
    connect(aircraft, &QObject::destroyed, 
            this, &AircraftManager::onAircraftDestroyed);
    
    // Set parent to this manager 
    aircraft->setParent(this);
    
    m_aircrafts.append(aircraft);
    
    emit aircraftCreated(aircraft);
    emit aircraftCountChanged(m_aircrafts.size());
    
    qDebug() << "Added existing aircraft" << aircraft->getCallSign() 
             << "at position" << aircraft->position() 
             << "Total:" << m_aircrafts.size();
}

void AircraftManager::removeAircraft(Aircraft* aircraft)
{
    if (!aircraft || !m_aircrafts.contains(aircraft)) {
        return;
    }
    
    m_aircrafts.removeAll(aircraft);
    
    emit aircraftRemoved(aircraft);
    emit aircraftCountChanged(m_aircrafts.size());
    
    // Delete the aircraft (will trigger onAircraftDestroyed)
    aircraft->deleteLater();
    
    qDebug() << "Removed aircraft, remaining:" << m_aircrafts.size();
}

void AircraftManager::clearAllAircraft()
{
    while (!m_aircrafts.isEmpty()) {
        Aircraft* aircraft = m_aircrafts.takeLast();
        emit aircraftRemoved(aircraft);
        aircraft->deleteLater();
    }
    
    emit aircraftCountChanged(0);
    qDebug() << "Cleared all aircraft";
}

void AircraftManager::setPolygonRegion(PolygonObject* polygon)
{
    m_polygonRegion = polygon;
    qDebug() << "Set polygon region for aircraft state detection";
}

void AircraftManager::startAllMovement()
{
    for (Aircraft* aircraft : m_aircrafts) {
        if (aircraft && !aircraft->isMoving()) {
            aircraft->startMovement();
        }
    }
    qDebug() << "Started movement for all aircraft";
}

void AircraftManager::stopAllMovement()
{
    for (Aircraft* aircraft : m_aircrafts) {
        if (aircraft && aircraft->isMoving()) {
            aircraft->stopMovement();
        }
    }
    qDebug() << "Stopped movement for all aircraft";
}

void AircraftManager::setAllUpdateInterval(int milliseconds)
{
    for (Aircraft* aircraft : m_aircrafts) {
        if (aircraft) {
            aircraft->setUpdateInterval(milliseconds);
        }
    }
    
    qDebug() << "Set update interval to" << milliseconds << "ms for all aircraft";
}

void AircraftManager::onAircraftDestroyed()
{
    // Remove destroyed aircraft from list
    m_aircrafts.removeAll(static_cast<Aircraft*>(sender()));
    emit aircraftCountChanged(m_aircrafts.size());
}

QPointF AircraftManager::generateRandomPosition()
{
    auto& config = ConfigManager::instance();
    auto boundary = config.getMovementBoundary();
    
    QRandomGenerator* rng = QRandomGenerator::global();
    
    double lon = boundary.x() + rng->generateDouble() * boundary.width();
    double lat = boundary.y() + rng->generateDouble() * boundary.height();
    
    return QPointF(lon, lat);
}

QPointF AircraftManager::generateRandomVelocity()
{
    auto& config = ConfigManager::instance();
    double speed = config.getDefaultAircraftSpeed();
    
    QRandomGenerator* rng = QRandomGenerator::global();
    
    // Generate random direction, scale by configured speed
    double vx = (rng->generateDouble() - 0.5) * speed * 4;
    double vy = (rng->generateDouble() - 0.5) * speed * 4;
    
    return QPointF(vx, vy);
}
