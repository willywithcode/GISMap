#pragma once

#include <QWidget>
#include <QPixmap>
#include <QPointF>
#include <QVector>
#include <QPolygonF>
#include <QTimer>
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QMap>
#include <memory>

// Include necessary headers for the architecture components
#include "../core/viewtransform.h"
#include "../layers/aircraftlayer.h"
#include "../managers/aircraftmanager.h"
#include "../models/polygonobject.h"
#include "aircraft.h"

class MapWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit MapWidget(QWidget *parent = nullptr);
    void setShapefilePolygon(const QVector<QPolygonF> &shapes);
    void setPostgisPolygon(const QVector<QPolygonF> &shapes);
    
    // New architecture methods
    AircraftLayer* aircraftLayer() const { return m_aircraftLayer.get(); }
    AircraftManager* aircraftManager() const { return m_aircraftManager.get(); }
    ViewTransform* viewTransform() const { return m_viewTransform.get(); }
    
    // Public methods for UI control
    void setTileServer(const QString& serverName);
    void refreshMap();
    void clearTileCache();
    qint64 getTileCacheSize() const; // Get cache size in MB
    void prefetchTiles(int radius = 1); // Prefetch surrounding tiles
    
    // Polygon refresh
    void refreshPolygons();

signals:
    void coordinatesChanged(double lon, double lat, int zoom);
    void aircraftSelected(Aircraft* aircraft);
    void aircraftClicked(Aircraft* aircraft, const QPointF& position);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onAircraftSelected(Aircraft* aircraft);
    void onAircraftClicked(Aircraft* aircraft, const QPointF& position);

private:
    void loadTileMap();
    QPointF geoToPixel(double lon, double lat, int zoom, int tileSize = 256) const;
    void drawTiles(QPainter &painter);
    void drawPolygon(QPainter &painter, const QPolygonF &polygon, QColor color = Qt::red);
    void drawPolygons(QPainter &painter, const QVector<QPolygonF> &polygons, QColor color = Qt::blue);
    void fetchShapefiles();
    void fetchPostgis();
    void createHanoiPolygonInDatabase();  // Create Hanoi area polygon in PostgreSQL database
    QPixmap createFallbackTile(int tileX, int tileY) const;
    
    // New architecture methods
    void initializeArchitecture();
    void initializeFromConfig();
    void updateViewTransform();
    void createSampleAircraft();
    void loadExistingAircraft();  // Load aircraft from database
    QString getCurrentTileServerUrl() const;
    
    // Tile caching methods
    QString getTileCachePath(int z, int x, int y, const QString& serverName) const;
    bool loadTileFromCache(int z, int x, int y, const QString& serverName, QPixmap& tile);
    void saveTileToCache(int z, int x, int y, const QString& serverName, const QPixmap& tile);
    qint64 getCacheSizeBytes() const;
    void ensureCacheSize();
    
    // Asynchronous tile loading
    void loadTileAsync(int z, int x, int y, int offsetX, int offsetY);
    void onTileLoaded(int z, int x, int y, int offsetX, int offsetY, const QPixmap& tile);

    // Configuration-based settings
    int m_minZoom;
    int m_maxZoom;
    int m_tileSize;
    QString m_activeTileServer;
    bool m_cacheEnabled;
    QString m_cacheDirectory;
    int m_maxCacheSizeMB;

    // Legacy data (keeping for now during transition)
    int m_zoom;
    QPointF m_centerGeo;
    QVector<QPixmap> m_tiles;
    QVector<QPoint> m_tilePositions;
    QPolygonF m_polygon;
    QVector<QPolygonF> m_shapefilePolygons;
    QVector<QPolygonF> m_postgisPolygons;
    int m_centerTileX, m_centerTileY;
    
    // New architecture components
    std::unique_ptr<ViewTransform> m_viewTransform;
    std::unique_ptr<AircraftLayer> m_aircraftLayer;
    std::unique_ptr<AircraftManager> m_aircraftManager;
    std::unique_ptr<PolygonObject> m_hanoiPolygon;
    
    // Asynchronous loading components
    QNetworkAccessManager* m_networkManager;
    QMap<QString, QPair<QPoint, QPoint>> m_pendingTiles; // URL -> (tile coords, offset coords)
    
    // Update timer for smooth animation
    QTimer* m_updateTimer;
    
    // Drag functionality
    bool m_dragging;
    QPointF m_lastPanPoint;
    QPointF m_dragStartGeo;
};
