#!/bin/bash

# Build script for OpenJTalk dependencies (Linux/macOS)
# Builds hts_engine_API and OpenJTalk using autotools

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
EXTERNAL_DIR="$ROOT_DIR/external"
INSTALL_DIR="$EXTERNAL_DIR/install"

# Platform detection
PLATFORM=$(uname -s)
if [ "$PLATFORM" = "Darwin" ]; then
    JOBS=$(sysctl -n hw.ncpu)
else
    JOBS=$(nproc)
fi

echo "=== Building OpenJTalk Dependencies ==="
echo "Platform: $PLATFORM"
echo "Jobs: $JOBS"

# Build hts_engine_API
echo "=== Building hts_engine_API ==="
cd "$EXTERNAL_DIR/hts_engine_API-1.10"
if [ ! -f "Makefile" ]; then
    CFLAGS="-fPIC" ./configure --prefix="$INSTALL_DIR"
fi
make -j$JOBS
make install

# Build OpenJTalk
echo "=== Building OpenJTalk ==="
cd "$EXTERNAL_DIR/open_jtalk-1.11"
if [ ! -f "Makefile" ]; then
    export CPPFLAGS="-I$INSTALL_DIR/include"
    export LDFLAGS="-L$INSTALL_DIR/lib"

    CFLAGS="-fPIC" CXXFLAGS="-fPIC" ./configure \
        --prefix="$INSTALL_DIR" \
        --with-hts-engine-header-path="$INSTALL_DIR/include" \
        --with-hts-engine-library-path="$INSTALL_DIR/lib" \
        --with-charset=UTF-8
fi
make -j$JOBS

echo "=== Dependencies built successfully ==="
echo "Install directory: $INSTALL_DIR"
