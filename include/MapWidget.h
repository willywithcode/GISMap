#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QWidget>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QPolygonF>
#include <QMouseEvent>
#include <QStatusBar>
#include <vector>

// Cấu trúc dữ liệu máy bay
struct Aircraft {
    QPointF position;     // Vị trí hiện tại (kinh độ, vĩ độ)
    QPointF direction;    // Hướng di chuyển
    double speed;         // Tốc độ
    QColor color;         // Màu sắc
    bool isSelected;      // Trạng thái được chọn
    QString id;           // ID của máy bay
};

class MapWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit MapWidget(QWidget *parent = nullptr);
    ~MapWidget() override;
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    
private slots:
    void updateAircrafts();
    void tileDownloaded(QNetworkReply *reply);
    
private:
    // Hiển thị bản đồ
    QNetworkAccessManager *networkManager;
    QHash<QString, QPixmap> tileCache;
    int zoomLevel;
    QPointF centerPoint;  // Kinh độ, vĩ độ trung tâm
    
    // Khu vực đa giác (Hà Nội)
    QPolygonF hanoiRegion;
    
    // Danh sách máy bay
    std::vector<Aircraft> aircrafts;
    
    // Timer cập nhật vị trí
    QTimer *updateTimer;
    
    // Status bar
    QStatusBar *statusBar;
    
    // Các hàm helper
    void loadMapTiles();
    QPoint latLongToPixel(double lat, double lon, int zoom);
    QPointF pixelToLatLong(int x, int y, int zoom);
    QString tileKey(int x, int y, int z);
    void drawAircraft(QPainter &painter, const Aircraft &aircraft);
    bool isPointInAircraft(const QPoint &point, const Aircraft &aircraft);
    bool isPointInPolygon(const QPointF &point, const QPolygonF &polygon);
    void initHanoiRegion();
    void initAircrafts();
};

#endif // MAPWIDGET_H 