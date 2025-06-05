#pragma once
#include "maplayer.h"
#include "../models/aircraft.h"
#include <QVector>

class PolygonObject;

/**
 * @brief Layer for managing and rendering aircraft objects
 */
class AircraftLayer : public MapLayer {
    Q_OBJECT
public:
    explicit AircraftLayer(QObject* parent = nullptr);
    ~AircraftLayer();

    // MapLayer interface
    void render(QPainter& painter, const ViewTransform& transform) override;
    bool handleMouseEvent(QMouseEvent* event, const ViewTransform& transform) override;

    // Aircraft management
    void addAircraft(Aircraft* aircraft);
    void removeAircraft(Aircraft* aircraft);
    void clearAircrafts();
    
    QVector<Aircraft*> aircrafts() const { return m_aircrafts; }
    Aircraft* selectedAircraft() const { return m_selectedAircraft; }
    
    // Region management for state detection
    void setPolygonRegion(PolygonObject* polygon);
    PolygonObject* polygonRegion() const { return m_polygonRegion; }

signals:
    void aircraftSelected(Aircraft* aircraft);
    void aircraftDeselected();
    void aircraftClicked(Aircraft* aircraft, const QPointF& position);

private slots:
    void onAircraftPositionChanged(const QPointF& newPosition);

private:
    void updateAircraftStates();
    Aircraft* getAircraftAt(const QPointF& screenPoint, const ViewTransform& transform);
    void selectAircraft(Aircraft* aircraft);
    void deselectAircraft();
    
    QVector<Aircraft*> m_aircrafts;
    Aircraft* m_selectedAircraft = nullptr;
    PolygonObject* m_polygonRegion = nullptr;
};
