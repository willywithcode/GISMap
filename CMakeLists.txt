cmake_minimum_required(VERSION 3.10)
project(GISMap VERSION 1.0)

# Thiết lập chuẩn C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Cấu hình Qt
find_package(Qt5 COMPONENTS Core Widgets Network REQUIRED)

# Thêm thư viện PostGIS và GDAL
find_package(GDAL REQUIRED)
find_package(PostgreSQL REQUIRED)

# Thêm include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${PostgreSQL_INCLUDE_DIRS}
    ${GDAL_INCLUDE_DIRS}
)

# Thu thập các file nguồn
file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

# Tạo target
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Liên kết với các thư viện
target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
    ${PostgreSQL_LIBRARIES}
    ${GDAL_LIBRARIES}
)

# Sao chép tài nguyên vào thư mục build
file(COPY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR}) 