#include "aircraftlayer.h"
#include "../models/aircraft.h"
#include "../models/polygonobject.h"
#include "../core/viewtransform.h"
#include "../core/configmanager.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

AircraftLayer::AircraftLayer(QObject* parent)
    : MapLayer("Aircraft Layer", parent)
    , m_selectedAircraft(nullptr)
    , m_polygonRegion(nullptr)
{
}

AircraftLayer::~AircraftLayer()
{
    clearAircrafts();
}

void AircraftLayer::render(QPainter& painter, const ViewTransform& transform)
{
    if (!isVisible()) return;
    
    painter.save();
    painter.setOpacity(opacity());
    
    // Render all aircraft
    for (Aircraft* aircraft : m_aircrafts) {
        if (aircraft) {
            aircraft->render(painter, transform);
        }
    }
    
    painter.restore();
}

bool AircraftLayer::handleMouseEvent(QMouseEvent* event, const ViewTransform& transform)
{
    if (!isVisible() || event->type() != QEvent::MouseButtonPress) {
        return false;
    }
    
    if (event->button() == Qt::LeftButton) {
        QPointF screenPoint = event->pos();
        QPointF geoPoint = transform.screenToGeo(screenPoint);
        
        // Find aircraft at click position
        Aircraft* clickedAircraft = getAircraftAt(screenPoint, transform);
        
        if (clickedAircraft) {
            selectAircraft(clickedAircraft);
            emit aircraftClicked(clickedAircraft, geoPoint);
            return true; // Event handled
        } else {
            // Click on empty space - deselect current aircraft
            if (m_selectedAircraft) {
                deselectAircraft();
                return true;
            }
        }
    }
    
    return false; // Event not handled
}

void AircraftLayer::addAircraft(Aircraft* aircraft)
{
    if (!aircraft || m_aircrafts.contains(aircraft)) {
        return;
    }
    
    m_aircrafts.append(aircraft);
    
    // Connect signals
    connect(aircraft, &Aircraft::positionChanged, 
            this, &AircraftLayer::onAircraftPositionChanged);
    
    // Start movement if not already moving
    if (!aircraft->isMoving()) {
        aircraft->startMovement();
    }
    
    emit layerChanged();
    qDebug() << "Added aircraft to layer, total:" << m_aircrafts.size();
}

void AircraftLayer::removeAircraft(Aircraft* aircraft)
{
    if (!aircraft) return;
    
    // Deselect if it's the selected aircraft
    if (m_selectedAircraft == aircraft) {
        deselectAircraft();
    }
    
    // Disconnect signals
    disconnect(aircraft, nullptr, this, nullptr);
    
    m_aircrafts.removeAll(aircraft);
    emit layerChanged();
    
    qDebug() << "Removed aircraft from layer, remaining:" << m_aircrafts.size();
}

void AircraftLayer::clearAircrafts()
{
    for (Aircraft* aircraft : m_aircrafts) {
        if (aircraft) {
            disconnect(aircraft, nullptr, this, nullptr);
            aircraft->stopMovement();
        }
    }
    
    m_selectedAircraft = nullptr;
    m_aircrafts.clear();
    emit layerChanged();
    emit aircraftDeselected();
}

void AircraftLayer::setPolygonRegion(PolygonObject* polygon)
{
    m_polygonRegion = polygon;
    updateAircraftStates();
}

void AircraftLayer::onAircraftPositionChanged(const QPointF& newPosition)
{
    Aircraft* aircraft = qobject_cast<Aircraft*>(sender());
    if (!aircraft) return;
    
    // Update aircraft state based on region
    updateAircraftStates();
    
    emit layerChanged();
}

Aircraft* AircraftLayer::getAircraftAt(const QPointF& screenPoint, const ViewTransform& transform)
{
    QPointF geoPoint = transform.screenToGeo(screenPoint);
    
    // Check aircraft in reverse order (top to bottom)
    for (int i = m_aircrafts.size() - 1; i >= 0; --i) {
        Aircraft* aircraft = m_aircrafts[i];
        if (aircraft && aircraft->containsPoint(geoPoint)) {
            return aircraft;
        }
    }
    
    return nullptr;
}

void AircraftLayer::selectAircraft(Aircraft* aircraft)
{
    if (m_selectedAircraft == aircraft) return;
    
    // Deselect previous aircraft
    if (m_selectedAircraft) {
        m_selectedAircraft->setSelected(false);
    }
    
    // Select new aircraft
    m_selectedAircraft = aircraft;
    if (m_selectedAircraft) {
        m_selectedAircraft->setSelected(true);
        emit aircraftSelected(m_selectedAircraft);
        qDebug() << "Selected aircraft at" << m_selectedAircraft->position();
    }
    
    emit layerChanged();
}

void AircraftLayer::deselectAircraft()
{
    if (m_selectedAircraft) {
        m_selectedAircraft->setSelected(false);
        m_selectedAircraft = nullptr;
        emit aircraftDeselected();
        emit layerChanged();
        qDebug() << "Deselected aircraft";
    }
}

void AircraftLayer::updateAircraftStates()
{
    if (!m_polygonRegion) return;
    
    for (Aircraft* aircraft : m_aircrafts) {
        if (!aircraft) continue;
        
        bool inRegion = m_polygonRegion->containsPoint(aircraft->position());
        
        // Update state only if not selected (selected state takes priority)
        if (aircraft->state() != Aircraft::Selected) {
            Aircraft::State newState = inRegion ? Aircraft::InRegion : Aircraft::Normal;
            if (aircraft->state() != newState) {
                aircraft->setState(newState);
                qDebug() << "Aircraft state changed to" << (inRegion ? "InRegion" : "Normal");
            }
        }
    }
}
