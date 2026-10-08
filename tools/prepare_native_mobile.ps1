# Prepare an authored UE5.8 mobile map with tracked CC0/free art.
# Requires Unreal 5.8, Python Editor Script Plugin and local ExternalAssets.
[CmdletBinding()]
param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [switch]$SetStartupMap,
    [switch]$RequireProductionArt
)
$ErrorActionPreference = "Stop"
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$Project = Join-Path $Root "WarriorParadise.uproject"
$Executable = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Script = Join-Path $Root "Content\Python\wp_prepare_native_mobile.py"
$Map = Join-Path $Root "Content\WarriorParadise\Maps\WP_NormandyMobile.umap"

if (-not (Test-Path $Executable)) {
    throw "UE5.8 editor not found at $Executable (provide -EngineRoot)"
}
if (-not (Test-Path $Project)) { throw "Project is missing at $Project" }
if (-not (Test-Path $Script)) { throw "Mobile Python pipeline is missing" }
if (Test-Path $Map) {
    throw "Refusing to overwrite existing map $Map. Back it up or manage it in UE Editor."
}
# Always generate and hash the eight project-original environment meshes.
# This is an offline starter art pipeline; it does not create professional
# rigged soldiers, weapon meshes or final commercial-quality materials.
& python (Join-Path $Root "tools\assets\generate_original_props.py") --root $Root
if ($LASTEXITCODE -ne 0) {
    throw "Original starter geometry generation failed; check for edited/generated file conflicts."
}
$Validator = Join-Path $Root "tools\assets\validate_asset_provenance.py"
& python $Validator --root $Root
if ($LASTEXITCODE -ne 0) {
    throw "Art integrity failed; refusing Unreal import."
}

Write-Host "[WP MOBILE ART] Import verified assets, build materials and generate map"
& $Executable $Project "-ExecutePythonScript=$Script" "-unattended" "-nop4" "-stdout" "-FullStdOutLogOutput"
if ($LASTEXITCODE -ne 0) { throw "Unreal Editor art pipeline failed. Inspect editor logs." }
if (-not (Test-Path $Map)) {
    throw "Editor exited without generating $Map. Inspect Python/Unreal logs."
}
Write-Host "[WP MOBILE ART] Saved map: $Map"
Write-Host "[WP MOBILE ART] Visually inspect the level, character rigging and asset license lock."

if ($SetStartupMap) {
    $Audit = Join-Path $Root "tools\assets\activate_mobile_map.py"
    $Args = @($Audit, "--apply")
    if ($RequireProductionArt) { $Args += "--require-production-art" }
    & python @Args
    if ($LASTEXITCODE -ne 0) { throw "Could not activate authored mobile map." }
}
