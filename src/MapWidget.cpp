#include "MapWidget.h"
#include <QPainter>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <cmath>
#include <QMainWindow>

// Hàm dựng
MapWidget::MapWidget(QWidget *parent) : QWidget(parent) {
    // Khởi tạo mạng
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MapWidget::tileDownloaded);
    
    // Thiết lập ban đầu cho bản đồ
    zoomLevel = 6;
    centerPoint = QPointF(105.8342, 21.0278); // Hà Nội
    
    // Khởi tạo khu vực Hà Nội
    initHanoiRegion();
    
    // Khởi tạo máy bay ở Vịnh Bắc Bộ
    initAircrafts();
    
    // Khởi tạo timer cập nhật vị trí máy bay
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MapWidget::updateAircrafts);
    updateTimer->start(1000); // Cập nhật mỗi giây
    
    // Thiết lập status bar nếu parent là QMainWindow
    if (QMainWindow *mainWindow = qobject_cast<QMainWindow*>(parent)) {
        statusBar = mainWindow->statusBar();
    } else {
        statusBar = nullptr;
    }
    
    // Tải dữ liệu bản đồ
    loadMapTiles();
}

MapWidget::~MapWidget() {
    delete networkManager;
    delete updateTimer;
}

void MapWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Vẽ nền
    painter.fillRect(rect(), Qt::white);
    
    // Vẽ các tile bản đồ
    // TODO: Implement drawing of map tiles from cache
    
    // Vẽ khu vực Hà Nội
    painter.setPen(QPen(Qt::red, 2));
    painter.setBrush(QBrush(QColor(255, 0, 0, 50)));
    // TODO: Convert hanoiRegion from lat/long to pixel coordinates
    
    // Vẽ máy bay
    for (const auto &aircraft : aircrafts) {
        drawAircraft(painter, aircraft);
    }
}

void MapWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        bool foundAircraft = false;
        
        // Kiểm tra xem có click vào máy bay nào không
        for (auto &aircraft : aircrafts) {
            if (isPointInAircraft(event->pos(), aircraft)) {
                aircraft.isSelected = true;
                foundAircraft = true;
                
                // Hiển thị thông tin trong status bar
                if (statusBar) {
                    statusBar->showMessage(QString("Aircraft %1: Latitude %2, Longitude %3")
                                           .arg(aircraft.id)
                                           .arg(aircraft.position.y(), 0, 'f', 6)
                                           .arg(aircraft.position.x(), 0, 'f', 6));
                }
            } else {
                aircraft.isSelected = false;
            }
        }
        
        // Nếu không click vào máy bay nào, xóa thông tin trong status bar
        if (!foundAircraft && statusBar) {
            statusBar->clearMessage();
        }
        
        update(); // Vẽ lại để hiển thị trạng thái đã chọn
    }
}

void MapWidget::resizeEvent(QResizeEvent *event) {
    // Tải lại các tile khi kích thước thay đổi
    loadMapTiles();
}

void MapWidget::updateAircrafts() {
    // Cập nhật vị trí của tất cả máy bay
    for (auto &aircraft : aircrafts) {
        // Di chuyển máy bay theo hướng và tốc độ
        aircraft.position += aircraft.speed * aircraft.direction;
        
        // Kiểm tra xem máy bay có nằm trong khu vực Hà Nội không
        if (isPointInPolygon(aircraft.position, hanoiRegion)) {
            aircraft.color = Qt::red;
        } else {
            aircraft.color = Qt::blue;
        }
    }
    
    // Cập nhật thông tin trong status bar nếu có máy bay được chọn
    if (statusBar) {
        for (const auto &aircraft : aircrafts) {
            if (aircraft.isSelected) {
                statusBar->showMessage(QString("Aircraft %1: Latitude %2, Longitude %3")
                                      .arg(aircraft.id)
                                      .arg(aircraft.position.y(), 0, 'f', 6)
                                      .arg(aircraft.position.x(), 0, 'f', 6));
                break;
            }
        }
    }
    
    // Vẽ lại để hiển thị vị trí mới
    update();
}

void MapWidget::tileDownloaded(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        // Lấy thông tin tile từ URL
        QString url = reply->url().toString();
        QStringList parts = url.split("/");
        QString z = parts[parts.size() - 3];
        QString x = parts[parts.size() - 2];
        QString y = parts[parts.size() - 1].split(".")[0];
        
        // Tạo key cho tile cache
        QString key = tileKey(x.toInt(), y.toInt(), z.toInt());
        
        // Lưu vào cache
        QPixmap pixmap;
        pixmap.loadFromData(reply->readAll());
        tileCache[key] = pixmap;
        
        // Vẽ lại
        update();
    }
    
    reply->deleteLater();
}

void MapWidget::loadMapTiles() {
    // TODO: Calculate visible tiles based on current view and request them
    // Example:
    // QString url = QString("https://tile.openstreetmap.org/%1/%2/%3.png").arg(z).arg(x).arg(y);
    // networkManager->get(QNetworkRequest(QUrl(url)));
}

QPoint MapWidget::latLongToPixel(double lat, double lon, int zoom) {
    // Chuyển đổi từ kinh độ, vĩ độ sang tọa độ pixel
    // Công thức dựa trên Web Mercator projection
    double n = pow(2.0, zoom);
    double x = ((lon + 180.0) / 360.0) * n;
    double y = (1.0 - log(tan(lat * M_PI/180.0) + 1.0 / cos(lat * M_PI/180.0)) / M_PI) / 2.0 * n;
    
    // TODO: Apply offset for current view
    
    return QPoint(x, y);
}

QPointF MapWidget::pixelToLatLong(int x, int y, int zoom) {
    // Chuyển đổi từ tọa độ pixel sang kinh độ, vĩ độ
    // TODO: Apply offset for current view
    
    double n = pow(2.0, zoom);
    double lon_deg = x / n * 360.0 - 180.0;
    double lat_rad = atan(sinh(M_PI * (1 - 2 * y / n)));
    double lat_deg = lat_rad * 180.0 / M_PI;
    
    return QPointF(lon_deg, lat_deg);
}

QString MapWidget::tileKey(int x, int y, int z) {
    return QString("%1/%2/%3").arg(z).arg(x).arg(y);
}

void MapWidget::drawAircraft(QPainter &painter, const Aircraft &aircraft) {
    // Chuyển đổi vị trí kinh độ, vĩ độ sang tọa độ pixel
    QPoint pixelPos = latLongToPixel(aircraft.position.y(), aircraft.position.x(), zoomLevel);
    
    // Lưu trạng thái painter
    painter.save();
    
    // Thiết lập màu sắc dựa trên trạng thái
    painter.setPen(QPen(aircraft.color.darker(), 2));
    
    // Nếu được chọn, vẽ viền và highlight
    if (aircraft.isSelected) {
        painter.setPen(QPen(Qt::black, 3));
        painter.setBrush(aircraft.color.lighter());
    } else {
        painter.setBrush(aircraft.color);
    }
    
    // Tính góc quay dựa trên hướng di chuyển
    double angle = atan2(aircraft.direction.y(), aircraft.direction.x()) * 180.0 / M_PI;
    
    // Di chuyển đến vị trí máy bay
    painter.translate(pixelPos);
    painter.rotate(angle);
    
    // Vẽ hình máy bay (tam giác đơn giản)
    const int size = 15;
    QPolygon plane;
    plane << QPoint(size, 0) << QPoint(-size, -size/2) << QPoint(-size, size/2);
    painter.drawPolygon(plane);
    
    // Khôi phục trạng thái painter
    painter.restore();
}

bool MapWidget::isPointInAircraft(const QPoint &point, const Aircraft &aircraft) {
    // Chuyển đổi vị trí máy bay sang tọa độ pixel
    QPoint pixelPos = latLongToPixel(aircraft.position.y(), aircraft.position.x(), zoomLevel);
    
    // Kiểm tra khoảng cách từ điểm click đến tâm máy bay
    const int hitRadius = 20; // Bán kính vùng hit
    int dx = point.x() - pixelPos.x();
    int dy = point.y() - pixelPos.y();
    
    return (dx*dx + dy*dy) < (hitRadius*hitRadius);
}

bool MapWidget::isPointInPolygon(const QPointF &point, const QPolygonF &polygon) {
    // Sử dụng ray casting algorithm để kiểm tra điểm có nằm trong đa giác không
    bool inside = false;
    int i, j;
    for (i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        if (((polygon[i].y() > point.y()) != (polygon[j].y() > point.y())) &&
            (point.x() < (polygon[j].x() - polygon[i].x()) * (point.y() - polygon[i].y()) / 
             (polygon[j].y() - polygon[i].y()) + polygon[i].x())) {
            inside = !inside;
        }
    }
    
    return inside;
}

void MapWidget::initHanoiRegion() {
    // Khởi tạo đa giác khu vực Hà Nội
    // Các tọa độ xung quanh Hà Nội (kinh độ, vĩ độ)
    hanoiRegion << QPointF(105.7000, 21.1500)  // Tây Bắc
               << QPointF(105.9500, 21.1500)   // Đông Bắc
               << QPointF(105.9500, 20.9500)   // Đông Nam
               << QPointF(105.7000, 20.9500);  // Tây Nam
}

void MapWidget::initAircrafts() {
    // Khởi tạo máy bay ở Vịnh Bắc Bộ
    Aircraft aircraft1;
    aircraft1.position = QPointF(106.5000, 20.5000); // Vịnh Bắc Bộ
    aircraft1.direction = QPointF(-0.01, 0.005);     // Hướng về phía tây bắc
    aircraft1.speed = 1.0;
    aircraft1.color = Qt::blue;
    aircraft1.isSelected = false;
    aircraft1.id = "VN01";
    
    Aircraft aircraft2;
    aircraft2.position = QPointF(106.7000, 20.7000);
    aircraft2.direction = QPointF(-0.008, 0.004);
    aircraft2.speed = 0.8;
    aircraft2.color = Qt::blue;
    aircraft2.isSelected = false;
    aircraft2.id = "VN02";
    
    aircrafts.push_back(aircraft1);
    aircrafts.push_back(aircraft2);
} 