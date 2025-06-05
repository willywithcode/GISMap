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
    setMinimumSize(800, 600);
    
    // Initialize configuration-based settings
    initializeFromConfig();
    
    // Initialize new architecture
    initializeArchitecture();
    
    // Initialize network manager for async tile loading
    m_networkManager = new QNetworkAccessManager(this);
    
    loadTileMap();
    
    // Create a sample polygon around Hoan Kiem Lake area in Hanoi
    QPolygonF hanoiPoly;
    hanoiPoly << QPointF(105.850, 21.025)   // Southwest corner
              << QPointF(105.850, 21.035)   // Northwest corner  
              << QPointF(105.860, 21.035)   // Northeast corner
              << QPointF(105.860, 21.025)   // Southeast corner
              << QPointF(105.850, 21.025);  // Close polygon
    setPolygon(hanoiPoly);
    
    fetchShapefile();
    fetchPostgis();
    
    // Create sample aircraft
    createSampleAircraft();
    
    // Emit initial coordinates
    emit coordinatesChanged(m_centerGeo.x(), m_centerGeo.y(), m_zoom);
}

void MapWidget::setPolygon(const QPolygonF &polygon) {
    m_polygon = polygon;
    
    // Update the polygon object in new architecture
    if (m_hanoiPolygon) {
        m_hanoiPolygon->setPolygon(polygon);
    }
    
    update();
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
    
    // Draw all polygons
    drawGeoPolygon(m_polygon, Qt::red);
    for (const auto &poly : m_shapefilePolygons) drawGeoPolygon(poly, Qt::blue);
    for (const auto &poly : m_postgisPolygons) drawGeoPolygon(poly, Qt::green);
    
    // Render aircraft layer using new architecture
    if (m_aircraftLayer && m_viewTransform) {
        updateViewTransform();
        m_aircraftLayer->render(painter, *m_viewTransform);
    }
}

void MapWidget::resizeEvent(QResizeEvent *) {
    loadTileMap();
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
        
        // Legacy behavior: recenter map on click
        QPointF pixelCenter = QPointF(width() / 2.0, height() / 2.0);
        QPointF pixelClick = event->pos();
        QPointF pixelOffset = pixelClick - pixelCenter;
        
        // Convert pixel offset to geographic offset using config tile size
        double scale = m_tileSize * (1 << m_zoom);
        double lon = m_centerGeo.x() + (pixelOffset.x() * 360.0) / scale;
        double lat_rad = atan(sinh((m_centerGeo.y() * M_PI / 180.0) + (pixelOffset.y() * 2.0 * M_PI) / scale));
        double lat = lat_rad * 180.0 / M_PI;
        
        // Recenter map on click position
        m_centerGeo = QPointF(lon, lat);
        loadTileMap();
        emit coordinatesChanged(m_centerGeo.x(), m_centerGeo.y(), m_zoom);
    }
    event->accept();
}

void MapWidget::loadTileMap() {
    // Calculate center tile coordinates
    int newCenterTileX = (int)((m_centerGeo.x() + 180.0) / 360.0 * (1 << m_zoom));
    int newCenterTileY = (int)((1.0 - log(tan(m_centerGeo.y() * M_PI / 180.0) + 1.0 / cos(m_centerGeo.y() * M_PI / 180.0)) / M_PI) / 2.0 * (1 << m_zoom));
    
    // Only reload if center has changed significantly
    if (abs(newCenterTileX - m_centerTileX) < 2 && abs(newCenterTileY - m_centerTileY) < 2 && !m_tiles.isEmpty()) {
        return; // Don't reload if we haven't moved much
    }
    
    m_centerTileX = newCenterTileX;
    m_centerTileY = newCenterTileY;
    
    qDebug() << "Center tile coordinates:" << m_centerTileX << m_centerTileY << "at zoom" << m_zoom;
    
    // Calculate how many tiles we need to cover the widget (using config tile size)
    int tilesX = qMin(4, (width() / m_tileSize) + 2);  // Limit to 4 tiles max
    int tilesY = qMin(4, (height() / m_tileSize) + 2);
    
    m_tiles.clear();
    m_tilePositions.clear();
    
    QNetworkAccessManager manager;
    
    // Get current tile server URL template
    QString urlTemplate = getCurrentTileServerUrl();
    
    // Load tiles in a grid around the center
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
                continue; // Skip network download
            }
            
            // If not in cache, download from server
            QString url = urlTemplate;
            url.replace("{z}", QString::number(m_zoom));
            url.replace("{x}", QString::number(tileX));
            url.replace("{y}", QString::number(tileY));
            
            qDebug() << "Downloading tile:" << url;
            
            QNetworkRequest request{QUrl(url)};
            // Add proper headers to avoid being blocked
            request.setHeader(QNetworkRequest::UserAgentHeader, "GISMap/1.0 (Qt Application)");
            request.setRawHeader("Referer", "https://www.openstreetmap.org/");
            
            QEventLoop loop;
            QNetworkReply *reply = manager.get(request);
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            
            // Add timeout to prevent hanging
            QTimer::singleShot(10000, &loop, &QEventLoop::quit);
            loop.exec();
            
            if (reply->error() == QNetworkReply::NoError) {
                tile.loadFromData(reply->readAll());
                if (!tile.isNull()) {
                    // Save to cache
                    saveTileToCache(m_zoom, tileX, tileY, m_activeTileServer, tile);
                    
                    m_tiles.append(tile);
                    m_tilePositions.append(QPoint(dx, dy));
                    qDebug() << "Successfully downloaded and cached tile at offset" << dx << dy;
                } else {
                    // Create fallback tile if data is invalid
                    tile = createFallbackTile(tileX, tileY);
                    m_tiles.append(tile);
                    m_tilePositions.append(QPoint(dx, dy));
                    qDebug() << "Created fallback tile for invalid data at offset" << dx << dy;
                }
            } else {
                qDebug() << "Failed to download tile:" << url << reply->errorString();
                // Create fallback tile when network fails
                tile = createFallbackTile(tileX, tileY);
                m_tiles.append(tile);
                m_tilePositions.append(QPoint(dx, dy));
                qDebug() << "Created fallback tile due to network error at offset" << dx << dy;
            }
            reply->deleteLater();
        }
    }
    qDebug() << "Total tiles loaded:" << m_tiles.size();
    update();
    
    // Trigger tile prefetching for smoother navigation
    QTimer::singleShot(1000, this, [this]() { prefetchTiles(2); });
}

void MapWidget::drawTiles(QPainter &painter) {
    if (m_tiles.isEmpty()) return;
    
    // Calculate center pixel position
    QPointF centerPixel = geoToPixel(m_centerGeo.x(), m_centerGeo.y(), m_zoom);
    
    // Draw each tile at its correct position
    for (int i = 0; i < m_tiles.size(); ++i) {
        QPoint tileOffset = m_tilePositions[i];
        
        // Calculate tile position in widget coordinates using config tile size
        int x = width() / 2 + tileOffset.x() * m_tileSize - m_tileSize/2;
        int y = height() / 2 + tileOffset.y() * m_tileSize - m_tileSize/2;
        
        QRect tileRect(x, y, m_tileSize, m_tileSize);
        painter.drawPixmap(tileRect, m_tiles[i]);
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

void MapWidget::fetchShapefile() {
    // Load shapefiles using configuration settings
    GDALAllRegister();
    
    ConfigManager& config = ConfigManager::instance();
    
    // Try to load each configured shapefile
    QStringList shapefilePaths = {
        "resources/shapefiles/vn.shp",
        "vn.shp"  // Fallback to current directory
    };
    
    for (const QString& path : shapefilePaths) {
        GDALDataset *poDS = (GDALDataset*) GDALOpenEx(path.toLocal8Bit().data(), GDAL_OF_VECTOR, NULL, NULL, NULL);
        if (poDS) {
            qDebug() << "Successfully opened shapefile:" << path;
            
            OGRLayer *poLayer = poDS->GetLayer(0);
            OGRFeature *poFeature;
            poLayer->ResetReading();
            
            int featureCount = 0;
            while ((poFeature = poLayer->GetNextFeature()) != nullptr && featureCount < 1000) {
                OGRGeometry *poGeometry = poFeature->GetGeometryRef();
                if (poGeometry && wkbFlatten(poGeometry->getGeometryType()) == wkbPolygon) {
                    OGRPolygon *poPolygon = (OGRPolygon *) poGeometry;
                    OGRLinearRing *ring = poPolygon->getExteriorRing();
                    QPolygonF qpoly;
                    for (int i = 0; i < ring->getNumPoints(); ++i) {
                        qpoly << QPointF(ring->getX(i), ring->getY(i));
                    }
                    m_shapefilePolygons.append(qpoly);
                    featureCount++;
                }
                OGRFeature::DestroyFeature(poFeature);
            }
            
            qDebug() << "Loaded" << featureCount << "features from shapefile";
            GDALClose(poDS);
            break; // Successfully loaded, don't try other paths
        } else {
            qDebug() << "Failed to open shapefile:" << path;
        }
    }
    
    if (m_shapefilePolygons.isEmpty()) {
        qDebug() << "No shapefiles loaded successfully";
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
        
    } catch (const std::exception &e) {
        qDebug() << "PostGIS error:" << e.what();
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
    QVector<QPointF> startPositions = {
        QPointF(105.840, 21.020),  // Southwest of Hoan Kiem Lake
        QPointF(105.855, 21.015),  // Southeast of Hoan Kiem Lake  
        QPointF(105.845, 21.040),  // Northwest of Hoan Kiem Lake
        QPointF(105.865, 21.030),  // Northeast of Hoan Kiem Lake
        QPointF(105.850, 21.025)   // Near center (should be in region)
    };
    
    for (const QPointF& position : startPositions) {
        Aircraft* aircraft = m_aircraftManager->createAircraft(position);
        if (aircraft) {
            // Set random velocity for movement
            double velocityMag = 0.0001; // Small velocity for slow movement
            double angle = (rand() % 360) * M_PI / 180.0;
            aircraft->setVelocity(QPointF(velocityMag * cos(angle), velocityMag * sin(angle)));
            aircraft->setHeading(angle * 180.0 / M_PI);
            
            // Start movement
            aircraft->startMovement();
            
            qDebug() << "Created aircraft at position:" << position;
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
