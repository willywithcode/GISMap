n dê#!/bin/bash

# GIS Map Application Build Script

echo "=== GIS Map Application Build Script ==="

# Create necessary directories
echo "Creating necessary directories..."
mkdir -p resources/shapefiles
mkdir -p resources/tiles
mkdir -p build

# Check dependencies
echo "Checking dependencies..."

# Check Qt5
if ! pkg-config --exists Qt5Core; then
    echo "ERROR: Qt5 not found. Please install Qt5 development packages."
    echo "Ubuntu/Debian: sudo apt install qt5-default qtbase5-dev qttools5-dev"
    exit 1
fi

# Check GDAL
if ! pkg-config --exists gdal; then
    echo "ERROR: GDAL not found. Please install GDAL development packages."
    echo "Ubuntu/Debian: sudo apt install libgdal-dev gdal-bin"
    exit 1
fi

# Check PostgreSQL
if ! pkg-config --exists libpqxx; then
    echo "ERROR: libpqxx not found. Please install PostgreSQL development packages."
    echo "Ubuntu/Debian: sudo apt install libpqxx-dev postgresql-client"
    exit 1
fi

echo "All dependencies found!"

# Build
echo "Building application..."
cd build

# Configure with CMake
if ! cmake ..; then
    echo "ERROR: CMake configuration failed!"
    exit 1
fi

# Build
if ! make -j$(nproc); then
    echo "ERROR: Build failed!"
    exit 1
fi

echo "Build successful!"

# Check if we should run the application
if [ "$1" = "run" ]; then
    echo "Starting GIS Map Application..."
    ./GISMap
else
    echo "Build complete. To run the application:"
    echo "  cd build && ./GISMap"
    echo "Or run: ./build.sh run"
fi 