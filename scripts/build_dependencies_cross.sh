#!/bin/bash
# Cross-compile build script for Windows using MinGW on Linux

set -ex

echo "=== Building OpenJTalk Dependencies for Windows Cross-Compilation ==="

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
EXTERNAL_DIR="$ROOT_DIR/external"
INSTALL_DIR="$EXTERNAL_DIR/install"

cd "$EXTERNAL_DIR"

# MinGW toolchain setup
export CC=x86_64-w64-mingw32-gcc
export CXX=x86_64-w64-mingw32-g++
export AR=x86_64-w64-mingw32-ar
export RANLIB=x86_64-w64-mingw32-ranlib
export WINDRES=x86_64-w64-mingw32-windres
export STRIP=x86_64-w64-mingw32-strip

export CFLAGS="-O3 -static-libgcc -static-libstdc++"
export CXXFLAGS="-O3 -static-libgcc -static-libstdc++"
export LDFLAGS="-static-libgcc -static-libstdc++ -static"

JOBS=$(nproc)

echo "Using $JOBS parallel jobs"

# Build hts_engine for Windows
echo "=== Building hts_engine for Windows ==="
if [ -d "hts_engine_API-1.10" ]; then
    cd hts_engine_API-1.10
    if [ -f "Makefile" ]; then
        make clean || true
    fi
    ./configure \
        --host=x86_64-w64-mingw32 \
        --prefix="$INSTALL_DIR" \
        --enable-static \
        --disable-shared || {
        echo "ERROR: hts_engine configure failed"
        exit 1
    }
    make -j$JOBS || {
        echo "ERROR: hts_engine build failed"
        exit 1
    }
    make install
    cd ..
else
    echo "ERROR: hts_engine_API-1.10 directory not found!"
    exit 1
fi

# Build OpenJTalk for Windows
echo "=== Building OpenJTalk for Windows ==="
if [ -d "open_jtalk-1.11" ]; then
    cd open_jtalk-1.11
    if [ -f "Makefile" ]; then
        make clean || true
    fi
    ./configure \
        --host=x86_64-w64-mingw32 \
        --prefix="$INSTALL_DIR" \
        --with-hts-engine-header-path="$INSTALL_DIR/include" \
        --with-hts-engine-library-path="$INSTALL_DIR/lib" \
        --with-charset=UTF-8 \
        --enable-static \
        --disable-shared || {
        echo "ERROR: OpenJTalk configure failed"
        exit 1
    }
    if [ -d "mecab/src" ]; then
        echo "Building mecab library..."
        (cd mecab/src && make libmecab.a -j$JOBS)
    fi
    for dir in text2mecab mecab2njd njd njd_set_pronunciation njd_set_digit njd_set_accent_phrase njd_set_accent_type njd_set_unvoiced_vowel njd_set_long_vowel njd2jpcommon jpcommon; do
        if [ -d "$dir" ]; then
            echo "Building $dir..."
            (cd "$dir" && make -j$JOBS)
        fi
    done
    cd ..
else
    echo "ERROR: open_jtalk-1.11 directory not found!"
    exit 1
fi

echo "=== Dependencies built successfully for Windows ==="
