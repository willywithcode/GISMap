cmake_minimum_required(VERSION 3.14)
project(GISMap LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets Network REQUIRED)
find_package(GDAL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

# Source files organized by directory
set(CORE_SOURCES
    src/core/geometryobject.cpp
    src/core/viewtransform.cpp
    src/core/configmanager.cpp
)

set(UI_SOURCES
    src/ui/mainwindow.cpp
    src/ui/mapwidget.cpp
    src/ui/aircraftdialog.cpp
    src/ui/polygoneditor.cpp
)

set(MODELS_SOURCES
    src/models/aircraft.cpp
    src/models/polygonobject.cpp
    src/models/flightroute.cpp
)

set(LAYERS_SOURCES
    src/layers/maplayer.cpp
    src/layers/aircraftlayer.cpp
)

set(MANAGERS_SOURCES
    src/managers/aircraftmanager.cpp
)

set(SERVICES_SOURCES
    src/services/databaseservice.cpp
)

# Header files (for IDE support)
set(CORE_HEADERS
    src/core/geometryobject.h
    src/core/viewtransform.h
    src/core/configmanager.h
)

set(UI_HEADERS
    src/ui/mainwindow.h
    src/ui/mapwidget.h
    src/ui/aircraftdialog.h
    src/ui/polygoneditor.h
)

set(MODELS_HEADERS
    src/models/aircraft.h
    src/models/polygonobject.h
    src/models/flightroute.h
)

set(LAYERS_HEADERS
    src/layers/maplayer.h
    src/layers/aircraftlayer.h
)

set(MANAGERS_HEADERS
    src/managers/aircraftmanager.h
)

set(SERVICES_HEADERS
    src/services/databaseservice.h
)

# UI files
set(UI_FILES
    src/ui/mainwindow.ui
)

add_executable(GISMap
    src/main.cpp
    ${CORE_SOURCES}
    ${UI_SOURCES}
    ${MODELS_SOURCES}
    ${LAYERS_SOURCES}
    ${MANAGERS_SOURCES}
    ${SERVICES_SOURCES}
    ${CORE_HEADERS}
    ${UI_HEADERS}
    ${MODELS_HEADERS}
    ${LAYERS_HEADERS}
    ${MANAGERS_HEADERS}
    ${SERVICES_HEADERS}
    ${UI_FILES}
)

target_link_libraries(GISMap PRIVATE Qt5::Widgets Qt5::Network ${GDAL_LIBRARIES} ${PQXX_LIBRARIES})

# Include directories for the new folder structure
target_include_directories(GISMap PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/layers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/managers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GDAL_INCLUDE_DIRS} 
    ${PQXX_INCLUDE_DIRS}
)
