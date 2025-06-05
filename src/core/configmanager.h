#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QPointF>
#include <QColor>
#include <QPolygonF>
#include <QVector>
#include <QString>

/**
 * @brief Manages application configuration from JSON files
 */
class ConfigManager : public QObject {
    Q_OBJECT

public:
    static ConfigManager& instance();
    
    // Initialization
    bool loadConfigs(const QString& configDir = "config");
    
    // Database configuration
    QString getDatabaseHost() const;
    int getDatabasePort() const;
    QString getDatabaseName() const;
    QString getDatabaseUser() const;
    QString getDatabasePassword() const;
    int getDatabaseTimeout() const;
    
    // Database table configuration
    QString getDatabasePolygonsTableName() const;
    QString getDatabasePolygonsGeometryColumn() const;
    int getDatabasePolygonsLimit() const;
    QString getDatabaseUsername() const;  // Alias for getDatabaseUser
    int getDatabaseConnectionTimeout() const; // Alias for getDatabaseTimeout
    
    // Map configuration
    QPointF getDefaultMapCenter() const;
    int getDefaultZoom() const;
    int getMinZoom() const;
    int getMaxZoom() const;
    int getTileSize() const;
    QString getTileServerUrl() const;
    bool isTileCacheEnabled() const;
    QString getTileCacheDirectory() const;
    int getMaxCacheSizeMB() const;
    
    // Aircraft configuration
    double getDefaultAircraftSpeed() const;
    int getAircraftUpdateInterval() const;
    int getAircraftIconSize() const;
    int getAircraftSelectionRadius() const;
    int getMaxAircraftCount() const;
    bool isBoundaryBounceEnabled() const;
    QColor getAircraftColor(const QString& state) const;
    QRectF getMovementBoundary() const;
    QPolygonF getHanoiRegion() const;
    
    // Application configuration
    QString getApplicationName() const;
    QString getApplicationVersion() const;
    QSize getDefaultWindowSize() const;
    QSize getMinimumWindowSize() const;
    QString getWindowTitle() const;
    bool isLoggingEnabled() const;
    QString getLogLevel() const;
    
    // Data sources configuration
    QVector<QJsonObject> getShapefileConfigs() const;
    QVector<QJsonObject> getPostgisLayerConfigs() const;
    double getPolygonOpacity() const;
    int getBorderWidth() const;
    bool isAntialiasingEnabled() const;

signals:
    void configurationChanged();

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    bool loadJsonFile(const QString& filePath, QJsonObject& result);
    
    QJsonObject m_databaseConfig;
    QJsonObject m_mapConfig;
    QJsonObject m_aircraftConfig;
    QJsonObject m_applicationConfig;
    QJsonObject m_dataSourcesConfig;
    
    bool m_configsLoaded = false;
};
