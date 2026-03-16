#!/bin/bash

# OpenJTalk dependency fetcher script
# Downloads OpenJTalk 1.11 and HTS Engine API 1.10

set -e

error_exit() {
    echo "Error: $1" >&2
    exit 1
}

download_with_retry() {
    local url="$1"
    local output="$2"
    local max_retries=3
    local retry_count=0

    while [ $retry_count -lt $max_retries ]; do
        echo "Downloading from $url (attempt $((retry_count + 1))/$max_retries)..."
        if curl -L --fail --connect-timeout 30 --max-time 300 "$url" -o "$output"; then
            echo "Download successful!"
            return 0
        fi
        retry_count=$((retry_count + 1))
        if [ $retry_count -lt $max_retries ]; then
            echo "Download failed, retrying in 5 seconds..."
            sleep 5
        fi
    done
    echo "ERROR: Failed to download after $max_retries attempts"
    return 1
}

download_with_fallback() {
    local primary_url="$1"
    local fallback_url="$2"
    local output="$3"

    echo "Trying primary mirror (GitHub)..."
    if curl -L --fail --connect-timeout 15 --max-time 120 -o "$output" "$primary_url" 2>/dev/null; then
        echo "Downloaded from GitHub mirror."
        return 0
    fi
    echo "Primary mirror unavailable, trying SourceForge..."
    download_with_retry "$fallback_url" "$output"
}

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
EXTERNAL_DIR="$ROOT_DIR/external"

# Create external directory
mkdir -p "$EXTERNAL_DIR"
cd "$EXTERNAL_DIR"

# Version definitions
OPENJTALK_VERSION="1.11"
HTS_ENGINE_VERSION="1.10"

# GitHub Release mirror for stable downloads (SourceForge hashes can change due to CDN re-compression)
GITHUB_MIRROR_BASE="https://github.com/ayutaz/openjtalk-native/releases/download/deps-v1"

# SHA256 checksums of .tar.gz files hosted on GitHub Release (stable)
HTS_ENGINE_GZ_SHA256="e2132be5860d8fb4a460be766454cfd7c3e21cf67b509c48e1804feab14968f7"
OPENJTALK_GZ_SHA256="20fdc6aeb6c757866034abc175820573db43e4284707c866fcd02c8ec18de71f"

# SHA256 checksums of inner tar content (stable across gzip re-compression)
HTS_ENGINE_TAR_SHA256="d2924c144f3463b0caea21b6e03cd1573f945652e71f6f221642699e3acec1ff"
OPENJTALK_TAR_SHA256="1c80e8478e0fa8b8d19e404c897d943d1c496aa84862bf60f58c100c30dd4804"

verify_checksum() {
    local file="$1"
    local expected="$2"
    if [ -z "$expected" ]; then
        echo "WARNING: No expected checksum provided, skipping verification for $file"
        return 0
    fi
    local actual
    if command -v sha256sum >/dev/null 2>&1; then
        actual=$(sha256sum "$file" | awk '{print $1}')
    elif command -v shasum >/dev/null 2>&1; then
        actual=$(shasum -a 256 "$file" | awk '{print $1}')
    else
        echo "WARNING: No SHA256 tool found, skipping checksum verification"
        return 0
    fi
    if [ "$actual" != "$expected" ]; then
        echo "WARNING: Archive checksum mismatch for $file (may be due to SourceForge re-compression)"
        echo "  Expected: $expected"
        echo "  Actual:   $actual"
        return 1
    fi
    echo "Checksum verified for $file"
    return 0
}

verify_gzip_format() {
    local file="$1"
    local magic
    magic=$(xxd -l 2 -p "$file" 2>/dev/null)
    if [ "$magic" != "1f8b" ]; then
        echo "ERROR: $file is not a valid gzip file (magic: $magic)"
        rm -f "$file"
        return 1
    fi
    return 0
}

verify_tar_checksum() {
    local file="$1"
    local expected="$2"
    if [ -z "$expected" ]; then
        echo "WARNING: No inner tar checksum provided, skipping for $file"
        return 0
    fi
    local actual
    if command -v sha256sum >/dev/null 2>&1; then
        actual=$(gzip -dc "$file" | sha256sum | awk '{print $1}')
    elif command -v shasum >/dev/null 2>&1; then
        actual=$(gzip -dc "$file" | shasum -a 256 | awk '{print $1}')
    else
        echo "WARNING: No SHA256 tool found, skipping inner tar verification"
        return 0
    fi
    if [ "$actual" != "$expected" ]; then
        echo "ERROR: Inner tar checksum mismatch for $file"
        echo "  Expected: $expected"
        echo "  Actual:   $actual"
        rm -f "$file"
        return 1
    fi
    echo "Inner tar checksum verified for $file"
    return 0
}

echo "=== Fetching OpenJTalk Dependencies ==="

# Download hts_engine_API
if [ ! -d "hts_engine_API-${HTS_ENGINE_VERSION}" ]; then
    echo "Downloading hts_engine_API ${HTS_ENGINE_VERSION}..."
    download_with_fallback \
        "${GITHUB_MIRROR_BASE}/hts_engine_API-${HTS_ENGINE_VERSION}.tar.gz" \
        "https://sourceforge.net/projects/hts-engine/files/hts_engine%20API/hts_engine_API-${HTS_ENGINE_VERSION}/hts_engine_API-${HTS_ENGINE_VERSION}.tar.gz/download" \
        "hts_engine_API.tar.gz" || \
        error_exit "Failed to download hts_engine_API"

    verify_gzip_format "hts_engine_API.tar.gz" || \
        error_exit "Downloaded hts_engine_API is not a valid gzip file"

    verify_checksum "hts_engine_API.tar.gz" "$HTS_ENGINE_GZ_SHA256" || \
        verify_tar_checksum "hts_engine_API.tar.gz" "$HTS_ENGINE_TAR_SHA256" || \
        error_exit "Checksum verification failed for hts_engine_API"

    echo "Extracting hts_engine_API..."
    tar xzf hts_engine_API.tar.gz || error_exit "Failed to extract hts_engine_API"
    rm hts_engine_API.tar.gz
fi

# Download OpenJTalk
if [ ! -d "open_jtalk-${OPENJTALK_VERSION}" ]; then
    echo "Downloading OpenJTalk ${OPENJTALK_VERSION}..."
    download_with_fallback \
        "${GITHUB_MIRROR_BASE}/open_jtalk-${OPENJTALK_VERSION}.tar.gz" \
        "https://sourceforge.net/projects/open-jtalk/files/Open%20JTalk/open_jtalk-${OPENJTALK_VERSION}/open_jtalk-${OPENJTALK_VERSION}.tar.gz/download" \
        "open_jtalk.tar.gz" || \
        error_exit "Failed to download OpenJTalk"

    verify_gzip_format "open_jtalk.tar.gz" || \
        error_exit "Downloaded OpenJTalk is not a valid gzip file"

    verify_checksum "open_jtalk.tar.gz" "$OPENJTALK_GZ_SHA256" || \
        verify_tar_checksum "open_jtalk.tar.gz" "$OPENJTALK_TAR_SHA256" || \
        error_exit "Checksum verification failed for OpenJTalk"

    echo "Extracting OpenJTalk..."
    tar xzf open_jtalk.tar.gz || error_exit "Failed to extract OpenJTalk"
    rm open_jtalk.tar.gz
fi

echo "=== Dependencies fetched successfully ==="
echo "  hts_engine_API-${HTS_ENGINE_VERSION}: $(ls -d hts_engine_API-${HTS_ENGINE_VERSION})"
echo "  open_jtalk-${OPENJTALK_VERSION}: $(ls -d open_jtalk-${OPENJTALK_VERSION})"
