#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QVariant>
#include <QMap>

namespace GISMap {
namespace Core {

/**
 * Singleton configuration class to manage application settings
 */
class Config
{
public:
    /**
     * Get the singleton instance
     * @return Reference to the Config instance
     */
    static Config& getInstance();
    
    /**
     * Load configuration from file
     * @param filename Path to the configuration file
     * @return True if loaded successfully
     */
    bool loadFromFile(const QString& filename);
    
    /**
     * Save configuration to file
     * @param filename Path to the configuration file
     * @return True if saved successfully
     */
    bool saveToFile(const QString& filename);
    
    /**
     * Get a configuration value
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return Configuration value
     */
    QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    
    /**
     * Set a configuration value
     * @param key Configuration key
     * @param value Value to set
     */
    void setValue(const QString& key, const QVariant& value);
    
    // Default configuration getters
    QString getTileServerUrl() const;
    QString getDatabaseHost() const;
    QString getDatabaseName() const;
    QString getDatabaseUser() const;
    QString getDatabasePassword() const;
    int getDatabasePort() const;
    QString getShapefilePath() const;

private:
    Config(); // Private constructor for singleton
    Config(const Config&) = delete; // No copy constructor
    Config& operator=(const Config&) = delete; // No assignment operator
    
    void setDefaults();
    
    QMap<QString, QVariant> m_configValues;
};

} // namespace Core
} // namespace GISMap

#endif // CONFIG_H 