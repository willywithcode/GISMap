#include "data/tileloader.h"
#include "core/config.h"
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QBuffer>

namespace GISMap {
namespace Data {

TileLoader::TileLoader(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_tileCache(100) 
{
    m_tileUrlTemplate = Core::Config::getInstance().getTileServerUrl();
    
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &TileLoader::handleTileDownloaded);
}

TileLoader::~TileLoader()
{
}

void TileLoader::requestTile(int x, int y, int zoom)
{
    QString key = tileKey(x, y, zoom);
    
    {
        QMutexLocker locker(&m_cacheMutex);
        if (m_tileCache.contains(key)) {
            emit tileLoaded(*m_tileCache.object(key), x, y, zoom);
            return;
        }
    }
    
    QNetworkRequest request(tileUrl(x, y, zoom));
    
    request.setHeader(QNetworkRequest::UserAgentHeader, 
                     "GISMap/1.0 (https://example.com; your@email.com)");
    
    request.setAttribute(QNetworkRequest::User + 1, x);
    request.setAttribute(QNetworkRequest::User + 2, y);
    request.setAttribute(QNetworkRequest::User + 3, zoom);
    
    m_networkManager->get(request);
}

QImage TileLoader::getTile(int x, int y, int zoom) const
{
    QMutexLocker locker(&m_cacheMutex);
    QString key = tileKey(x, y, zoom);
    
    if (m_tileCache.contains(key)) {
        return *m_tileCache.object(key);
    }
    
    return QImage(); 
}

void TileLoader::clearCache()
{
    QMutexLocker locker(&m_cacheMutex);
    m_tileCache.clear();
}

void TileLoader::setTileUrlTemplate(const QString &urlTemplate)
{
    m_tileUrlTemplate = urlTemplate;
}

void TileLoader::setCacheSize(int size)
{
    QMutexLocker locker(&m_cacheMutex);
    m_tileCache.setMaxCost(size);
}

QString TileLoader::tileKey(int x, int y, int zoom) const
{
    return QString("%1/%2/%3").arg(zoom).arg(x).arg(y);
}

QUrl TileLoader::tileUrl(int x, int y, int zoom) const
{
    QString url = m_tileUrlTemplate;
    url.replace("{x}", QString::number(x));
    url.replace("{y}", QString::number(y));
    url.replace("{z}", QString::number(zoom));
    
    return QUrl(url);
}

void TileLoader::handleTileDownloaded(QNetworkReply *reply)
{
    int x = reply->request().attribute(QNetworkRequest::User + 1).toInt();
    int y = reply->request().attribute(QNetworkRequest::User + 2).toInt();
    int zoom = reply->request().attribute(QNetworkRequest::User + 3).toInt();
    
    if (reply->error() != QNetworkReply::NoError) {
        emit tileLoadError(x, y, zoom, reply->errorString());
        reply->deleteLater();
        return;
    }
    
    QByteArray imageData = reply->readAll();
    QImage image;
    
    if (!image.loadFromData(imageData)) {
        emit tileLoadError(x, y, zoom, "Invalid image data");
        reply->deleteLater();
        return;
    }
    
    QString key = tileKey(x, y, zoom);
    {
        QMutexLocker locker(&m_cacheMutex);
        m_tileCache.insert(key, new QImage(image), 1);
    }
    
    emit tileLoaded(image, x, y, zoom);
    
    reply->deleteLater();
}

} 
} 