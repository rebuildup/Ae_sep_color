#!/usr/bin/env pwsh
# Sets up the After Effects SDK for Windows runners by downloading the SDK
# archive from a GitHub release, extracting Headers/Util folders (including
# nested archives), and copying the current project into the SDK Template tree.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ExpectedSdkPath,

    [Parameter(Mandatory = $true)]
    [string]$ProjectDir,

    [Parameter(Mandatory = $true)]
    [string]$SdkAsset,

    [string]$Repo = "rebuildup/my-ae-sdk-storage",

    [string[]]$ReleaseTags = @("sdk", "ae-sdk_25.6_61", "v25.6.61", "25.6.61"),

    [string]$Token
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $Token) {
    $Token = $env:AE_SDK_TOKEN
}
if (-not $Token) {
    $Token = $env:AE_SDK_DOWNLOAD_TOKEN
}
if (-not $Token) {
    $Token = $env:GITHUB_TOKEN
}
if (-not $Token) {
    throw "AE_SDK token is not configured. Please set AE_SDK_DOWNLOAD_TOKEN."
}

function LogInfo([string]$Message) { Write-Output ("[INFO] {0}" -f $Message) }
function LogWarn([string]$Message) { Write-Output ("[WARN] {0}" -f $Message) }
function LogError([string]$Message) { Write-Output ("[ERROR] {0}" -f $Message) }
function LogBlank() { Write-Output "" }

if ((Test-Path "$ExpectedSdkPath\Headers") -and (Test-Path "$ExpectedSdkPath\Util")) {
    LogInfo ("AeSDK already prepared at {0}" -f $ExpectedSdkPath)
    return
}

function Get-SevenZipPath {
    $command = Get-Command 7z.exe -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }

    $candidates = @(
        "$env:ProgramFiles\7-Zip\7z.exe",
        "$env:ProgramFiles(x86)\7-Zip\7z.exe",
        "C:\Program Files\7-Zip\7z.exe"
    )
    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) { return $candidate }
    }
    return $null
}

$SevenZipPath = Get-SevenZipPath
if (-not $SevenZipPath) {
    LogWarn "7-Zip was not found; falling back to Expand-Archive only."
}

function Invoke-GitHubApi {
    param(
        [string]$Uri
    )

    $headers = @{
        Authorization = "token $Token"
        Accept        = "application/vnd.github+json"
        "User-Agent"  = "github-actions-ae-sdk"
    }

    return Invoke-RestMethod -Uri $Uri -Headers $headers -ErrorAction Stop
}

function Get-ReleaseMetadata {
    foreach ($tag in $ReleaseTags) {
        try {
            LogInfo ("Requesting release metadata (tag: {0})" -f $tag)
            return Invoke-GitHubApi -Uri "https://api.github.com/repos/$Repo/releases/tags/$tag"
        } catch {
            LogWarn ("Tag '{0}' not available ({1})" -f $tag, $_.Exception.Response.StatusCode.value__)
        }
    }

    LogWarn "Falling back to latest release metadata."
    try {
        return Invoke-GitHubApi -Uri "https://api.github.com/repos/$Repo/releases/latest"
    } catch {
        LogError "Unable to obtain release metadata from GitHub."
        LogError ("Status: {0}" -f $_.Exception.Response.StatusCode.value__)
        LogError ("Message: {0}" -f $_.Exception.Message)
        throw
    }
}

$releaseJson = Get-ReleaseMetadata

$asset = $releaseJson.assets | Where-Object { $_.name -eq $SdkAsset } | Select-Object -First 1
if (-not $asset) {
    LogError ("SDK asset '{0}' was not found in release '{1}'." -f $SdkAsset, $releaseJson.tag_name)
    Write-Output "Available assets:"
    $releaseJson.assets | ForEach-Object { Write-Output ("  - {0}" -f $_.name) }
    throw "SDK asset missing from release."
}

LogInfo ("Download URL (API): {0}" -f $asset.url)
if ($asset.browser_download_url) {
    LogInfo ("Download URL (browser): {0}" -f $asset.browser_download_url)
}

$outputPath = Join-Path (Get-Location) "AE_SDK.zip"

function Download-Asset {
    param(
        [string]$Uri,
        [bool]$Authenticated
    )

    $headers = @{
        Accept       = "application/octet-stream"
        "User-Agent" = "github-actions-ae-sdk"
    }
    if ($Authenticated) {
        $headers.Authorization = "token $Token"
    }

    try {
        Invoke-WebRequest -Uri $Uri -Headers $headers -OutFile $outputPath -ErrorAction Stop
        return $true
    } catch {
        LogWarn ("Download failed from {0}: {1}" -f $Uri, $_.Exception.Message)
        return $false
    }
}

if (-not (Download-Asset -Uri $asset.url -Authenticated $true)) {
    if ($asset.browser_download_url) {
        LogInfo "Retrying download via browser URL without authentication."
        if (-not (Download-Asset -Uri $asset.browser_download_url -Authenticated $false)) {
            throw "SDK download failed for both API and browser URLs."
        }
    } else {
        throw "SDK download failed and no browser_download_url was provided."
    }
}

if (-not (Test-Path $outputPath) -or (Get-Item $outputPath).Length -eq 0) {
    throw "Downloaded SDK archive is missing or empty."
}

$sizeMb = [math]::Round((Get-Item $outputPath).Length / 1MB, 2)
LogInfo ("Download complete ({0} MB)." -f $sizeMb)

$extractDir = Join-Path $env:RUNNER_TEMP ("ae_sdk_" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $extractDir -Force | Out-Null

function Expand-ZipWithFallback {
    param(
        [Parameter(Mandatory = $true)][string]$Archive,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    try {
        Expand-Archive -LiteralPath $Archive -DestinationPath $Destination -Force -ErrorAction Stop
        return $true
    } catch {
        if ($SevenZipPath) {
            LogWarn ("Expand-Archive failed ({0}); retrying with 7-Zip." -f $_.Exception.Message)
            & $SevenZipPath x $Archive ("-o$Destination") -y | Out-Null
            return $true
        }

        LogError ("Expand-Archive failed and 7-Zip is unavailable: {0}" -f $_.Exception.Message)
        return $false
    }
}

if (-not (Expand-ZipWithFallback -Archive $outputPath -Destination $extractDir)) {
    Get-ChildItem $extractDir -Recurse -ErrorAction SilentlyContinue | Select-Object -First 40 | Format-Table FullName, Length -AutoSize
    throw "Unable to extract SDK archive."
}

function Expand-ArchiveSmart {
    param(
        [string]$Archive,
        [string]$Destination
    )

    $lower = $Archive.ToLowerInvariant()

    if ($lower.EndsWith(".zip")) {
        return (Expand-ZipWithFallback -Archive $Archive -Destination $Destination)
    }

    if ($SevenZipPath) {
        & $SevenZipPath x $Archive ("-o" + $Destination) -y | Out-Null
        return $true
    }

    if ($lower.EndsWith(".tar")) {
        & tar -xf $Archive -C $Destination
        return $true
    }

    if ($lower.EndsWith(".tar.gz") -or $lower.EndsWith(".tgz")) {
        & tar -xzf $Archive -C $Destination
        return $true
    }

    if ($lower.EndsWith(".tar.bz2") -or $lower.EndsWith(".tbz") -or $lower.EndsWith(".tbz2")) {
        & tar -xjf $Archive -C $Destination
        return $true
    }

    if ($lower.EndsWith(".tar.xz") -or $lower.EndsWith(".txz")) {
        & tar -xJf $Archive -C $Destination
        return $true
    }

    LogWarn ("Unsupported archive format: {0}" -f $Archive)
    return $false
}

function Expand-NestedArchives {
    param([string]$Root)

    for ($i = 0; $i -lt 6; $i++) {
        $archives = Get-ChildItem -Path $Root -Recurse -File -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -match '\.(zip|tar|tgz|tar\.gz|tbz|tar\.bz2|txz|tar\.xz|tar\.zst|tar\.zstd|zst|zstd)$' }
        if (-not $archives) { break }

        $expanded = $false
        foreach ($archive in $archives) {
            if (-not (Test-Path $archive.FullName)) { continue }
            $dest = Join-Path $archive.Directory.FullName ("nested_{0}_{1}" -f $i, [guid]::NewGuid().ToString("N"))
            New-Item -ItemType Directory -Force -Path $dest | Out-Null
            if (Expand-ArchiveSmart -Archive $archive.FullName -Destination $dest) {
                Remove-Item $archive.FullName -Force -ErrorAction SilentlyContinue
                $expanded = $true
            } else {
                Remove-Item -Recurse -Force $dest -ErrorAction SilentlyContinue
            }
        }

        if (-not $expanded) { break }
    }
}

Expand-NestedArchives -Root $extractDir

function Get-SdkSourceDirs {
    param([string]$Root)

    $headersDirs = Get-ChildItem -Path $Root -Directory -Recurse -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -ieq "Headers" }

    foreach ($hdr in $headersDirs) {
        $utilPath = Join-Path $hdr.Parent.FullName "Util"
        if (Test-Path $utilPath) {
            return [pscustomobject]@{
                Headers = $hdr.FullName
                Util    = $utilPath
                Root    = $hdr.Parent.FullName
            }
        }
    }
    return $null
}

function Invoke-SdkPrepScripts {
    param([string]$Root)

    $scripts = Get-ChildItem -Path $Root -Filter "*.bat" -Recurse -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '(setup|install|extract|expand|prepare|unpack)' } |
        Sort-Object FullName

    foreach ($script in $scripts) {
        LogInfo ("Running vendor script: {0}" -f $script.FullName)
        Push-Location -LiteralPath $script.DirectoryName
        try {
            & cmd.exe /c "`"$($script.FullName)`""
        } finally {
            Pop-Location
        }

        if ($LASTEXITCODE -ne 0) {
            LogWarn ("Script exited with code {0}" -f $LASTEXITCODE)
        }

        $result = Get-SdkSourceDirs -Root $Root
        if ($result) { return $result }
    }

    return $null
}

function Invoke-BuildAll {
    param([string]$Root)

    $solution = Get-ChildItem -Path $Root -Recurse -Filter "BuildAll.sln" -ErrorAction SilentlyContinue |
        Select-Object -First 1

    if ($solution) {
        LogInfo ("Building vendor solution: {0}" -f $solution.FullName)
        & msbuild $solution.FullName `
            /m `
            /p:Configuration=Release `
            /p:Platform=x64 `
            /v:minimal
    } else {
        LogWarn "BuildAll.sln not found; skipping vendor compilation."
    }
}

$sources = Get-SdkSourceDirs -Root $extractDir
if (-not $sources) {
    $sources = Invoke-SdkPrepScripts -Root $extractDir
}

if (-not $sources) {
    LogError "Headers/Util directories not found inside SDK archive."
    Get-ChildItem $extractDir -Recurse -Directory | Select-Object -First 40 | Format-Table FullName
    throw "SDK archive did not contain Headers/Util."
}

$sdkRoot = $sources.Root
Invoke-BuildAll -Root $sdkRoot

if ($ProjectDir -and (Test-Path $ProjectDir)) {
    $sdkTemplateRoot = Join-Path $sdkRoot "Template"
    New-Item -ItemType Directory -Force -Path $sdkTemplateRoot | Out-Null
    $targetTemplate = Join-Path $sdkTemplateRoot (Split-Path $ProjectDir -Leaf)
    if (Test-Path $targetTemplate) {
        Remove-Item -Recurse -Force $targetTemplate -ErrorAction SilentlyContinue
    }
    LogInfo ("Copying project into SDK template: {0}" -f $targetTemplate)
    Copy-Item -Recurse -Force $ProjectDir $targetTemplate
}

New-Item -ItemType Directory -Force -Path $ExpectedSdkPath | Out-Null
Remove-Item -Recurse -Force (Join-Path $ExpectedSdkPath "Headers") -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force (Join-Path $ExpectedSdkPath "Util") -ErrorAction SilentlyContinue
Copy-Item -Recurse -Force $sources.Headers (Join-Path $ExpectedSdkPath "Headers")
Copy-Item -Recurse -Force $sources.Util (Join-Path $ExpectedSdkPath "Util")

if ((Test-Path "$ExpectedSdkPath\Headers") -and (Test-Path "$ExpectedSdkPath\Util")) {
    $headerCount = (Get-ChildItem "$ExpectedSdkPath\Headers" -File | Measure-Object).Count
    $utilCount = (Get-ChildItem "$ExpectedSdkPath\Util" -File | Measure-Object).Count
    LogInfo "AeSDK setup complete."
    LogInfo ("Headers: {0}\Headers ({1} files)" -f $ExpectedSdkPath, $headerCount)
    LogInfo ("Util    : {0}\Util ({1} files)" -f $ExpectedSdkPath, $utilCount)
} else {
    LogError "AeSDK setup failed."
    Write-Output ("Expected path: {0}" -f $ExpectedSdkPath)
    Write-Output "Presence check:"
    if (Test-Path "$ExpectedSdkPath") {
        Write-Output "  - Root directory exists"
    } else {
        Write-Output "  - Root directory is missing"
    }
    if (Test-Path "$ExpectedSdkPath\Headers") {
        Write-Output "  - Headers\ exists"
    } else {
        Write-Output "  - Headers\ is missing"
    }
    if (Test-Path "$ExpectedSdkPath\Util") {
        Write-Output "  - Util\ exists"
    } else {
        Write-Output "  - Util\ is missing"
    }
    Write-Output "Expanded directories:"
    $parentDir = Split-Path -Parent $ExpectedSdkPath
    Get-ChildItem $parentDir | Select-Object -First 20 | Format-Table Name, Length, LastWriteTime
    throw "AeSDK copy incomplete."
}

Remove-Item $outputPath -Force -ErrorAction SilentlyContinue
Remove-Item $extractDir -Recurse -Force -ErrorAction SilentlyContinue
