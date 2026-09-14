# Builds SplitPlay (Release, x64 + x86) and produces a portable zip in the repository root.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File tools\build-release.ps1
#
# Output:
#   SplitPlay.zip  (contains a single "SplitPlay" folder with SplitPlay.exe and the hook DLLs)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$solution = Join-Path $repoRoot "src\SplitPlay\SplitPlay.sln"
$releaseDir = Join-Path $repoRoot "src\SplitPlay\Release"

function Find-MSBuild
{
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere)
    {
        $path = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
        if ($path) { return $path }
    }

    $candidates = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
    )

    foreach ($candidate in $candidates)
    {
        if (Test-Path $candidate) { return $candidate }
    }

    throw "MSBuild.exe not found. Install Visual Studio with the C++ workload."
}

$msbuild = Find-MSBuild
Write-Host "Using MSBuild: $msbuild"

# The MSVC v143/v145 compiler can occasionally throw a transient internal compiler
# error (C1001) on this codebase. Retrying the build almost always succeeds, so we
# retry automatically before giving up.
function Invoke-BuildWithRetry
{
    param(
        [string]$Platform,
        [string]$Label,
        [int]$MaxAttempts = 5
    )

    for ($attempt = 1; $attempt -le $MaxAttempts; $attempt++)
    {
        Write-Host "Building $Label (attempt $attempt of $MaxAttempts)..."

        $output = & $msbuild $solution /p:Configuration=Release /p:Platform=$Platform /m /v:minimal 2>&1
        $exitCode = $LASTEXITCODE
        $output | Write-Host

        if ($exitCode -eq 0)
        {
            return
        }

        $transient = ($output | Select-String -Pattern "error C1001|fatal error C1001" -Quiet) -or
                     ($output | Select-String -Pattern "LNK1181|LNK1104" -Quiet)

        if (-not $transient)
        {
            throw "$Label build failed"
        }

        Write-Host "Transient compiler/linker error detected. Retrying..."
        Start-Sleep -Seconds 2
    }

    throw "$Label build failed after $MaxAttempts attempts"
}

Invoke-BuildWithRetry -Platform "x64" -Label "x64 Release"
Invoke-BuildWithRetry -Platform "x86" -Label "x86 Release"

# Assemble the portable folder.
# We always stage into a temp dir and copy it into a clean "SplitPlay" folder so the
# zip always contains "SplitPlay/<files>" even if an old folder was locked by a running game.
$stagingRoot = Join-Path $repoRoot "out\_staging"
if (Test-Path $stagingRoot)
{
    try { Remove-Item -Recurse -Force $stagingRoot -ErrorAction Stop }
    catch { $stagingRoot = Join-Path $repoRoot ("out\_staging-" + (Get-Date -Format "yyyyMMdd-HHmmss")) }
}
$staging = Join-Path $stagingRoot "SplitPlay"
New-Item -ItemType Directory -Force -Path $staging | Out-Null

$requiredFiles = @(
    "x64\SplitPlay.exe",
    "SplitPlayHooks32.dll",
    "SplitPlayHooks64.dll",
    "SplitPlayLoader32.dll",
    "SplitPlayLoader64.dll",
    "SplitPlayIJ32.exe",
    "SplitPlayIJ64.exe",
    "SplitPlayIJP32.dll",
    "SplitPlayIJP64.dll"
)

foreach ($file in $requiredFiles)
{
    $source = Join-Path $releaseDir $file
    if (!(Test-Path $source))
    {
        throw "Missing required build output: $file"
    }

    Copy-Item $source -Destination $staging
}

Copy-Item (Join-Path $repoRoot "README.md") -Destination $staging -ErrorAction SilentlyContinue

# Zip it up
$zipPath = Join-Path $repoRoot "SplitPlay.zip"
if (Test-Path $zipPath)
{
    try
    {
        Remove-Item -Force $zipPath -ErrorAction Stop
    }
    catch
    {
        $zipPath = Join-Path $repoRoot ("SplitPlay-" + (Get-Date -Format "yyyyMMdd-HHmmss") + ".zip")
        Write-Host "Note: SplitPlay.zip was locked. Writing: $zipPath"
    }
}

Compress-Archive -Path $staging -DestinationPath $zipPath

Write-Host ""
Write-Host "Done. Portable release: $zipPath"
Write-Host "Contents (unzips to a single SplitPlay folder):"
Get-ChildItem $staging | ForEach-Object { Write-Host "  SplitPlay/$($_.Name)" }
