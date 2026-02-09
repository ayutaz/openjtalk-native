#!/bin/bash
# iOS-specific dependency build script for OpenJTalk

set -e

echo "=== Building OpenJTalk Dependencies for iOS ==="

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
EXTERNAL_DIR="$ROOT_DIR/external"
INSTALL_DIR="$EXTERNAL_DIR/install_ios"

# iOS specific settings
export PLATFORM="OS64"
export DEPLOYMENT_TARGET="11.0"
export ARCH="arm64"

# Get the iOS SDK path
XCODE_PATH=$(xcode-select -p)
SDK_PATH="${XCODE_PATH}/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk"

if [ ! -d "$SDK_PATH" ]; then
    echo "ERROR: iOS SDK not found at $SDK_PATH"
    echo "Please install Xcode and run: sudo xcode-select --switch /Applications/Xcode.app"
    exit 1
fi

echo "Using iOS SDK: $SDK_PATH"

# iOS compiler flags
export CC="xcrun -sdk iphoneos clang"
export CXX="xcrun -sdk iphoneos clang++"
export AR="xcrun -sdk iphoneos ar"
export RANLIB="xcrun -sdk iphoneos ranlib"
export CFLAGS="-arch arm64 -isysroot $SDK_PATH -mios-version-min=$DEPLOYMENT_TARGET -fembed-bitcode"
export CXXFLAGS="$CFLAGS"
export LDFLAGS="-arch arm64 -isysroot $SDK_PATH -mios-version-min=$DEPLOYMENT_TARGET"

# Build hts_engine
echo "=== Building hts_engine for iOS ==="
if [ -d "$EXTERNAL_DIR/hts_engine_API-1.10" ]; then
    cd "$EXTERNAL_DIR/hts_engine_API-1.10"
    if [ -f "Makefile" ]; then
        make clean || true
    fi
    ./configure \
        --prefix="$INSTALL_DIR" \
        --host=arm-apple-darwin \
        --enable-static \
        --disable-shared \
        --disable-dependency-tracking || {
        echo "ERROR: hts_engine configure failed"
        exit 1
    }
    make -j$(sysctl -n hw.ncpu) || {
        echo "ERROR: hts_engine build failed"
        exit 1
    }
    make install
else
    echo "ERROR: hts_engine_API-1.10 directory not found!"
    exit 1
fi

# Build OpenJTalk
echo "=== Building OpenJTalk for iOS ==="
if [ -d "$EXTERNAL_DIR/open_jtalk-1.11" ]; then
    cd "$EXTERNAL_DIR/open_jtalk-1.11"
    if [ -f "Makefile" ]; then
        make clean || true
    fi
    ./configure \
        --prefix="$INSTALL_DIR" \
        --host=arm-apple-darwin \
        --with-hts-engine-header-path="$INSTALL_DIR/include" \
        --with-hts-engine-library-path="$INSTALL_DIR/lib" \
        --enable-static \
        --disable-shared \
        --disable-dependency-tracking || {
        echo "ERROR: OpenJTalk configure failed"
        exit 1
    }
    for dir in mecab/src text2mecab mecab2njd njd njd_set_pronunciation njd_set_digit njd_set_accent_phrase njd_set_accent_type njd_set_unvoiced_vowel njd_set_long_vowel njd2jpcommon jpcommon; do
        if [ -d "$dir" ]; then
            echo "Building $dir..."
            (cd "$dir" && make -j$(sysctl -n hw.ncpu)) || {
                echo "ERROR: Failed to build $dir"
                exit 1
            }
        fi
    done
else
    echo "ERROR: open_jtalk-1.11 directory not found!"
    exit 1
fi

echo "=== iOS Dependencies built successfully ==="
