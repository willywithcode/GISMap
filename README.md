# GIS Map Application - Hanoi, Vietnam

Ứng dụng GIS hiển thị bản đồ Hà Nội với các tính năng tương tác và hiển thị đối tượng động.

## ✅ Tính năng đã hoàn thành

### 🗺️ Hiển thị bản đồ
- **Tile map service**: OpenStreetMap và Satellite imagery
- **Zoom limits**: Giới hạn zoom 10-18 phù hợp cho khu vực Hà Nội
- **Pan bounds**: Giới hạn di chuyển trong khu vực hợp lý (105-107°E, 20.5-21.5°N)
- **Tile caching**: Cache tiles locally để tối ưu performance
- **Async loading**: Load tiles bất đồng bộ để tránh lag

### 🛩️ Đối tượng động (Aircraft)
- **Real-time movement**: Máy bay di chuyển mượt mà với update interval 500ms
- **State detection**: Tự động đổi màu khi vào/ra khỏi khu vực Hà Nội
- **Multiple aircraft**: 7 máy bay với trajectory khác nhau
- **Smooth animation**: Update timer 100ms cho animation mượt mà

### 🔴 Đối tượng tĩnh
- **Hanoi region**: Khu vực đa giác bao quanh Hà Nội
- **Shapefile support**: Tải shapefile từ SimpleMaps (Vietnam data)
- **PostGIS integration**: Kết nối PostgreSQL/PostGIS để load polygon data

### 🖱️ Tương tác người dùng
- **Click to select**: Click chuột trái để chọn máy bay
- **Drag to pan**: Kéo thả để di chuyển bản đồ
- **Real-time coordinates**: Hiển thị tọa độ máy bay real-time ở status bar
- **Aircraft highlighting**: Máy bay được chọn sẽ được highlight màu vàng
- **State colors**: 
  - 🔵 Xanh: Máy bay bình thường (ngoài khu vực)
  - 🔴 Đỏ: Máy bay trong khu vực Hà Nội  
  - 🟡 Vàng: Máy bay được chọn

### ⚡ Performance Optimizations
- **Tile caching**: Cache size 200MB với auto cleanup
- **Async tile loading**: Không block UI khi load tiles
- **Optimized tile grid**: Giới hạn 3x3 tiles để tối ưu memory
- **Smart reload**: Chỉ reload tiles khi cần thiết
- **Prefetching**: Prefetch tiles xung quanh để navigation mượt mà

## Yêu cầu hệ thống

### Dependencies cơ bản
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    qt5-default \
    qtbase5-dev \
    qttools5-dev \
    libgdal-dev \
    gdal-bin \
    libpqxx-dev \
    postgresql-client \
    libqt5network5-dev

# Optional: QGIS development libraries
sudo apt install -y \
    qgis-dev \
    libqgis-dev
```

### PostgreSQL/PostGIS Setup
```bash
# Cài đặt PostgreSQL và PostGIS
sudo apt install -y postgresql postgresql-contrib postgis

# Tạo database
sudo -u postgres createdb gisdb
sudo -u postgres psql -d gisdb -c "CREATE EXTENSION postgis;"

# Tạo user (thay đổi password trong config/database.json)
sudo -u postgres psql -c "CREATE USER postgres WITH PASSWORD '88888888';"
sudo -u postgres psql -c "GRANT ALL PRIVILEGES ON DATABASE gisdb TO postgres;"

# Load sample data
sudo -u postgres psql -d gisdb -f setup_database.sql
```

## Build và Run

### Sử dụng build script (Khuyến nghị)
```bash
# Build và check dependencies
./build.sh

# Build và run ngay
./build.sh run
```

### Build thủ công
```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make -j$(nproc)

# Run
./GISMap
```

## Cấu hình

Các file cấu hình trong thư mục `config/`:

### `map.json` - Cấu hình bản đồ
```json
{
  "map": {
    "default_center": {"longitude": 105.8542, "latitude": 21.0285},
    "default_zoom": 13,
    "min_zoom": 10,
    "max_zoom": 18,
    "pan_bounds": {
      "min_longitude": 105.0, "max_longitude": 107.0,
      "min_latitude": 20.5, "max_latitude": 21.5
    }
  }
}
```

### `aircraft.json` - Cấu hình máy bay
```json
{
  "aircraft": {
    "default_speed": 0.0005,
    "update_interval_ms": 500,
    "icon_size": 24,
    "selection_radius": 20
  }
}
```

## Sử dụng

### 🗺️ Điều khiển bản đồ
- **Zoom**: Scroll wheel (giới hạn zoom 10-18)
- **Pan**: Click và drag (giới hạn trong khu vực Hà Nội)
- **Tile servers**: Menu Map > Tile Server (OpenStreetMap/Satellite)
- **Refresh**: Menu Map > Refresh Map hoặc toolbar
- **Clear cache**: Menu Map > Clear Tile Cache

### ✈️ Tương tác với máy bay
- **Chọn máy bay**: Click chuột trái lên máy bay
- **Xem thông tin**: Tọa độ real-time hiển thị ở status bar
- **Deselect**: Click vào vùng trống
- **Aircraft info**: Longitude, Latitude, Heading, State

### 🎨 Màu sắc và trạng thái
- **🔵 Xanh (Normal)**: Máy bay ngoài khu vực Hà Nội
- **🔴 Đỏ (In Region)**: Máy bay trong khu vực Hà Nội
- **🟡 Vàng (Selected)**: Máy bay được chọn (có highlight)

## Cấu trúc dự án

```
src/
├── core/           # Core components
│   ├── viewtransform.cpp/h     # Coordinate transformations
│   ├── configmanager.cpp/h     # Configuration management
│   └── geometryobject.cpp/h    # Base geometry class
├── ui/             # User interface
│   ├── mainwindow.cpp/h        # Main application window
│   └── mapwidget.cpp/h         # Map display widget
├── models/         # Data models
│   ├── aircraft.cpp/h          # Aircraft object with movement
│   └── polygonobject.cpp/h     # Polygon regions
├── layers/         # Map layers
│   ├── aircraftlayer.cpp/h     # Aircraft rendering layer
│   └── maplayer.cpp/h          # Base layer class
├── managers/       # Object managers
│   └── aircraftmanager.cpp/h  # Aircraft lifecycle management
└── main.cpp        # Entry point

config/             # Configuration files
├── map.json        # Map settings
├── aircraft.json   # Aircraft settings  
├── database.json   # PostgreSQL/PostGIS config
├── application.json # App settings
└── data_sources.json # Data source config

resources/          # Resources
├── shapefiles/     # Shapefile data
└── tiles/          # Tile cache

setup_database.sql  # Database setup script
build.sh           # Build script
```

## 🚀 Performance Features

### Tile Management
- **Smart caching**: 200MB cache với automatic cleanup
- **Async loading**: Non-blocking tile downloads
- **Prefetching**: Load surrounding tiles proactively
- **Fallback tiles**: Placeholder tiles khi network fail

### Aircraft Animation
- **Smooth movement**: 500ms update interval
- **Real-time rendering**: 100ms repaint timer
- **Efficient collision detection**: Optimized point-in-polygon tests
- **State management**: Automatic region detection

### Memory Optimization
- **Limited tile grid**: 3x3 tiles maximum
- **Smart reload**: Only reload when necessary
- **Cache size limits**: Automatic cleanup khi vượt giới hạn
- **Efficient data structures**: STL containers và Qt collections

## 🔧 Troubleshooting

### Build Issues
```bash
# Nếu thiếu Qt5
export QT_SELECT=qt5
sudo apt install qt5-default qtbase5-dev

# Nếu thiếu GDAL
sudo apt install libgdal-dev gdal-bin

# Nếu thiếu PostgreSQL
sudo apt install libpqxx-dev postgresql-client
```

### Runtime Issues
```bash
# Kiểm tra PostgreSQL connection
psql -h localhost -U postgres -d gisdb

# Test GDAL
gdalinfo --version

# Test Qt
qmake --version

# Check config files
ls -la config/
```

### Performance Issues
- **Slow tile loading**: Check internet connection và tile cache
- **High memory usage**: Clear tile cache hoặc giảm cache size
- **Laggy aircraft**: Tăng update interval trong aircraft.json

## 📋 TODO / Future Improvements

- [ ] **QGIS Integration**: Full QGIS library integration
- [ ] **Shapefile extraction**: Auto-extract downloaded zip files
- [ ] **More data sources**: WMS, WFS support
- [ ] **Advanced aircraft**: Flight paths, altitude data
- [ ] **User interface**: Better controls, settings dialog
- [ ] **Export features**: Save screenshots, export data

## License

MIT License - xem file LICENSE để biết thêm chi tiết. 