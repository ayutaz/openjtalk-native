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

# Expected SHA256 checksums (update these when upgrading versions)
$HTSEngineSHA256 = "e2132be5e550c1de2d36ab23711e2d9cc58e6efca5768afa5b76cf0b174e7b00"
$OpenJTalkSHA256 = "1e2e564da5e64b289056eb57eee04078e72ea0e224d67cd5f512c6eb6e3a3e94"

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
        Write-Host "ERROR: Checksum mismatch for $FilePath"
        Write-Host "  Expected: $ExpectedHash"
        Write-Host "  Actual:   $actualHash"
        Remove-Item $FilePath -Force
        return $false
    }
    Write-Host "Checksum verified for $FilePath"
    return $true
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
    $result = Download-WithRetry `
        -Url "https://sourceforge.net/projects/hts-engine/files/hts_engine%20API/hts_engine_API-$HTSEngineVersion/hts_engine_API-$HTSEngineVersion.tar.gz/download" `
        -Output "hts_engine_API.tar.gz"

    if (-not $result) {
        Write-Error "Failed to download hts_engine_API after multiple attempts"
        exit 1
    }

    if (-not (Verify-Checksum "hts_engine_API.tar.gz" $HTSEngineSHA256)) {
        Write-Error "Checksum verification failed for hts_engine_API"
        exit 1
    }

    Write-Host "Extracting hts_engine_API..."
    Extract-TarGz "hts_engine_API.tar.gz"
    Remove-Item "hts_engine_API.tar.gz"
}

# Download OpenJTalk
$OJTDir = "open_jtalk-$OpenJTalkVersion"
if (-not (Test-Path $OJTDir)) {
    Write-Host "Downloading OpenJTalk $OpenJTalkVersion..."
    $result = Download-WithRetry `
        -Url "https://sourceforge.net/projects/open-jtalk/files/Open%20JTalk/open_jtalk-$OpenJTalkVersion/open_jtalk-$OpenJTalkVersion.tar.gz/download" `
        -Output "open_jtalk.tar.gz"

    if (-not $result) {
        Write-Error "Failed to download OpenJTalk after multiple attempts"
        exit 1
    }

    if (-not (Verify-Checksum "open_jtalk.tar.gz" $OpenJTalkSHA256)) {
        Write-Error "Checksum verification failed for OpenJTalk"
        exit 1
    }

    Write-Host "Extracting OpenJTalk..."
    Extract-TarGz "open_jtalk.tar.gz"
    Remove-Item "open_jtalk.tar.gz"
}

Write-Host "=== Dependencies fetched successfully ==="
Write-Host "  hts_engine_API-$HTSEngineVersion"
Write-Host "  open_jtalk-$OpenJTalkVersion"
