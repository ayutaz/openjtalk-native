# OpenJTalk dependency fetcher script for Windows
# Downloads OpenJTalk 1.11 and HTS Engine API 1.10

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = Split-Path -Parent $ScriptDir
$ExternalDir = Join-Path $RootDir "external"

# Create external directory
New-Item -ItemType Directory -Force -Path $ExternalDir | Out-Null
Set-Location $ExternalDir

$OpenJTalkVersion = "1.11"
$HTSEngineVersion = "1.10"

# GitHub Release mirror for stable downloads (SourceForge hashes can change due to CDN re-compression)
$GitHubMirrorBase = "https://github.com/ayutaz/openjtalk-native/releases/download/deps-v1"

# SHA256 checksums of .tar.gz files hosted on GitHub Release (stable)
$HTSEngineGzSHA256 = "e2132be5860d8fb4a460be766454cfd7c3e21cf67b509c48e1804feab14968f7"
$OpenJTalkGzSHA256 = "20fdc6aeb6c757866034abc175820573db43e4284707c866fcd02c8ec18de71f"

# SHA256 checksums of inner tar content (stable across gzip re-compression)
$HTSEngineTarSHA256 = "d2924c144f3463b0caea21b6e03cd1573f945652e71f6f221642699e3acec1ff"
$OpenJTalkTarSHA256 = "1c80e8478e0fa8b8d19e404c897d943d1c496aa84862bf60f58c100c30dd4804"

function Verify-Checksum {
    param(
        [string]$FilePath,
        [string]$ExpectedHash
    )

    if ([string]::IsNullOrEmpty($ExpectedHash)) {
        Write-Host "WARNING: No expected checksum provided, skipping verification for $FilePath"
        return $true
    }

    $actualHash = (Get-FileHash -Path $FilePath -Algorithm SHA256).Hash.ToLower()
    if ($actualHash -ne $ExpectedHash) {
        Write-Host "WARNING: Archive checksum mismatch for $FilePath (may be due to SourceForge re-compression)"
        Write-Host "  Expected: $ExpectedHash"
        Write-Host "  Actual:   $actualHash"
        return $false
    }
    Write-Host "Checksum verified for $FilePath"
    return $true
}

function Verify-GzipFormat {
    param([string]$FilePath)

    $bytes = [System.IO.File]::ReadAllBytes($FilePath)
    if ($bytes.Length -lt 2 -or $bytes[0] -ne 0x1F -or $bytes[1] -ne 0x8B) {
        Write-Host "ERROR: $FilePath is not a valid gzip file"
        Remove-Item $FilePath -Force
        return $false
    }
    return $true
}

function Verify-TarChecksum {
    param(
        [string]$FilePath,
        [string]$ExpectedHash
    )

    if ([string]::IsNullOrEmpty($ExpectedHash)) {
        Write-Host "WARNING: No inner tar checksum provided, skipping for $FilePath"
        return $true
    }

    try {
        $inStream = [System.IO.File]::OpenRead($FilePath)
        $gzStream = New-Object System.IO.Compression.GZipStream($inStream, [System.IO.Compression.CompressionMode]::Decompress)
        $sha256 = [System.Security.Cryptography.SHA256]::Create()
        $hashBytes = $sha256.ComputeHash($gzStream)
        $gzStream.Close()
        $inStream.Close()
        $actualHash = -join ($hashBytes | ForEach-Object { $_.ToString("x2") })

        if ($actualHash -ne $ExpectedHash) {
            Write-Host "ERROR: Inner tar checksum mismatch for $FilePath"
            Write-Host "  Expected: $ExpectedHash"
            Write-Host "  Actual:   $actualHash"
            Remove-Item $FilePath -Force
            return $false
        }
        Write-Host "Inner tar checksum verified for $FilePath"
        return $true
    } catch {
        Write-Host "WARNING: Failed to verify inner tar checksum: $_"
        return $true
    }
}

function Download-WithRetry {
    param(
        [string]$Url,
        [string]$Output,
        [int]$MaxRetries = 3
    )

    for ($i = 1; $i -le $MaxRetries; $i++) {
        Write-Host "Downloading from $Url (attempt $i/$MaxRetries)..."
        try {
            [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
            Invoke-WebRequest -Uri $Url -OutFile $Output -UseBasicParsing -TimeoutSec 300
            Write-Host "Download successful!"
            return $true
        } catch {
            Write-Host "Download failed: $_"
            if ($i -lt $MaxRetries) {
                Write-Host "Retrying in 5 seconds..."
                Start-Sleep -Seconds 5
            }
        }
    }
    return $false
}

function Download-WithFallback {
    param(
        [string]$PrimaryUrl,
        [string]$FallbackUrl,
        [string]$Output
    )

    Write-Host "Trying primary mirror (GitHub)..."
    try {
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        Invoke-WebRequest -Uri $PrimaryUrl -OutFile $Output -UseBasicParsing -TimeoutSec 120
        Write-Host "Downloaded from GitHub mirror."
        return $true
    } catch {
        Write-Host "Primary mirror unavailable: $_"
    }

    Write-Host "Trying SourceForge..."
    return (Download-WithRetry -Url $FallbackUrl -Output $Output)
}

function Extract-TarGz {
    param([string]$Archive)

    if (Get-Command tar -ErrorAction SilentlyContinue) {
        tar xzf $Archive
    } else {
        # Fallback: use 7zip if available
        if (Get-Command 7z -ErrorAction SilentlyContinue) {
            7z x $Archive -so | 7z x -si -ttar
        } else {
            Write-Error "Neither tar nor 7z found. Please install one of them."
            exit 1
        }
    }
}

Write-Host "=== Fetching OpenJTalk Dependencies ==="

# Download hts_engine_API
$HTSDir = "hts_engine_API-$HTSEngineVersion"
if (-not (Test-Path $HTSDir)) {
    Write-Host "Downloading hts_engine_API $HTSEngineVersion..."
    $result = Download-WithFallback `
        -PrimaryUrl "$GitHubMirrorBase/hts_engine_API-$HTSEngineVersion.tar.gz" `
        -FallbackUrl "https://sourceforge.net/projects/hts-engine/files/hts_engine%20API/hts_engine_API-$HTSEngineVersion/hts_engine_API-$HTSEngineVersion.tar.gz/download" `
        -Output "hts_engine_API.tar.gz"

    if (-not $result) {
        Write-Error "Failed to download hts_engine_API"
        exit 1
    }

    if (-not (Verify-GzipFormat "hts_engine_API.tar.gz")) {
        Write-Error "Downloaded hts_engine_API is not a valid gzip file"
        exit 1
    }

    if (-not (Verify-Checksum "hts_engine_API.tar.gz" $HTSEngineGzSHA256)) {
        if (-not (Verify-TarChecksum "hts_engine_API.tar.gz" $HTSEngineTarSHA256)) {
            Write-Error "Checksum verification failed for hts_engine_API"
            exit 1
        }
    }

    Write-Host "Extracting hts_engine_API..."
    Extract-TarGz "hts_engine_API.tar.gz"
    Remove-Item "hts_engine_API.tar.gz"
}

# Download OpenJTalk
$OJTDir = "open_jtalk-$OpenJTalkVersion"
if (-not (Test-Path $OJTDir)) {
    Write-Host "Downloading OpenJTalk $OpenJTalkVersion..."
    $result = Download-WithFallback `
        -PrimaryUrl "$GitHubMirrorBase/open_jtalk-$OpenJTalkVersion.tar.gz" `
        -FallbackUrl "https://sourceforge.net/projects/open-jtalk/files/Open%20JTalk/open_jtalk-$OpenJTalkVersion/open_jtalk-$OpenJTalkVersion.tar.gz/download" `
        -Output "open_jtalk.tar.gz"

    if (-not $result) {
        Write-Error "Failed to download OpenJTalk"
        exit 1
    }

    if (-not (Verify-GzipFormat "open_jtalk.tar.gz")) {
        Write-Error "Downloaded OpenJTalk is not a valid gzip file"
        exit 1
    }

    if (-not (Verify-Checksum "open_jtalk.tar.gz" $OpenJTalkGzSHA256)) {
        if (-not (Verify-TarChecksum "open_jtalk.tar.gz" $OpenJTalkTarSHA256)) {
            Write-Error "Checksum verification failed for OpenJTalk"
            exit 1
        }
    }

    Write-Host "Extracting OpenJTalk..."
    Extract-TarGz "open_jtalk.tar.gz"
    Remove-Item "open_jtalk.tar.gz"
}

Write-Host "=== Dependencies fetched successfully ==="
Write-Host "  hts_engine_API-$HTSEngineVersion"
Write-Host "  open_jtalk-$OpenJTalkVersion"
