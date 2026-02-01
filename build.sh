#!/bin/bash

set -euo pipefail

SRC_DIR="src"
BUILD_DIR="build"
INCLUDES_DIR="includes"
LIB_TYPE="shared" # default

usage() {
    cat <<EOF
Usage: $0 [--shared|--static]

Options:
  --shared    Build shared library (default)
  --static    Build static archive
  -h, --help   Show this help message
EOF
}

if [[ ${#@} -gt 0 ]]; then
    for arg in "$@"; do
        case "$arg" in
            --shared)
                LIB_TYPE="shared"
                shift || true
                ;;
            --static)
                LIB_TYPE="static"
                shift || true
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            *)
                echo "Unknown option: $arg"
                usage
                exit 1
                ;;
        esac
    done
fi

mkdir -p "$BUILD_DIR"
mkdir -p "$BUILD_DIR/includes"

echo "Build start... (type=$LIB_TYPE)"

if [[ "$LIB_TYPE" == "shared" ]]; then
    OUTPUT="$BUILD_DIR/libhttp.so"
    g++ -shared -fPIC -I. "$SRC_DIR"/*.cpp -o "$OUTPUT" -O2 -std=c++11
    if [ -f "$OUTPUT" ]; then
        echo "Build successful: $OUTPUT"
    fi
else
    # static: compile object files then archive
    OBJ_DIR="$BUILD_DIR/obj"
    mkdir -p "$OBJ_DIR"
    echo "Compiling object files to $OBJ_DIR..."
    for f in "$SRC_DIR"/*.cpp; do
        obj="$OBJ_DIR/$(basename "${f%.cpp}.o")"
        echo "  compiling $(basename "$f") -> $(basename "$obj")"
        g++ -c -fPIC -I. "$f" -o "$obj" -O2 -std=c++11
    done
    OUTPUT="$BUILD_DIR/libhttp.a"
    echo "Archiving to $OUTPUT"
    ar rcs "$OUTPUT" "$OBJ_DIR"/*.o
    rm -r "$OBJ_DIR"
    echo "Build successful: $OUTPUT"
fi

echo "Copying header files to build directory..."
cp "$INCLUDES_DIR"/http.hpp "$BUILD_DIR/includes/"
cp "$INCLUDES_DIR"/http_request.hpp "$BUILD_DIR/includes/"
cp "$INCLUDES_DIR"/http_response.hpp "$BUILD_DIR/includes/"
cp "$INCLUDES_DIR"/http_constants.hpp "$BUILD_DIR/includes/"
echo "Copied successfully."