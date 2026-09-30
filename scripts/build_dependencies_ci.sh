#!/bin/bash
# CI-specific dependency build script

set -e

echo "=== Building OpenJTalk Dependencies for CI ==="

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
EXTERNAL_DIR="$ROOT_DIR/external"
INSTALL_DIR="$EXTERNAL_DIR/install"

cd "$EXTERNAL_DIR"

# Platform detection
if [[ "$OSTYPE" == "darwin"* ]]; then
    JOBS=$(sysctl -n hw.ncpu)
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
    JOBS=2
else
    JOBS=$(nproc)
fi

# GitHub Actions: limit parallel jobs
if [ -n "$GITHUB_ACTIONS" ]; then
    echo "Running in GitHub Actions - limiting parallel jobs"
    if [ $JOBS -gt 2 ]; then
        JOBS=2
    fi
fi

echo "Platform: $OSTYPE"
echo "Using $JOBS parallel jobs"

# Build hts_engine
echo "=== Building hts_engine ==="
if [ -d "hts_engine_API-1.10" ]; then
    cd hts_engine_API-1.10
    if [ -f "Makefile" ]; then
        make clean || true
    fi
    if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
        ./configure --prefix="$INSTALL_DIR" || {
            echo "ERROR: hts_engine configure failed"
            exit 1
        }
    else
        CFLAGS="-fPIC" ./configure --prefix="$INSTALL_DIR" || {
            echo "ERROR: hts_engine configure failed"
            exit 1
        }
    fi
    make -j$JOBS || {
        echo "ERROR: hts_engine build failed"
        exit 1
    }
    make install || {
        echo "ERROR: hts_engine install failed"
        exit 1
    }
    cd ..
else
    echo "WARNING: hts_engine_API-1.10 directory not found, skipping..."
fi

# Build OpenJTalk
echo "=== Building OpenJTalk ==="
if [ -d "open_jtalk-1.11" ]; then
    cd open_jtalk-1.11
    if [ -f "Makefile" ]; then
        make clean || true
    fi
    if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
        ./configure --prefix="$INSTALL_DIR" \
            --with-hts-engine-header-path="$INSTALL_DIR/include" \
            --with-hts-engine-library-path="$INSTALL_DIR/lib" \
            --enable-static --disable-shared || {
            echo "ERROR: OpenJTalk configure failed"
            exit 1
        }
    else
        CFLAGS="-fPIC" CXXFLAGS="-fPIC" ./configure --prefix="$INSTALL_DIR" \
            --with-hts-engine-header-path="$INSTALL_DIR/include" \
            --with-hts-engine-library-path="$INSTALL_DIR/lib" \
            --with-charset=UTF-8 \
            --enable-static --disable-shared || {
            echo "ERROR: OpenJTalk configure failed"
            exit 1
        }
    fi

    if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
        # Build libraries only on Windows
        if [ -d "mecab/src" ]; then
            echo "Building mecab library..."
            (cd mecab/src && make libmecab.a -j$JOBS) || {
                echo "ERROR: Failed to build mecab library"
                exit 1
            }
        fi
        for dir in text2mecab mecab2njd njd njd_set_pronunciation njd_set_digit njd_set_accent_phrase njd_set_accent_type njd_set_unvoiced_vowel njd_set_long_vowel njd2jpcommon jpcommon; do
            if [ -d "$dir" ]; then
                echo "Building $dir..."
                (cd "$dir" && make -j$JOBS) || {
                    echo "ERROR: Failed to build $dir"
                    exit 1
                }
            fi
        done
    else
        make -j$JOBS || {
            echo "ERROR: OpenJTalk build failed"
            exit 1
        }
    fi
    cd ..
else
    echo "ERROR: open_jtalk-1.11 directory not found!"
    exit 1
fi

echo "=== Dependencies built successfully ==="
