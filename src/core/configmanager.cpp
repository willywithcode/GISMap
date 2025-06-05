#include "configmanager.h"
#include <QFile>
#include <QJsonParseError>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>

ConfigManager& ConfigManager::instance()
{
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadConfigs(const QString& configDir)
{
    QString baseDir = QCoreApplication::applicationDirPath() + "/../" + configDir;
    QDir dir(baseDir);
    
    if (!dir.exists()) {
        qWarning() << "Config directory does not exist:" << baseDir;
        return false;
    }
    
    bool allLoaded = true;
    
    // Load all config files
    allLoaded &= loadJsonFile(dir.filePath("database.json"), m_databaseConfig);
    allLoaded &= loadJsonFile(dir.filePath("map.json"), m_mapConfig);
    allLoaded &= loadJsonFile(dir.filePath("aircraft.json"), m_aircraftConfig);
    allLoaded &= loadJsonFile(dir.filePath("application.json"), m_applicationConfig);
    allLoaded &= loadJsonFile(dir.filePath("data_sources.json"), m_dataSourcesConfig);
    
    if (allLoaded) {
        m_configsLoaded = true;
        emit configurationChanged();
        qDebug() << "All configuration files loaded successfully";
    } else {
        qWarning() << "Failed to load some configuration files";
    }
    
    return allLoaded;
}

bool ConfigManager::loadJsonFile(const QString& filePath, QJsonObject& result)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open config file:" << filePath;
        return false;
    }
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error in" << filePath << ":" << parseError.errorString();
        return false;
    }
    
    result = doc.object();
    qDebug() << "Loaded config file:" << filePath;
    return true;
}

// Database configuration
QString ConfigManager::getDatabaseHost() const
{
    return m_databaseConfig["postgis"]["host"].toString("localhost");
}

int ConfigManager::getDatabasePort() const
{
    return m_databaseConfig["postgis"]["port"].toInt(5432);
}

QString ConfigManager::getDatabaseName() const
{
    return m_databaseConfig["postgis"]["database"].toString("gisdb");
}

QString ConfigManager::getDatabaseUser() const
{
    return m_databaseConfig["postgis"]["username"].toString("postgres");
}

QString ConfigManager::getDatabasePassword() const
{
    return m_databaseConfig["postgis"]["password"].toString();
}

int ConfigManager::getDatabaseTimeout() const
{
    return m_databaseConfig["postgis"]["connection_timeout"].toInt(30);
}

// Database table configuration
QString ConfigManager::getDatabasePolygonsTableName() const
{
    return m_databaseConfig["tables"]["polygons"]["table_name"].toString("polygons");
}

QString ConfigManager::getDatabasePolygonsGeometryColumn() const
{
    return m_databaseConfig["tables"]["polygons"]["geometry_column"].toString("geom");
}

int ConfigManager::getDatabasePolygonsLimit() const
{
    return m_databaseConfig["tables"]["polygons"]["limit"].toInt(1000);
}

QString ConfigManager::getDatabaseUsername() const
{
    return getDatabaseUser(); // Alias
}

int ConfigManager::getDatabaseConnectionTimeout() const
{
    return getDatabaseTimeout(); // Alias
}

// Map configuration
QPointF ConfigManager::getDefaultMapCenter() const
{
    auto center = m_mapConfig["map"]["default_center"].toObject();
    return QPointF(
        center["longitude"].toDouble(105.85),
        center["latitude"].toDouble(21.03)
    );
}

int ConfigManager::getDefaultZoom() const
{
    return m_mapConfig["map"]["default_zoom"].toInt(12);
}

int ConfigManager::getMinZoom() const
{
    return m_mapConfig["map"]["min_zoom"].toInt(3);
}

int ConfigManager::getMaxZoom() const
{
    return m_mapConfig["map"]["max_zoom"].toInt(18);
}

int ConfigManager::getTileSize() const
{
    return m_mapConfig["map"]["tile_size"].toInt(256);
}

QString ConfigManager::getTileServerUrl() const
{
    return m_mapConfig["tile_servers"]["openstreetmap"]["url"].toString(
        "https://tile.openstreetmap.org/{z}/{x}/{y}.png");
}

bool ConfigManager::isTileCacheEnabled() const
{
    return m_mapConfig["cache"]["enabled"].toBool(true);
}

QString ConfigManager::getTileCacheDirectory() const
{
    return m_mapConfig["cache"]["cache_directory"].toString("resources/tiles");
}

int ConfigManager::getMaxCacheSizeMB() const
{
    return m_mapConfig["cache"]["max_size_mb"].toInt(100);
}

// Aircraft configuration
double ConfigManager::getDefaultAircraftSpeed() const
{
    return m_aircraftConfig["aircraft"]["default_speed"].toDouble(0.001);
}

int ConfigManager::getAircraftUpdateInterval() const
{
    return m_aircraftConfig["aircraft"]["update_interval_ms"].toInt(1000);
}

int ConfigManager::getAircraftIconSize() const
{
    return m_aircraftConfig["aircraft"]["icon_size"].toInt(20);
}

int ConfigManager::getAircraftSelectionRadius() const
{
    return m_aircraftConfig["aircraft"]["selection_radius"].toInt(15);
}

int ConfigManager::getMaxAircraftCount() const
{
    return m_aircraftConfig["aircraft"]["max_aircraft"].toInt(100);
}

bool ConfigManager::isBoundaryBounceEnabled() const
{
    return m_aircraftConfig["aircraft"]["boundary_bounce"].toBool(true);
}

QColor ConfigManager::getAircraftColor(const QString& state) const
{
    auto colors = m_aircraftConfig["colors"].toObject();
    QString colorStr = colors[state + "_state"].toString("#0066CC");
    return QColor(colorStr);
}

QRectF ConfigManager::getMovementBoundary() const
{
    auto boundary = m_aircraftConfig["movement"]["boundary"].toObject();
    return QRectF(
        boundary["min_longitude"].toDouble(105.0),
        boundary["min_latitude"].toDouble(20.0),
        boundary["max_longitude"].toDouble(107.0) - boundary["min_longitude"].toDouble(105.0),
        boundary["max_latitude"].toDouble(22.0) - boundary["min_latitude"].toDouble(20.0)
    );
}

QPolygonF ConfigManager::getHanoiRegion() const
{
    QPolygonF polygon;
    auto regions = m_aircraftConfig["regions"].toObject();
    auto hanoi = regions["hanoi"].toObject();
    auto points = hanoi["polygon"].toArray();
    
    for (const auto& point : points) {
        auto coords = point.toArray();
        if (coords.size() >= 2) {
            polygon << QPointF(coords[0].toDouble(), coords[1].toDouble());
        }
    }
    
    return polygon;
}

// Application configuration
QString ConfigManager::getApplicationName() const
{
    return m_applicationConfig["application"]["name"].toString("GIS Map Application");
}

QString ConfigManager::getApplicationVersion() const
{
    return m_applicationConfig["application"]["version"].toString("1.0.0");
}

QSize ConfigManager::getDefaultWindowSize() const
{
    auto window = m_applicationConfig["application"]["window"].toObject();
    return QSize(
        window["width"].toInt(1200),
        window["height"].toInt(800)
    );
}

QSize ConfigManager::getMinimumWindowSize() const
{
    auto window = m_applicationConfig["application"]["window"].toObject();
    return QSize(
        window["minimum_width"].toInt(800),
        window["minimum_height"].toInt(600)
    );
}

QString ConfigManager::getWindowTitle() const
{
    auto window = m_applicationConfig["application"]["window"].toObject();
    return window["title"].toString("GIS Map Application");
}

bool ConfigManager::isLoggingEnabled() const
{
    return m_applicationConfig["logging"]["file_enabled"].toBool(true);
}

QString ConfigManager::getLogLevel() const
{
    return m_applicationConfig["logging"]["level"].toString("INFO");
}

// Data sources configuration
QVector<QJsonObject> ConfigManager::getShapefileConfigs() const
{
    QVector<QJsonObject> configs;
    auto shapefiles = m_dataSourcesConfig["data_sources"]["shapefiles"].toArray();
    
    for (const auto& item : shapefiles) {
        configs.append(item.toObject());
    }
    
    return configs;
}

QVector<QJsonObject> ConfigManager::getPostgisLayerConfigs() const
{
    QVector<QJsonObject> configs;
    auto layers = m_dataSourcesConfig["data_sources"]["postgis_layers"].toArray();
    
    for (const auto& item : layers) {
        configs.append(item.toObject());
    }
    
    return configs;
}

double ConfigManager::getPolygonOpacity() const
{
    return m_dataSourcesConfig["rendering"]["polygon_opacity"].toDouble(0.3);
}

int ConfigManager::getBorderWidth() const
{
    return m_dataSourcesConfig["rendering"]["border_width"].toInt(2);
}

bool ConfigManager::isAntialiasingEnabled() const
{
    return m_dataSourcesConfig["rendering"]["antialiasing"].toBool(true);
}
