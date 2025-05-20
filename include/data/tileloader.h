#ifndef TILELOADER_H
#define TILELOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QImage>
#include <QCache>
#include <QMutex>
#include <QUrl>
#include "core/types.h"

namespace GISMap {
namespace Data {

/**
 * Class responsible for loading map tiles from OpenStreetMap tile server
 * and handling the tile cache
 */
class TileLoader : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor with parent object for memory management
     */
    explicit TileLoader(QObject *parent = nullptr);
    
    /**
     * Destructor
     */
    ~TileLoader();
    
    /**
     * Request a map tile asynchronously
     * @param x Tile x coordinate
     * @param y Tile y coordinate
     * @param zoom Zoom level
     */
    void requestTile(int x, int y, int zoom);
    
    /**
     * Get a tile if it's already in the cache
     * @param x Tile x coordinate
     * @param y Tile y coordinate
     * @param zoom Zoom level
     * @return Tile image if found, empty image otherwise
     */
    QImage getTile(int x, int y, int zoom) const;
    
    /**
     * Clear the tile cache
     */
    void clearCache();
    
    /**
     * Set the tile server URL template
     * @param urlTemplate URL template with {z}, {x}, {y} placeholders
     */
    void setTileUrlTemplate(const QString &urlTemplate);
    
    /**
     * Set the cache size in items
     * @param size Maximum number of tiles to cache
     */
    void setCacheSize(int size);

signals:
    /**
     * Signal emitted when a tile is successfully loaded
     * @param tile Loaded tile image
     * @param x Tile x coordinate
     * @param y Tile y coordinate
     * @param zoom Zoom level
     */
    void tileLoaded(const QImage &tile, int x, int y, int zoom);
    
    /**
     * Signal emitted when a tile loading error occurs
     * @param x Tile x coordinate
     * @param y Tile y coordinate
     * @param zoom Zoom level
     * @param error Error message
     */
    void tileLoadError(int x, int y, int zoom, const QString &error);

private slots:
    /**
     * Handle tile download completion
     * @param reply Network reply containing the downloaded tile
     */
    void handleTileDownloaded(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_networkManager;
    QCache<QString, QImage> m_tileCache;
    QMutex m_cacheMutex;
    QString m_tileUrlTemplate;
    
    /**
     * Generate a unique key for a tile
     * @param x Tile x coordinate
     * @param y Tile y coordinate
     * @param zoom Zoom level
     * @return Unique string key
     */
    QString tileKey(int x, int y, int zoom) const;
    
    /**
     * Generate a URL for a tile
     * @param x Tile x coordinate
     * @param y Tile y coordinate
     * @param zoom Zoom level
     * @return URL for the tile
     */
    QUrl tileUrl(int x, int y, int zoom) const;
};

} // namespace Data
} // namespace GISMap

#endif // TILELOADER_H 