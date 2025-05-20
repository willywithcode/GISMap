#include "core/config.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace GISMap {
namespace Core {

Config& Config::getInstance()
{
    static Config instance;
    return instance;
}

Config::Config()
{
    setDefaults();
}

void Config::setDefaults()
{
    m_configValues["tile.server.url"] = "https://tile.openstreetmap.org/{z}/{x}/{y}.png";
    m_configValues["tile.max.zoom"] = 19;
    m_configValues["tile.min.zoom"] = 1;
    m_configValues["tile.size"] = 256;
    
    m_configValues["db.host"] = "localhost";
    m_configValues["db.port"] = 5432;
    m_configValues["db.name"] = "gismap";
    m_configValues["db.user"] = "postgres";
    m_configValues["db.password"] = "";
    
    m_configValues["shapefile.path"] = "./data/shapefiles/vietnam";
    
    m_configValues["map.initial.lat"] = 21.0278;  
    m_configValues["map.initial.lon"] = 105.8342; 
    m_configValues["map.initial.zoom"] = 10;
    
    m_configValues["aircraft.default.color"] = "#0000FF"; 
    m_configValues["aircraft.warning.color"] = "#FF0000"; 
    m_configValues["aircraft.count"] = 5;                 
    m_configValues["aircraft.update.ms"] = 1000;          
}

bool Config::loadFromFile(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QByteArray jsonData = file.readAll();
    QJsonDocument document = QJsonDocument::fromJson(jsonData);
    
    if (document.isNull() || !document.isObject()) {
        return false;
    }
    
    QJsonObject object = document.object();
    for (auto it = object.begin(); it != object.end(); ++it) {
        m_configValues[it.key()] = it.value().toVariant();
    }
    
    return true;
}

bool Config::saveToFile(const QString& filename)
{
    QJsonObject object;
    
    for (auto it = m_configValues.begin(); it != m_configValues.end(); ++it) {
        object[it.key()] = QJsonValue::fromVariant(it.value());
    }
    
    QJsonDocument document(object);
    QByteArray jsonData = document.toJson(QJsonDocument::Indented);
    
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    file.write(jsonData);
    return true;
}

QVariant Config::getValue(const QString& key, const QVariant& defaultValue) const
{
    return m_configValues.value(key, defaultValue);
}

void Config::setValue(const QString& key, const QVariant& value)
{
    m_configValues[key] = value;
}

QString Config::getTileServerUrl() const
{
    return getValue("tile.server.url").toString();
}

QString Config::getDatabaseHost() const
{
    return getValue("db.host").toString();
}

QString Config::getDatabaseName() const
{
    return getValue("db.name").toString();
}

QString Config::getDatabaseUser() const
{
    return getValue("db.user").toString();
}

QString Config::getDatabasePassword() const
{
    return getValue("db.password").toString();
}

int Config::getDatabasePort() const
{
    return getValue("db.port").toInt();
}

QString Config::getShapefilePath() const
{
    return getValue("shapefile.path").toString();
}

} 
} 