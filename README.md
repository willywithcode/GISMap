# GIS Map Application

Ứng dụng hiển thị và tương tác với dữ liệu GIS được xây dựng bằng C++ và Qt.

## Tính năng

- Hiển thị bản đồ từ OpenStreetMap tiles
- Hiển thị dữ liệu shapefile và dữ liệu từ PostgreSQL/PostGIS
- Vẽ khu vực đa giác tĩnh (khu vực Hà Nội)
- Hiển thị đối tượng động (máy bay) di chuyển
- Tương tác với đối tượng máy bay (click để hiển thị thông tin)
- Đổi màu máy bay khi vào vùng đặc biệt

## Yêu cầu hệ thống

- Linux hoặc WSL (Windows Subsystem for Linux)
- C++ Compiler (GCC/G++ ≥ 7.0)
- Qt5 (≥ 5.12)
- GDAL library
- PostgreSQL với extension PostGIS

## Cài đặt trên WSL

### 1. Cài đặt môi trường phát triển

```bash
# Cập nhật hệ thống
sudo apt update
sudo apt upgrade -y

# Cài đặt các package cần thiết
sudo apt install -y build-essential cmake git
sudo apt install -y qtbase5-dev qt5-default libqt5network5 qttools5-dev-tools
sudo apt install -y libgdal-dev libpq-dev postgresql postgresql-contrib postgresql-client
sudo apt install -y postgis
```

### 2. Cài đặt PostgreSQL và PostGIS

```bash
# Khởi động PostgreSQL
sudo service postgresql start

# Tạo user và database
sudo -u postgres createuser --superuser $USER
sudo -u postgres createdb gismap

# Kích hoạt PostGIS extension
sudo -u postgres psql -c "CREATE EXTENSION postgis;" gismap
```

### 3. Tải dữ liệu Shapefile

```bash
# Tạo thư mục cho dữ liệu
mkdir -p data/shapefiles
cd data/shapefiles

# Tải shapefile Việt Nam
wget https://simplemaps.com/static/data/country-cities/vn/vn.zip
unzip vn.zip
```

## Biên dịch và chạy

```bash
# Clone project
git clone <repository-url> gismap
cd gismap

# Tạo thư mục build
mkdir build
cd build

# Cấu hình và biên dịch
cmake ..
make -j4

# Chạy ứng dụng
./GISMap
```

## Cấu trúc project

- `src/`: Chứa mã nguồn C++
- `include/`: Chứa các file header
- `resources/`: Chứa tài nguyên (icons, images)
- `data/`: Chứa dữ liệu GIS (shapefiles, etc.)
- `CMakeLists.txt`: File cấu hình CMake

## Tài liệu tham khảo

- [Qt Documentation](https://doc.qt.io/)
- [QGIS GitHub Repository](https://github.com/qgis/QGIS)
- [PostGIS Documentation](https://postgis.net/docs/)
- [OpenStreetMap Tile Usage Policy](https://operations.osmfoundation.org/policies/tiles/) 