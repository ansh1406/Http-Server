#!/bin/bash

SRC_DIR="src"
BUILD_DIR="build"
INCLUDES_DIR="includes"
OUTPUT="$BUILD_DIR/libhttp.so"

mkdir -p "$BUILD_DIR"
mkdir -p "$BUILD_DIR/includes"

echo "Build start..."

g++ -shared -fPIC -I. "$SRC_DIR"/*.cpp -o "$OUTPUT" -O2 -std=c++11

if [ -f "$OUTPUT" ]; then
    echo "Build successful"
fi

echo "Copying header files to build directory..."
cp "$INCLUDES_DIR"/http.hpp "$BUILD_DIR/includes/"
cp "$INCLUDES_DIR"/http_request.hpp "$BUILD_DIR/includes/"
cp "$INCLUDES_DIR"/http_response.hpp "$BUILD_DIR/includes/"
cp "$INCLUDES_DIR"/http_constants.hpp "$BUILD_DIR/includes/"
echo "Copied successfully..."