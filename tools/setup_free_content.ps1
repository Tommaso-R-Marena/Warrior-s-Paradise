[CmdletBinding()]
param(
    [string]$EngineRoot = "",
    [ValidateSet("1k", "2k", "4k", "8k")]
    [string]$Resolution = "2k",
    [switch]$SkipDownload,
    [switch]$SkipBuild,
    [switch]$SkipPolyHaven,
    [switch]$SkipDirectCC0
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$ProjectFile = Join-Path $RepoRoot "WarriorParadise.uproject"

function Write-Step([string]$Message) {
    Write-Host ""
    Write-Host "==> $Message" -ForegroundColor Cyan
}

function Resolve-Python {
    foreach ($candidate in @("py", "python", "python3")) {
        $command = Get-Command $candidate -ErrorAction SilentlyContinue
        if ($null -ne $command) {
            return $command.Source
        }
    }
    throw "Python 3 was not found on PATH."
}

function Test-EngineRoot([string]$Path) {
    if ([string]::IsNullOrWhiteSpace($Path)) {
        return $false
    }
    return Test-Path (Join-Path $Path "Engine\Binaries\Win64\UnrealEditor-Cmd.exe")
}

function Find-EngineRoot {
    param([string]$ExplicitRoot)

    $candidates = New-Object System.Collections.Generic.List[string]

    if (-not [string]::IsNullOrWhiteSpace($ExplicitRoot)) {
        $candidates.Add($ExplicitRoot)
    }

    if (-not [string]::IsNullOrWhiteSpace($env:UE_5_8_ROOT)) {
        $candidates.Add($env:UE_5_8_ROOT)
    }

    foreach ($root in @(
        "C:\Program Files\Epic Games\UE_5.8",
        "C:\Epic Games\UE_5.8",
        "D:\Program Files\Epic Games\UE_5.8",
        "D:\Epic Games\UE_5.8"
    )) {
        $candidates.Add($root)
    }

    $registryPath = "HKCU:\Software\Epic Games\Unreal Engine\Builds"
    if (Test-Path $registryPath) {
        $properties = Get-ItemProperty $registryPath
        foreach ($property in $properties.PSObject.Properties) {
            if ($property.Name -match "^PS") {
                continue
            }

            $value = [string]$property.Value
            if ($value -match "5\.8" -or $property.Name -match "5\.8") {
                $candidates.Add($value)
            }
        }
    }

    foreach ($candidate in $candidates) {
        if (Test-EngineRoot $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    throw "Unreal Engine 5.8 was not found. Pass -EngineRoot, set UE_5_8_ROOT, or install UE 5.8 through Epic Games Launcher."
}

function Invoke-Checked {
    param(
        [Parameter(Mandatory=$true)]
        [string]$Executable,
        [Parameter(Mandatory=$true)]
        [string[]]$Arguments
    )

    Write-Host "$Executable $($Arguments -join ' ')" -ForegroundColor DarkGray
    & $Executable @Arguments

    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE."
    }
}

function Invoke-UnrealPython {
    param(
        [Parameter(Mandatory=$true)]
        [string]$UnrealEditorCmd,
        [Parameter(Mandatory=$true)]
        [string]$ScriptPath
    )

    Write-Step "Running Unreal Python: $([IO.Path]::GetFileName($ScriptPath))"

    Invoke-Checked -Executable $UnrealEditorCmd -Arguments @(
        $ProjectFile,
        "-run=pythonscript",
        "-script=$ScriptPath",
        "-unattended",
        "-nop4",
        "-nosplash",
        "-stdout",
        "-FullStdOutLogOutput"
    )
}

if (-not (Test-Path $ProjectFile)) {
    throw "Project file not found: $ProjectFile"
}

$Python = Resolve-Python

if (-not $SkipDownload) {
    $Bootstrap = Join-Path $RepoRoot "tools\assets\bootstrap_free_assets.py"
    $Arguments = @($Bootstrap)

    if (-not $SkipPolyHaven) {
        $Arguments += "--polyhaven"
        $Arguments += "--resolution"
        $Arguments += $Resolution
    }

    if (-not $SkipDirectCC0) {
        $Arguments += "--direct-cc0"
    }

    if ($Arguments.Count -gt 1) {
        Write-Step "Downloading redistributable free assets"
        Invoke-Checked -Executable $Python -Arguments $Arguments
    }
}

Write-Step "Validating source, licenses and asset-pipeline contracts"
Invoke-Checked -Executable $Python -Arguments @(
    (Join-Path $RepoRoot "tools\validate_project.py")
)

$ResolvedEngineRoot = Find-EngineRoot -ExplicitRoot $EngineRoot
$UnrealEditorCmd = Join-Path $ResolvedEngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
Write-Host "Using Unreal Engine: $ResolvedEngineRoot" -ForegroundColor Green

if (-not $SkipBuild) {
    Write-Step "Compiling WarriorParadiseEditor before asset generation"

    $BuildArgs = @(
        "-NoLogo",
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        (Join-Path $RepoRoot "tools\build_local.ps1"),
        "-EngineRoot",
        $ResolvedEngineRoot
    )

    Invoke-Checked -Executable "powershell.exe" -Arguments $BuildArgs
}

Invoke-UnrealPython -UnrealEditorCmd $UnrealEditorCmd -ScriptPath (Join-Path $RepoRoot "Content\Python\wp_import_external_assets.py")
Invoke-UnrealPython -UnrealEditorCmd $UnrealEditorCmd -ScriptPath (Join-Path $RepoRoot "Content\Python\wp_build_cc0_materials.py")
Invoke-UnrealPython -UnrealEditorCmd $UnrealEditorCmd -ScriptPath (Join-Path $RepoRoot "Content\Python\wp_setup_free_profiles.py")

Write-Step "Checking acquired redistributable asset coverage"
$CoverageScript = Join-Path $RepoRoot "tools\assets\check_free_asset_coverage.py"
& $Python $CoverageScript
$CoverageExit = $LASTEXITCODE

if ($CoverageExit -ne 0) {
    Write-Warning "Free-resource coverage is incomplete. This does not invalidate the project; rerun the bootstrapper or inspect the missing slots above."
}

Write-Step "Final source/pipeline validation"
Invoke-Checked -Executable $Python -Arguments @(
    (Join-Path $RepoRoot "tools\validate_project.py")
)

Write-Host ""
Write-Host "Free-content bootstrap completed." -ForegroundColor Green
Write-Host ""
Write-Host "Optional next additions:" -ForegroundColor Yellow
Write-Host "  - migrate Epic Game Animation Sample content"
Write-Host "  - create/import MetaHuman soldier bodies"
Write-Host "  - install the tracked free Fab Kar98k/MP40/Lee-Enfield/PPSh assets"
Write-Host "  - install the tracked free Niagara packs"
Write-Host "  - copy selected Sonniss WAVs into a local project content folder"
Write-Host ""
Write-Host "After adding any of those, rerun this same command."
