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

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
EXTERNAL_DIR="$ROOT_DIR/external"

# Create external directory
mkdir -p "$EXTERNAL_DIR"
cd "$EXTERNAL_DIR"

# Version definitions
OPENJTALK_VERSION="1.11"
HTS_ENGINE_VERSION="1.10"

echo "=== Fetching OpenJTalk Dependencies ==="

# Download hts_engine_API
if [ ! -d "hts_engine_API-${HTS_ENGINE_VERSION}" ]; then
    echo "Downloading hts_engine_API ${HTS_ENGINE_VERSION}..."
    download_with_retry \
        "https://sourceforge.net/projects/hts-engine/files/hts_engine%20API/hts_engine_API-${HTS_ENGINE_VERSION}/hts_engine_API-${HTS_ENGINE_VERSION}.tar.gz/download" \
        "hts_engine_API.tar.gz" || \
        error_exit "Failed to download hts_engine_API after multiple attempts"

    echo "Extracting hts_engine_API..."
    tar xzf hts_engine_API.tar.gz || error_exit "Failed to extract hts_engine_API"
    rm hts_engine_API.tar.gz
fi

# Download OpenJTalk
if [ ! -d "open_jtalk-${OPENJTALK_VERSION}" ]; then
    echo "Downloading OpenJTalk ${OPENJTALK_VERSION}..."
    download_with_retry \
        "https://sourceforge.net/projects/open-jtalk/files/Open%20JTalk/open_jtalk-${OPENJTALK_VERSION}/open_jtalk-${OPENJTALK_VERSION}.tar.gz/download" \
        "open_jtalk.tar.gz" || \
        error_exit "Failed to download OpenJTalk after multiple attempts"

    echo "Extracting OpenJTalk..."
    tar xzf open_jtalk.tar.gz || error_exit "Failed to extract OpenJTalk"
    rm open_jtalk.tar.gz
fi

echo "=== Dependencies fetched successfully ==="
echo "  hts_engine_API-${HTS_ENGINE_VERSION}: $(ls -d hts_engine_API-${HTS_ENGINE_VERSION})"
echo "  open_jtalk-${OPENJTALK_VERSION}: $(ls -d open_jtalk-${OPENJTALK_VERSION})"
