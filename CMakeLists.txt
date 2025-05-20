cmake_minimum_required(VERSION 3.16)
project(GISMap VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required packages
find_package(Qt6 COMPONENTS Core Widgets Network REQUIRED)
find_package(GDAL REQUIRED)
find_package(PostgreSQL REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GDAL_INCLUDE_DIRS}
    ${PostgreSQL_INCLUDE_DIRS}
)

# Add source files by component
set(CORE_SOURCES
    src/core/config.cpp
    src/core/geoutil.cpp
)

set(DATA_SOURCES
    src/data/tileloader.cpp
    src/data/database.cpp
    src/data/shapefileloader.cpp
)

set(MODEL_SOURCES
    src/model/aircraft.cpp
    src/model/polygon.cpp
    src/model/mapfeature.cpp
)

set(VIEW_SOURCES
    src/view/mainwindow.cpp
    src/view/mapwidget.cpp
    src/view/statusbar.cpp
)

set(CONTROLLER_SOURCES
    src/controller/mapcontroller.cpp
    src/controller/aircraftcontroller.cpp
)

# Combine all sources
set(SOURCES
    ${CORE_SOURCES}
    ${DATA_SOURCES}
    ${MODEL_SOURCES}
    ${VIEW_SOURCES}
    ${CONTROLLER_SOURCES}
    src/main.cpp
)

# Add resource files
set(RESOURCES
    resources/resources.qrc
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${RESOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    ${GDAL_LIBRARIES}
    ${PostgreSQL_LIBRARIES}
)

# Install
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Copy resources to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR})

# Add tests if requested
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif() 