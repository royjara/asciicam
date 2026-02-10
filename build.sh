#!/bin/bash

# Build script for img2ascii on macOS with Homebrew

set -e

# Check if required packages are installed
if ! pkg-config --exists gstreamer-1.0; then
    echo "Error: GStreamer not found. Install with:"
    echo "brew install gstreamer gst-plugins-base gst-plugins-good gst-plugins-bad gst-plugins-ugly"
    exit 1
fi

if ! brew list glfw &> /dev/null; then
    echo "Error: GLFW not found. Install with:"
    echo "brew install glfw"
    exit 1
fi

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(sysctl -n hw.ncpu)

echo "Build complete! Run with: ./build/img2ascii"