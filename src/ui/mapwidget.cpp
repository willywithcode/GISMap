#include "mapwidget.h"
#include "../core/configmanager.h"
#include <QPainter>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QEventLoop>
#include <QTimer>
#include <QFile>
#include <QDebug>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QDateTime>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <gdal.h>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <pqxx/pqxx>

// New architecture includes
#include "aircraft.h"
#include "polygonobject.h"

MapWidget::MapWidget(QWidget *parent) : QWidget(parent) {
    // Remove fixed minimum size, use size policy instead
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(400, 300);  // Set reasonable minimum but allow expansion
    
    // Initialize configuration-based settings
    initializeFromConfig();
    
    // Initialize new architecture
    initializeArchitecture();
    
    // Initialize network manager for async tile loading
    m_networkManager = new QNetworkAccessManager(this);
    
    // Add timer for regular updates to ensure aircraft movement is visible
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, [this]() {
        update(); // Force repaint to show aircraft movement
    });
    m_updateTimer->start(100); // Update every 100ms for smooth animation
    
    // Initialize drag state
    m_dragging = false;
    
    loadTileMap();
    
    // According to requirements: Only create ONE polygon area around Hanoi
    // This will be stored in PostgreSQL database and used for aircraft interaction
    // Remove hardcoded red polygon, use database-stored polygon instead
    
    // Create Hanoi area polygon for database storage
    createHanoiPolygonInDatabase();
    
    fetchShapefiles();  // Load Vietnam shapefile from SimpleMaps (for reference/display)
    fetchPostgis();     // Load the main Hanoi polygon from database
    
    // Create sample aircraft
    createSampleAircraft();
    
    // Emit initial coordinates
    emit coordinatesChanged(m_centerGeo.x(), m_centerGeo.y(), m_zoom);
}

void MapWidget::setShapefilePolygon(const QVector<QPolygonF> &shapes) {
    m_shapefilePolygons = shapes;
    update();
}

void MapWidget::setPostgisPolygon(const QVector<QPolygonF> &shapes) {
    m_postgisPolygons = shapes;
    update();
}

void MapWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Fill background with neutral color to avoid white gaps
    painter.fillRect(rect(), QColor(230, 240, 250)); // Light blue-gray background
    
    // Draw tiles
    drawTiles(painter);
    
    // Draw polygons using legacy method (for now)
    auto drawGeoPolygon = [&](const QPolygonF &geoPoly, QColor color) {
        QPolygonF pxPoly;
        QPointF centerPixel = geoToPixel(m_centerGeo.x(), m_centerGeo.y(), m_zoom);
        
        for (const QPointF &pt : geoPoly) {
            QPointF pixelPos = geoToPixel(pt.x(), pt.y(), m_zoom);
            // Translate to widget coordinates relative to center
            QPointF widgetPos = pixelPos - centerPixel + QPointF(width()/2, height()/2);
            pxPoly << widgetPos;
        }
        
        painter.setPen(QPen(color, 3));
        painter.setBrush(QBrush(color, Qt::SolidPattern));
        painter.setOpacity(0.3);
        painter.drawPolygon(pxPoly);
        painter.setOpacity(1.0);
        painter.setBrush(Qt::NoBrush);
        painter.drawPolygon(pxPoly);
    };
    
    // Draw polygons according to project requirements:
    // - BLUE polygons: Vietnam administrative boundaries from SimpleMaps shapefile (for reference)  
    // - GREEN polygon: Main Hanoi area polygon from PostgreSQL database (for aircraft interaction)
    // - Removed RED hardcoded polygon as per requirements
    
    for (const auto &poly : m_shapefilePolygons) drawGeoPolygon(poly, Qt::blue);     // Vietnam provinces from shapefile
    for (const auto &poly : m_postgisPolygons) drawGeoPolygon(poly, Qt::green);     // Hanoi area from database
    
    // Render aircraft layer using new architecture
    if (m_aircraftLayer && m_viewTransform) {
        updateViewTransform();
        m_aircraftLayer->render(painter, *m_viewTransform);
    }
}

void MapWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    
    qDebug() << "MapWidget resized to:" << size();
    
    // Force reload tiles to ensure full coverage after resize
    m_centerTileX = -999;  // Force reload
    m_centerTileY = -999;
    loadTileMap();
    
    // Update view transform with new size
    updateViewTransform();
}

void MapWidget::wheelEvent(QWheelEvent *event) {
    // Zoom in/out with mouse wheel using config-based limits
    int delta = event->angleDelta().y();
    if (delta > 0 && m_zoom < m_maxZoom) {
        m_zoom++;
        loadTileMap();
        emit coordinatesChanged(m_centerGeo.x(), m_centerGeo.y(), m_zoom);
    } else if (delta < 0 && m_zoom > m_minZoom) {
        m_zoom--;
        loadTileMap();
        emit coordinatesChanged(m_centerGeo.x(), m_centerGeo.y(), m_zoom);
    }
    event->accept();
}

void MapWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        // Try to handle with aircraft layer first
        if (m_aircraftLayer && m_viewTransform) {
            updateViewTransform();
            if (m_aircraftLayer->handleMouseEvent(event, *m_viewTransform)) {
                event->accept();
                return; // Event was handled by aircraft layer
            }
        }
        
        // Start drag operation
        m_dragging = true;
        m_lastPanPoint = event->pos();
        m_dragStartGeo = m_centerGeo;
        setCursor(Qt::ClosedHandCursor);
    }
    event->accept();
}

void MapWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        // Calculate drag offset in pixels
        QPointF currentPos = event->pos();
        QPointF dragOffset = currentPos - m_lastPanPoint;
        
        // Convert pixel offset to geographic offset using Web Mercator
        // At current zoom level, calculate how much geographic distance each pixel represents
        double scale = (1 << m_zoom) * m_tileSize; // Total pixels at this zoom level
        
        // Longitude conversion (straightforward)
        double lonOffset = -(dragOffset.x() * 360.0) / scale;
        
        // Latitude conversion (needs to account for Web Mercator projection)
        double currentLatRad = m_dragStartGeo.y() * M_PI / 180.0;
        double latScale = scale * cos(currentLatRad); // Account for latitude scaling
        double latOffset = (dragOffset.y() * 360.0) / latScale;
        
        // Calculate new center position
        double newLon = m_dragStartGeo.x() + lonOffset;
        double newLat = m_dragStartGeo.y() + latOffset;
        
        // Apply geographic constraints for Hanoi area
        newLon = qBound(105.0, newLon, 107.0);
        newLat = qBound(20.5, newLat, 21.5);
        
        // Update center position
        m_centerGeo = QPointF(newLon, newLat);
        
        // Force update view transform immediately
        updateViewTransform();
        
        // Force reload tiles every time during drag for responsiveness
        m_centerTileX = -999; // Force tile reload
        m_centerTileY = -999; 
        loadTileMap();
        
        // Update display immediately
        update();
        emit coordinatesChanged(m_centerGeo.x(), m_centerGeo.y(), m_zoom);
    }
    event->accept();
}

void MapWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        setCursor(Qt::ArrowCursor);
        
        // Force update view transform one more time
        updateViewTransform();
        
        // Final tile load to ensure we have all needed tiles
        // Reset center tile coordinates to ensure proper reload
        m_centerTileX = -999;
        m_centerTileY = -999;
        loadTileMap();
        
        // Trigger final update
        update();
    }
    event->accept();
}

void MapWidget::loadTileMap() {
    // Calculate center tile coordinates
    int newCenterTileX = (int)((m_centerGeo.x() + 180.0) / 360.0 * (1 << m_zoom));
    int newCenterTileY = (int)((1.0 - log(tan(m_centerGeo.y() * M_PI / 180.0) + 1.0 / cos(m_centerGeo.y() * M_PI / 180.0)) / M_PI) / 2.0 * (1 << m_zoom));
    
    // Only reload if center has changed significantly or tiles are empty
    // BUT: Always reload if we're dragging to ensure smooth tile updates
    if (!m_dragging && 
        abs(newCenterTileX - m_centerTileX) < 1 && 
        abs(newCenterTileY - m_centerTileY) < 1 && 
        !m_tiles.isEmpty()) {
        return; // Don't reload if we haven't moved much and not dragging
    }
    
    m_centerTileX = newCenterTileX;
    m_centerTileY = newCenterTileY;
    
    qDebug() << "Center tile coordinates:" << m_centerTileX << m_centerTileY << "at zoom" << m_zoom;
    
    // Calculate how many tiles we need to cover the widget completely
    int tilesX = (width() / m_tileSize) + 3;   // Add extra tiles to ensure full coverage
    int tilesY = (height() / m_tileSize) + 3;  // Add extra tiles to ensure full coverage
    
    // Set reasonable maximums to avoid performance issues
    tilesX = qMin(tilesX, 8);  // Maximum 8 tiles horizontally
    tilesY = qMin(tilesY, 6);  // Maximum 6 tiles vertically
    
    qDebug() << "Widget size:" << width() << "x" << height() << "Tile grid:" << tilesX << "x" << tilesY;
    
    m_tiles.clear();
    m_tilePositions.clear();
    
    // Get current tile server URL template
    QString urlTemplate = getCurrentTileServerUrl();
    
    // Load tiles in a grid around the center - prioritize cached tiles first
    QVector<QPair<QPoint, QPoint>> tilesToLoad; // (tile coords, offset coords)
    
    for (int dx = -tilesX/2; dx <= tilesX/2; dx++) {
        for (int dy = -tilesY/2; dy <= tilesY/2; dy++) {
            int tileX = m_centerTileX + dx;
            int tileY = m_centerTileY + dy;
            
            // Ensure tile coordinates are valid
            if (tileX < 0 || tileY < 0 || tileX >= (1 << m_zoom) || tileY >= (1 << m_zoom)) {
                continue;
            }
            
            QPixmap tile;
            
            // Try to load from cache first
            if (loadTileFromCache(m_zoom, tileX, tileY, m_activeTileServer, tile)) {
                m_tiles.append(tile);
                m_tilePositions.append(QPoint(dx, dy));
                qDebug() << "Loaded tile from cache at offset:" << dx << dy;
            } else {
                // Add to async loading queue
                tilesToLoad.append(qMakePair(QPoint(tileX, tileY), QPoint(dx, dy)));
                
                // Create placeholder tile for immediate display
                QPixmap placeholder = createFallbackTile(tileX, tileY);
                m_tiles.append(placeholder);
                m_tilePositions.append(QPoint(dx, dy));
            }
        }
    }
    
    // Start async loading for missing tiles
    for (const auto& tilePair : tilesToLoad) {
        QPoint tileCoords = tilePair.first;
        QPoint offsetCoords = tilePair.second;
        loadTileAsync(m_zoom, tileCoords.x(), tileCoords.y(), offsetCoords.x(), offsetCoords.y());
    }
    
    qDebug() << "Total tiles loaded:" << m_tiles.size() << "Async loading:" << tilesToLoad.size();
    update();
    
    // Trigger tile prefetching for smoother navigation (but not during drag for performance)
    if (!m_dragging) {
        QTimer::singleShot(500, this, [this]() { prefetchTiles(1); });
    }
}

void MapWidget::drawTiles(QPainter &painter) {
    if (m_tiles.isEmpty()) return;
    
    // Calculate the pixel position of the current center
    QPointF centerPixel = geoToPixel(m_centerGeo.x(), m_centerGeo.y(), m_zoom);
    
    // Calculate the pixel position of the center tile
    double centerTilePixelX = (m_centerTileX + 0.5) * m_tileSize;
    double centerTilePixelY = (m_centerTileY + 0.5) * m_tileSize;
    
    // Calculate offset between actual center and center tile center
    double offsetX = centerPixel.x() - centerTilePixelX;
    double offsetY = centerPixel.y() - centerTilePixelY;
    
    // Draw each tile at its correct position
    for (int i = 0; i < m_tiles.size(); ++i) {
        QPoint tileOffset = m_tilePositions[i];
        
        // Calculate tile position in widget coordinates
        // Position relative to center of widget, accounting for geographic offset
        double x = width() / 2 + tileOffset.x() * m_tileSize - offsetX;
        double y = height() / 2 + tileOffset.y() * m_tileSize - offsetY;
        
        // Create tile rectangle, ensuring it covers any potential fractional pixels
        QRect tileRect(static_cast<int>(floor(x)), static_cast<int>(floor(y)), 
                       m_tileSize + 1, m_tileSize + 1);
        
        // Only draw if tile is at least partially visible
        QRect widgetRect(0, 0, width(), height());
        if (tileRect.intersects(widgetRect)) {
            painter.drawPixmap(tileRect, m_tiles[i]);
        }
    }
    
    // Debug: Draw widget boundaries to check coverage
    if (qgetenv("DEBUG_TILES") == "1") {
        painter.setPen(QPen(Qt::red, 2));
        painter.drawRect(0, 0, width()-1, height()-1);
    }
}

void MapWidget::drawPolygon(QPainter &painter, const QPolygonF &polygon, QColor color) {
    if (!polygon.isEmpty()) {
        painter.setPen(QPen(color, 2));
        painter.drawPolygon(polygon);
    }
}

void MapWidget::drawPolygons(QPainter &painter, const QVector<QPolygonF> &polygons, QColor color) {
    for (const auto &poly : polygons) {
        drawPolygon(painter, poly, color);
    }
}

void MapWidget::fetchShapefiles() {
    // Initialize GDAL/OGR for reading vector data
    GDALAllRegister();
    
    ConfigManager& config = ConfigManager::instance();
    
    // Try to load vector data files in priority order: GeoJSON first, then shapefiles
    QStringList vectorDataPaths = {
        "resources/shapefiles/vn.json",     // GeoJSON format (priority)
        "resources/shapefiles/vn.shp",      // Shapefile format  
        "vn.json",                          // Fallback to current directory
        "vn.shp"                           // Fallback shapefile
    };
    
    for (const QString& path : vectorDataPaths) {
        QFileInfo fileInfo(path);
        if (!fileInfo.exists()) {
            qDebug() << "Vector data file not found:" << path;
            continue;
        }
        
        qDebug() << "Attempting to load vector data from:" << path;
        
        GDALDataset *poDS = (GDALDataset*) GDALOpenEx(path.toLocal8Bit().data(), GDAL_OF_VECTOR, NULL, NULL, NULL);
        if (poDS) {
            qDebug() << "Successfully opened vector data:" << path;
            
            OGRLayer *poLayer = poDS->GetLayer(0);
            if (!poLayer) {
                qDebug() << "No layer found in vector data:" << path;
                GDALClose(poDS);
                continue;
            }
            
            OGRFeature *poFeature;
            poLayer->ResetReading();
            
            int featureCount = 0;
            int polygonCount = 0;
            
            // Process all features (no limit for GeoJSON, limited for performance with large shapefiles)
            int maxFeatures = path.endsWith(".json") ? 1000 : 100; // More features for GeoJSON
            
            while ((poFeature = poLayer->GetNextFeature()) != nullptr && featureCount < maxFeatures) {
                OGRGeometry *poGeometry = poFeature->GetGeometryRef();
                if (poGeometry) {
                    OGRwkbGeometryType geomType = wkbFlatten(poGeometry->getGeometryType());
                    
                    if (geomType == wkbPolygon) {
                        // Handle single polygon
                        OGRPolygon *poPolygon = (OGRPolygon *) poGeometry;
                        OGRLinearRing *ring = poPolygon->getExteriorRing();
                        if (ring && ring->getNumPoints() > 0) {
                            QPolygonF qpoly;
                            for (int i = 0; i < ring->getNumPoints(); ++i) {
                                qpoly << QPointF(ring->getX(i), ring->getY(i));
                            }
                            m_shapefilePolygons.append(qpoly);
                            polygonCount++;
                        }
                    } 
                    else if (geomType == wkbMultiPolygon) {
                        // Handle multipolygon (important for complex provinces)
                        OGRMultiPolygon *poMultiPolygon = (OGRMultiPolygon *) poGeometry;
                        for (int i = 0; i < poMultiPolygon->getNumGeometries(); ++i) {
                            OGRGeometry *poSubGeom = poMultiPolygon->getGeometryRef(i);
                            if (poSubGeom && wkbFlatten(poSubGeom->getGeometryType()) == wkbPolygon) {
                                OGRPolygon *poPolygon = (OGRPolygon *) poSubGeom;
                                OGRLinearRing *ring = poPolygon->getExteriorRing();
                                if (ring && ring->getNumPoints() > 0) {
                                    QPolygonF qpoly;
                                    for (int j = 0; j < ring->getNumPoints(); ++j) {
                                        qpoly << QPointF(ring->getX(j), ring->getY(j));
                                    }
                                    m_shapefilePolygons.append(qpoly);
                                    polygonCount++;
                                }
                            }
                        }
                    }
                    
                    // Log feature properties for GeoJSON
                    if (path.endsWith(".json") && featureCount < 10) {
                        const char* name = poFeature->GetFieldAsString("name");
                        if (name && strlen(name) > 0) {
                            qDebug() << "  Feature" << featureCount << ": Province =" << name;
                        }
                    }
                }
                
                OGRFeature::DestroyFeature(poFeature);
                featureCount++;
            }
            
            qDebug() << "Successfully loaded" << featureCount << "features with" << polygonCount << "polygons from" << path;
            
            if (path.endsWith(".json")) {
                qDebug() << "Loaded Vietnam administrative boundaries from GeoJSON";
            } else {
                qDebug() << "Loaded boundaries from shapefile";
            }
            
            GDALClose(poDS);
            break; // Successfully loaded, don't try other paths
        } else {
            qDebug() << "Failed to open vector data:" << path << "- GDAL Error:" << CPLGetLastErrorMsg();
        }
    }
    
    if (m_shapefilePolygons.isEmpty()) {
        qDebug() << "No vector data loaded successfully. Application will work without administrative boundaries.";
        qDebug() << "To add Vietnam provinces, place vn.json in resources/shapefiles/ directory";
    } else {
        qDebug() << "Total polygons loaded:" << m_shapefilePolygons.size();
        qDebug() << "Vietnam administrative boundaries ready for display";
    }
}

void MapWidget::fetchPostgis() {
    // Connect and query PostGIS using configuration settings
    try {
        ConfigManager& config = ConfigManager::instance();
        
        // Build connection string from configuration
        QString connString = QString("host=%1 port=%2 dbname=%3 user=%4 password=%5 connect_timeout=%6")
            .arg(config.getDatabaseHost())
            .arg(config.getDatabasePort())
            .arg(config.getDatabaseName())
            .arg(config.getDatabaseUsername())
            .arg(config.getDatabasePassword())
            .arg(config.getDatabaseConnectionTimeout());
        
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        // Get table configuration
        QString tableName = config.getDatabasePolygonsTableName();
        QString geomColumn = config.getDatabasePolygonsGeometryColumn();
        int limit = config.getDatabasePolygonsLimit();
        
        QString query = QString("SELECT ST_AsText(%1) FROM %2 LIMIT %3")
            .arg(geomColumn)
            .arg(tableName)
            .arg(limit);
        
        pqxx::result r = txn.exec(query.toStdString());
        
        for (auto row : r) {
            std::string wkt = row[0].c_str();
            OGRGeometry *geom = nullptr;
            OGRGeometryFactory::createFromWkt(wkt.c_str(), nullptr, &geom);
            if (geom && wkbFlatten(geom->getGeometryType()) == wkbPolygon) {
                OGRPolygon *poPolygon = (OGRPolygon *) geom;
                OGRLinearRing *ring = poPolygon->getExteriorRing();
                QPolygonF qpoly;
                for (int i = 0; i < ring->getNumPoints(); ++i) {
                    qpoly << QPointF(ring->getX(i), ring->getY(i));
                }
                m_postgisPolygons.append(qpoly);
            }
            if (geom) OGRGeometryFactory::destroyGeometry(geom);
        }
        
        qDebug() << "Loaded" << m_postgisPolygons.size() << "polygons from PostGIS";
        
        // CRITICAL: Update the polygon region for aircraft interaction
        // This ensures aircraft change color when entering the database-stored Hanoi area
        if (!m_postgisPolygons.isEmpty() && m_hanoiPolygon) {
            // Use the first polygon from database as the main Hanoi area for aircraft interaction
            QPolygonF mainHanoiArea = m_postgisPolygons.first();
            m_hanoiPolygon->setPolygon(mainHanoiArea);
            
            qDebug() << "Set Hanoi polygon for aircraft interaction from database";
            qDebug() << "Polygon has" << mainHanoiArea.size() << "points";
            qDebug() << "Aircraft will change color when entering this area";
        }
        
    } catch (const std::exception &e) {
        qDebug() << "PostGIS error:" << e.what();
        qDebug() << "Aircraft interaction will use fallback polygon";
    }
}

QPointF MapWidget::geoToPixel(double lon, double lat, int zoom, int tileSize) const {
    // Chuyển đổi toạ độ địa lý sang pixel (Web Mercator)
    double x = (lon + 180.0) / 360.0 * (1 << zoom) * tileSize;
    double y = (1.0 - log(tan(lat * M_PI / 180.0) + 1.0 / cos(lat * M_PI / 180.0)) / M_PI) / 2.0 * (1 << zoom) * tileSize;
    return QPointF(x, y);
}

QPixmap MapWidget::createFallbackTile(int tileX, int tileY) const {
    QPixmap tile(m_tileSize, m_tileSize);
    tile.fill(QColor(240, 248, 255)); // Light blue background
    
    QPainter painter(&tile);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw grid lines based on tile size
    painter.setPen(QPen(QColor(200, 200, 200), 1));
    int gridSpacing = m_tileSize / 8;
    for (int i = 0; i <= m_tileSize; i += gridSpacing) {
        painter.drawLine(i, 0, i, m_tileSize);
        painter.drawLine(0, i, m_tileSize, i);
    }
    
    // Draw tile coordinates
    painter.setPen(QPen(Qt::darkGray, 1));
    painter.setFont(QFont("Arial", qMax(8, m_tileSize / 32)));
    QString tileInfo = QString("Tile %1/%2\nZoom %3").arg(tileX).arg(tileY).arg(m_zoom);
    painter.drawText(QRect(10, 10, m_tileSize - 20, 50), Qt::AlignLeft | Qt::AlignTop, tileInfo);
    
    // Draw simple land/water pattern
    painter.setPen(QPen(QColor(180, 180, 180), 1));
    painter.setBrush(QBrush(QColor(220, 240, 220))); // Light green for land
    
    // Create simple geometric shapes to simulate land masses
    for (int i = 0; i < 3; ++i) {
        int x = (tileX * 37 + i * 67) % (m_tileSize - 60);
        int y = (tileY * 43 + i * 53) % (m_tileSize - 40);
        int w = 30 + (tileX + tileY + i) % 60;
        int h = 20 + (tileX - tileY + i) % 40;
        painter.drawEllipse(x, y, w, h);
    }
    
    return tile;
}

// New architecture implementation methods
void MapWidget::initializeFromConfig()
{
    ConfigManager& config = ConfigManager::instance();
    
    // Load map settings from configuration
    QPointF defaultCenter = config.getDefaultMapCenter();
    m_centerGeo.setX(defaultCenter.x());
    m_centerGeo.setY(defaultCenter.y());
    m_zoom = config.getDefaultZoom();
    m_minZoom = config.getMinZoom();
    m_maxZoom = config.getMaxZoom();
    m_tileSize = config.getTileSize();
    
    // Load tile server settings
    m_activeTileServer = "openstreetmap"; // Default to OpenStreetMap
    
    // Load cache settings
    m_cacheEnabled = config.isTileCacheEnabled();
    m_cacheDirectory = config.getTileCacheDirectory();
    m_maxCacheSizeMB = config.getMaxCacheSizeMB();
    
    qDebug() << "MapWidget initialized from config:";
    qDebug() << "  Center:" << m_centerGeo;
    qDebug() << "  Zoom:" << m_zoom << "(" << m_minZoom << "-" << m_maxZoom << ")";
    qDebug() << "  Tile size:" << m_tileSize;
    qDebug() << "  Cache enabled:" << m_cacheEnabled;
}

QString MapWidget::getCurrentTileServerUrl() const
{
    ConfigManager& config = ConfigManager::instance();
    
    if (m_activeTileServer == "openstreetmap") {
        return "https://tile.openstreetmap.org/{z}/{x}/{y}.png";
    } else if (m_activeTileServer == "satellite") {
        return "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}";
    }
    
    // Default fallback
    return "https://tile.openstreetmap.org/{z}/{x}/{y}.png";
}

void MapWidget::setTileServer(const QString& serverName)
{
    if (m_activeTileServer != serverName) {
        m_activeTileServer = serverName;
        qDebug() << "Switched to tile server:" << serverName;
        
        // Force reload tiles with new server
        m_tiles.clear();
        m_tilePositions.clear();
        loadTileMap();
        update();
    }
}

void MapWidget::refreshMap()
{
    qDebug() << "Refreshing map tiles";
    
    // Clear existing tiles to force reload
    m_tiles.clear();
    m_tilePositions.clear();
    
    // Reload tiles
    loadTileMap();
    update();
}

// Tile caching implementation
QString MapWidget::getTileCachePath(int z, int x, int y, const QString& serverName) const
{
    return QString("%1/%2/%3/%4/%5.png")
        .arg(m_cacheDirectory)
        .arg(serverName)
        .arg(z)
        .arg(x)
        .arg(y);
}

bool MapWidget::loadTileFromCache(int z, int x, int y, const QString& serverName, QPixmap& tile)
{
    if (!m_cacheEnabled) {
        return false;
    }
    
    QString cachePath = getTileCachePath(z, x, y, serverName);
    QFileInfo fileInfo(cachePath);
    
    if (fileInfo.exists() && fileInfo.isFile()) {
        // Check if the cached tile is not too old (e.g., 7 days)
        QDateTime lastModified = fileInfo.lastModified();
        QDateTime now = QDateTime::currentDateTime();
        if (lastModified.daysTo(now) > 7) {
            qDebug() << "Cached tile is too old, removing:" << cachePath;
            QFile::remove(cachePath);
            return false;
        }
        
        if (tile.load(cachePath)) {
            qDebug() << "Loaded tile from cache:" << cachePath;
            return true;
        } else {
            qDebug() << "Failed to load cached tile:" << cachePath;
            QFile::remove(cachePath); // Remove corrupted cache file
        }
    }
    
    return false;
}

void MapWidget::saveTileToCache(int z, int x, int y, const QString& serverName, const QPixmap& tile)
{
    if (!m_cacheEnabled || tile.isNull()) {
        return;
    }
    
    QString cachePath = getTileCachePath(z, x, y, serverName);
    QFileInfo fileInfo(cachePath);
    
    // Create directory structure if it doesn't exist
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(dir.absolutePath())) {
            qDebug() << "Failed to create cache directory:" << dir.absolutePath();
            return;
        }
    }
    
    // Save tile to cache
    if (tile.save(cachePath, "PNG")) {
        qDebug() << "Saved tile to cache:" << cachePath;
        
        // Check cache size and clean if necessary
        ensureCacheSize();
    } else {
        qDebug() << "Failed to save tile to cache:" << cachePath;
    }
}

void MapWidget::clearTileCache()
{
    if (!m_cacheEnabled) {
        return;
    }
    
    QDir cacheDir(m_cacheDirectory);
    if (cacheDir.exists()) {
        // Remove all cache files recursively
        cacheDir.removeRecursively();
        cacheDir.mkpath(m_cacheDirectory);
        qDebug() << "Cleared tile cache directory:" << m_cacheDirectory;
    }
}

qint64 MapWidget::getTileCacheSize() const
{
    qint64 sizeBytes = getCacheSizeBytes();
    return sizeBytes / (1024 * 1024); // Convert to MB
}

void MapWidget::prefetchTiles(int radius)
{
    if (!m_cacheEnabled) {
        qDebug() << "Cache disabled, skipping tile prefetching";
        return;
    }
    
    qDebug() << "Prefetching tiles with radius:" << radius;
    
    // Calculate current center tile
    int centerTileX = (int)((m_centerGeo.x() + 180.0) / 360.0 * (1 << m_zoom));
    int centerTileY = (int)((1.0 - log(tan(m_centerGeo.y() * M_PI / 180.0) + 1.0 / cos(m_centerGeo.y() * M_PI / 180.0)) / M_PI) / 2.0 * (1 << m_zoom));
    
    QString urlTemplate = getCurrentTileServerUrl();
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    
    int tilesRequested = 0;
    int maxTiles = 25; // Limit prefetching to avoid overwhelming the server
    
    // Prefetch tiles in expanding rings around center
    for (int r = 1; r <= radius && tilesRequested < maxTiles; r++) {
        for (int dx = -r; dx <= r && tilesRequested < maxTiles; dx++) {
            for (int dy = -r; dy <= r && tilesRequested < maxTiles; dy++) {
                // Skip if not on the ring border (avoid duplicate requests)
                if (abs(dx) != r && abs(dy) != r) continue;
                
                int tileX = centerTileX + dx;
                int tileY = centerTileY + dy;
                
                // Ensure tile coordinates are valid
                if (tileX < 0 || tileY < 0 || tileX >= (1 << m_zoom) || tileY >= (1 << m_zoom)) {
                    continue;
                }
                
                // Check if tile is already cached
                QPixmap cachedTile;
                if (loadTileFromCache(m_zoom, tileX, tileY, m_activeTileServer, cachedTile)) {
                    continue; // Already cached, skip
                }
                
                // Download tile asynchronously
                QString url = urlTemplate;
                url.replace("{z}", QString::number(m_zoom));
                url.replace("{x}", QString::number(tileX));
                url.replace("{y}", QString::number(tileY));
                
                QNetworkRequest request{QUrl(url)};
                request.setHeader(QNetworkRequest::UserAgentHeader, "GISMap/1.0 (Qt Application)");
                request.setRawHeader("Referer", "https://www.openstreetmap.org/");
                
                QNetworkReply* reply = manager->get(request);
                
                // Use lambda to capture tile coordinates for async handling
                connect(reply, &QNetworkReply::finished, [this, reply, tileX, tileY]() {
                    if (reply->error() == QNetworkReply::NoError) {
                        QPixmap tile;
                        tile.loadFromData(reply->readAll());
                        if (!tile.isNull()) {
                            saveTileToCache(m_zoom, tileX, tileY, m_activeTileServer, tile);
                            qDebug() << "Prefetched tile:" << tileX << tileY;
                        }
                    }
                    reply->deleteLater();
                });
                
                tilesRequested++;
            }
        }
    }
    
    qDebug() << "Requested prefetching of" << tilesRequested << "tiles";
}

void MapWidget::loadTileAsync(int z, int x, int y, int offsetX, int offsetY)
{
    QString urlTemplate = getCurrentTileServerUrl();
    QString url = urlTemplate;
    url.replace("{z}", QString::number(z));
    url.replace("{x}", QString::number(x));
    url.replace("{y}", QString::number(y));
    
    // Check if this tile is already being loaded
    if (m_pendingTiles.contains(url)) {
        return;
    }
    
    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::UserAgentHeader, "GISMap/1.0 (Qt Application)");
    request.setRawHeader("Referer", "https://www.openstreetmap.org/");
    
    // Track this tile as pending
    m_pendingTiles[url] = qMakePair(QPoint(x, y), QPoint(offsetX, offsetY));
    
    QNetworkReply* reply = m_networkManager->get(request);
    
    // Connect the reply to our handler
    connect(reply, &QNetworkReply::finished, [this, reply, z, x, y, offsetX, offsetY, url]() {
        // Remove from pending list
        m_pendingTiles.remove(url);
        
        if (reply->error() == QNetworkReply::NoError) {
            QPixmap tile;
            tile.loadFromData(reply->readAll());
            if (!tile.isNull()) {
                // Save to cache
                saveTileToCache(z, x, y, m_activeTileServer, tile);
                // Handle the loaded tile
                onTileLoaded(z, x, y, offsetX, offsetY, tile);
            } else {
                // Create fallback tile for invalid data
                QPixmap fallbackTile = createFallbackTile(x, y);
                onTileLoaded(z, x, y, offsetX, offsetY, fallbackTile);
            }
        } else {
            qDebug() << "Async tile load failed:" << reply->errorString();
            // Create fallback tile for network error
            QPixmap fallbackTile = createFallbackTile(x, y);
            onTileLoaded(z, x, y, offsetX, offsetY, fallbackTile);
        }
        
        reply->deleteLater();
    });
    
    qDebug() << "Started async loading of tile:" << x << y << "at zoom" << z;
}

void MapWidget::onTileLoaded(int z, int x, int y, int offsetX, int offsetY, const QPixmap& tile)
{
    // Only add the tile if it matches current zoom and is needed
    if (z != m_zoom) {
        qDebug() << "Ignoring tile for different zoom level:" << z << "vs" << m_zoom;
        return;
    }
    
    // Check if this tile offset is still relevant for current view
    bool tileStillNeeded = false;
    for (int i = 0; i < m_tilePositions.size(); ++i) {
        if (m_tilePositions[i] == QPoint(offsetX, offsetY)) {
            tileStillNeeded = true;
            break;
        }
    }
    
    if (tileStillNeeded) {
        // Add tile to current view
        m_tiles.append(tile);
        m_tilePositions.append(QPoint(offsetX, offsetY));
        
        qDebug() << "Added async loaded tile at offset:" << offsetX << offsetY;
        
        // Trigger repaint
        update();
    } else {
        qDebug() << "Discarding async loaded tile, no longer needed:" << offsetX << offsetY;
    }
}

// Implementation of missing architecture methods
void MapWidget::initializeArchitecture()
{
    qDebug() << "Initializing MapWidget architecture components";
    
    // Initialize ViewTransform with current settings
    m_viewTransform = std::make_unique<ViewTransform>(m_centerGeo, m_zoom, size(), this);
    
    // Initialize AircraftManager
    m_aircraftManager = std::make_unique<AircraftManager>(this);
    
    // Initialize AircraftLayer
    m_aircraftLayer = std::make_unique<AircraftLayer>(this);
    
    // Initialize polygon region (Hanoi area)
    m_hanoiPolygon = std::make_unique<PolygonObject>(this);
    
    // Connect aircraft manager to layer
    connect(m_aircraftManager.get(), &AircraftManager::aircraftCreated,
            this, [this](Aircraft* aircraft) {
                m_aircraftLayer->addAircraft(aircraft);
            });
    
    connect(m_aircraftManager.get(), &AircraftManager::aircraftRemoved,
            this, [this](Aircraft* aircraft) {
                m_aircraftLayer->removeAircraft(aircraft);
            });
    
    // Connect aircraft layer signals to mapwidget signals
    connect(m_aircraftLayer.get(), &AircraftLayer::aircraftSelected,
            this, &MapWidget::aircraftSelected);
    
    connect(m_aircraftLayer.get(), &AircraftLayer::aircraftClicked,
            this, &MapWidget::aircraftClicked);
    
    // Connect internal slots
    connect(m_aircraftLayer.get(), &AircraftLayer::aircraftSelected,
            this, &MapWidget::onAircraftSelected);
    
    connect(m_aircraftLayer.get(), &AircraftLayer::aircraftClicked,
            this, &MapWidget::onAircraftClicked);
    
    // Set up polygon region for both manager and layer
    m_aircraftManager->setPolygonRegion(m_hanoiPolygon.get());
    m_aircraftLayer->setPolygonRegion(m_hanoiPolygon.get());
    
    // Connect view transform change signals
    connect(m_viewTransform.get(), &ViewTransform::transformChanged,
            this, [this]() { update(); });
    
    qDebug() << "Architecture components initialized successfully";
}

void MapWidget::updateViewTransform()
{
    if (!m_viewTransform) {
        return;
    }
    
    // Update view transform with current map state
    m_viewTransform->setCenter(m_centerGeo);
    m_viewTransform->setZoom(m_zoom);
    m_viewTransform->setViewSize(size());
}

void MapWidget::createSampleAircraft()
{
    if (!m_aircraftManager) {
        qDebug() << "Aircraft manager not initialized, cannot create sample aircraft";
        return;
    }
    
    qDebug() << "Creating sample aircraft";
    
    // Create several aircraft at different positions around Hanoi
    // Some starting from Gulf of Tonkin (east) moving toward Hanoi
    QVector<QPointF> startPositions = {
        QPointF(106.2, 20.8),   // From Gulf of Tonkin, moving west
        QPointF(106.1, 21.1),   // From northeast, moving southwest  
        QPointF(105.3, 21.2),   // From west, moving east
        QPointF(105.4, 20.7),   // From southwest, moving northeast
        QPointF(105.85, 21.03), // Starting in Hanoi center
        QPointF(106.0, 21.0),   // From east, moving west through Hanoi
        QPointF(105.5, 21.1)    // From west, moving east through Hanoi
    };
    
    // Corresponding velocities to ensure aircraft move through Hanoi region
    QVector<QPointF> velocities = {
        QPointF(-0.0008, 0.0003),  // Moving west-northwest
        QPointF(-0.0005, -0.0006), // Moving southwest
        QPointF(0.0007, -0.0002),  // Moving east-southeast
        QPointF(0.0006, 0.0008),   // Moving northeast
        QPointF(0.0003, 0.0004),   // Moving northeast slowly
        QPointF(-0.0009, 0.0001),  // Moving west through Hanoi
        QPointF(0.0008, -0.0003)   // Moving east through Hanoi
    };
    
    for (int i = 0; i < startPositions.size(); ++i) {
        const QPointF& position = startPositions[i];
        const QPointF& velocity = velocities[i];
        
        Aircraft* aircraft = m_aircraftManager->createAircraft(position);
        if (aircraft) {
            // Set specific velocity for this aircraft
            aircraft->setVelocity(velocity);
            
            // Calculate heading from velocity
            double heading = qAtan2(velocity.x(), velocity.y()) * 180.0 / M_PI;
            aircraft->setHeading(heading);
            
            // Start movement
            aircraft->startMovement();
            
            qDebug() << "Created aircraft at position:" << position << "with velocity:" << velocity;
        }
    }
    
    qDebug() << "Created" << m_aircraftManager->aircraftCount() << "sample aircraft";
}

void MapWidget::onAircraftSelected(Aircraft* aircraft)
{
    if (aircraft) {
        qDebug() << "Aircraft selected:" << aircraft->getInfo();
        // Additional UI feedback could be added here
    } else {
        qDebug() << "Aircraft deselected";
    }
}

void MapWidget::onAircraftClicked(Aircraft* aircraft, const QPointF& position)
{
    if (aircraft) {
        qDebug() << "Aircraft clicked:" << aircraft->getInfo() << "at position:" << position;
        // Additional click handling could be added here
        // For example, show aircraft details in a popup or side panel
    }
}

qint64 MapWidget::getCacheSizeBytes() const
{
    if (!m_cacheEnabled) {
        return 0;
    }
    
    QDir cacheDir(m_cacheDirectory);
    if (!cacheDir.exists()) {
        return 0;
    }
    
    qint64 totalSize = 0;
    
    // Recursively calculate directory size
    QDirIterator it(m_cacheDirectory, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QFileInfo fileInfo = it.fileInfo();
        if (fileInfo.isFile()) {
            totalSize += fileInfo.size();
        }
    }
    
    return totalSize;
}

void MapWidget::ensureCacheSize()
{
    if (!m_cacheEnabled) {
        return;
    }
    
    qint64 maxSizeBytes = static_cast<qint64>(m_maxCacheSizeMB) * 1024 * 1024;
    qint64 currentSize = getCacheSizeBytes();
    
    if (currentSize <= maxSizeBytes) {
        return; // Cache size is within limits
    }
    
    qDebug() << "Cache size exceeded limit:" << currentSize / (1024*1024) << "MB /" << m_maxCacheSizeMB << "MB";
    
    // Get all cache files with modification times
    QList<QPair<QDateTime, QString>> cacheFiles;
    QDirIterator it(m_cacheDirectory, QStringList() << "*.png", QDir::Files, QDirIterator::Subdirectories);
    
    while (it.hasNext()) {
        it.next();
        QFileInfo fileInfo = it.fileInfo();
        cacheFiles.append(qMakePair(fileInfo.lastModified(), fileInfo.absoluteFilePath()));
    }
    
    // Sort by modification time (oldest first)
    std::sort(cacheFiles.begin(), cacheFiles.end());
    
    // Remove oldest files until we're under the size limit
    qint64 removedSize = 0;
    int filesRemoved = 0;
    
    for (const auto& filePair : cacheFiles) {
        if (currentSize - removedSize <= maxSizeBytes) {
            break;
        }
        
        QFileInfo fileInfo(filePair.second);
        qint64 fileSize = fileInfo.size();
        
        if (QFile::remove(filePair.second)) {
            removedSize += fileSize;
            filesRemoved++;
        }
    }
    
    qDebug() << "Removed" << filesRemoved << "cache files, freed" << removedSize / (1024*1024) << "MB";
}

void MapWidget::createHanoiPolygonInDatabase()
{
    try {
        ConfigManager& config = ConfigManager::instance();
        
        QString connString = QString("host=%1 port=%2 dbname=%3 user=%4 password=%5 connect_timeout=%6")
            .arg(config.getDatabaseHost())
            .arg(config.getDatabasePort())
            .arg(config.getDatabaseName())
            .arg(config.getDatabaseUsername())
            .arg(config.getDatabasePassword())
            .arg(config.getDatabaseConnectionTimeout());
        
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        // Get table configuration
        QString tableName = config.getDatabasePolygonsTableName();
        QString geomColumn = config.getDatabasePolygonsGeometryColumn();
        
        // Check if the polygon already exists
        QString checkQuery = QString("SELECT COUNT(*) FROM %1 WHERE name = 'Hanoi Area'")
            .arg(tableName);
        
        pqxx::result checkResult = txn.exec(checkQuery.toStdString());
        int count = checkResult[0][0].as<int>();
        
        if (count == 0) {
            // Create a larger polygon area around Hanoi for aircraft interaction
            // This represents a detection zone, not just Hoan Kiem Lake
            QString wktPolygon = "POLYGON((105.7 20.8, 105.7 21.3, 106.1 21.3, 106.1 20.8, 105.7 20.8))";
            
            QString insertQuery = QString("INSERT INTO %1 (name, %2) VALUES ('Hanoi Area', ST_GeomFromText('%3', 4326))")
                .arg(tableName)
                .arg(geomColumn)
                .arg(wktPolygon);
            
            txn.exec(insertQuery.toStdString());
            
            // OPTIONAL: Add a smaller demo polygon for testing if you want to see the difference
            // This creates a visible contrast between having/not having shapefile data
            QString demoPolygon = "POLYGON((105.85 21.00, 105.85 21.05, 105.90 21.05, 105.90 21.00, 105.85 21.00))";
            QString demoQuery = QString("INSERT INTO %1 (name, %2) VALUES ('Demo Small Area', ST_GeomFromText('%3', 4326))")
                .arg(tableName)
                .arg(geomColumn)
                .arg(demoPolygon);
            
            txn.exec(demoQuery.toStdString());
            
            txn.commit();
            
            qDebug() << "Created Hanoi area polygon in PostgreSQL database";
            qDebug() << "Main polygon covers area from 105.7°E to 106.1°E, 20.8°N to 21.3°N";
            qDebug() << "Demo polygon covers smaller area around 105.85-105.90°E, 21.00-21.05°N";
        } else {
            qDebug() << "Hanoi area polygon already exists in database";
        }
        
    } catch (const std::exception &e) {
        qDebug() << "Error creating Hanoi polygon in database:" << e.what();
        qDebug() << "Application will continue without database polygon";
    }
}
