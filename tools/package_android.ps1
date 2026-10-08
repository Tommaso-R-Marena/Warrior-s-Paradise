# Development Android packaging through the official UE5.8 AutomationTool.
# This wrapper cannot replace the Android SDK/NDK/JDK setup or signing.
[CmdletBinding()]
param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [ValidateSet("ETC2", "ASTC")][string]$TextureFormat = "ETC2",
    [ValidateSet("Development", "Shipping")][string]$Configuration = "Development",
    [string]$ArchiveDirectory = "",
    [switch]$ValidateOnly,
    [switch]$RequireProductionArt
)

$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$ProjectFile = Join-Path $RepoRoot "WarriorParadise.uproject"
$PythonCommand = Get-Command python -ErrorAction SilentlyContinue
if (-not $PythonCommand) {
    throw "Python 3 is required for source preflight checks. Install Python or run this script on a Windows development machine."
}

Write-Host "[WP ANDROID] Running project and mobile-source preflight"
& $PythonCommand.Source (Join-Path $RepoRoot "tools\validate_project.py")
if ($LASTEXITCODE -ne 0) { throw "Repository contract validation failed." }
& $PythonCommand.Source (Join-Path $RepoRoot "tools\check_compile_readiness.py")
if ($LASTEXITCODE -ne 0) { throw "Compile-readiness source audit failed." }
& $PythonCommand.Source -m unittest discover -s (Join-Path $RepoRoot "tools\tests") -p "test_*.py" -v
if ($LASTEXITCODE -ne 0) { throw "Source regression tests failed." }

# Development packages may intentionally use code-generated stand-ins.
# Shipping requires a minimally populated, reviewed art inventory.
$ArtAuditArgs = @((Join-Path $RepoRoot "tools\assets\audit_mobile_content.py"))
if ($RequireProductionArt -or $Configuration -eq "Shipping") {
    $ArtAuditArgs += "--strict"
}
& $PythonCommand.Source @ArtAuditArgs
if ($LASTEXITCODE -ne 0) {
    throw "Production 3D art readiness gate failed. Import/verify assets before Shipping."
}

if ($Configuration -eq "Shipping") {
    # Prevent a seemingly successful package containing only the engine Entry
    # map, even if imported art is present elsewhere on disk.
    $AuthoredMap = Join-Path $RepoRoot "Content\WarriorParadise\Maps\WP_NormandyMobile.umap"
    $DefaultEngine = Get-Content (Join-Path $RepoRoot "Config\DefaultEngine.ini") -Raw
    if (-not (Test-Path $AuthoredMap)) {
        throw "Shipping blocked: no authored Normandy .umap. Run tools/prepare_native_mobile.ps1."
    }
    if (-not $DefaultEngine.Contains("GameDefaultMap=/Game/WarriorParadise/Maps/WP_NormandyMobile")) {
        throw "Shipping blocked: game still starts in Entry, not the authored Normandy map."
    }
}

if ($ValidateOnly) {
    Write-Host "[WP ANDROID] Source preflight passed; NO Unreal build or APK was produced."
    exit 0
}

if (-not (Test-Path $ProjectFile)) { throw "Missing WarriorParadise.uproject" }
$UAT = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
if (-not (Test-Path $UAT)) {
    throw "Unreal 5.8 AutomationTool not found: $UAT. Set -EngineRoot to your UE5.8 installation."
}
if ([string]::IsNullOrWhiteSpace($ArchiveDirectory)) {
    $ArchiveDirectory = Join-Path $RepoRoot "Saved\AndroidBuilds\$TextureFormat"
}
New-Item -ItemType Directory -Force -Path $ArchiveDirectory | Out-Null

Write-Host "[WP ANDROID] Android $TextureFormat $Configuration build"
Write-Host "[WP ANDROID] Ensure Unreal Project Settings > Android > Configure Now and Turnkey SDK setup have been completed."
$Arguments = @(
    "BuildCookRun",
    "-project=$ProjectFile",
    "-noP4",
    "-platform=Android",
    "-cookflavor=$TextureFormat",
    "-clientconfig=$Configuration",
    "-build",
    "-cook",
    "-stage",
    "-package",
    "-pak",
    "-archive",
    "-archivedirectory=$ArchiveDirectory",
    "-utf8output"
)
& $UAT @Arguments
if ($LASTEXITCODE -ne 0) {
    throw "Unreal AutomationTool failed with exit code $LASTEXITCODE. Inspect Saved/Logs and UAT logs. This is NOT a successful APK."
}
Write-Host "[WP ANDROID] AutomationTool completed. Check packaged APK/install scripts under $ArchiveDirectory."
