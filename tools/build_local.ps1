[CmdletBinding()]
param(
    [string]$EngineRoot = "",
    [ValidateSet("Development", "DebugGame")]
    [string]$Configuration = "Development",
    [switch]$ValidateOnly,
    [switch]$Package,
    [string]$ArchiveDirectory = ""
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
    $candidates = @("py", "python", "python3")
    foreach ($candidate in $candidates) {
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

    $buildBat = Join-Path $Path "Engine\Build\BatchFiles\Build.bat"
    $uatBat = Join-Path $Path "Engine\Build\BatchFiles\RunUAT.bat"
    return (Test-Path $buildBat) -and (Test-Path $uatBat)
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

    $commonRoots = @(
        "C:\Program Files\Epic Games\UE_5.8",
        "C:\Epic Games\UE_5.8",
        "D:\Program Files\Epic Games\UE_5.8",
        "D:\Epic Games\UE_5.8"
    )

    foreach ($root in $commonRoots) {
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

    throw @"
Unreal Engine 5.8 was not found.

Either:
  1. install UE 5.8 through Epic Games Launcher,
  2. pass -EngineRoot "C:\...\UE_5.8", or
  3. set UE_5_8_ROOT to the UE 5.8 install directory.
"@
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

if (-not (Test-Path $ProjectFile)) {
    throw "Project file not found: $ProjectFile"
}

$Python = Resolve-Python

Write-Step "Running Warrior's Paradise source + free-asset validation"
Invoke-Checked -Executable $Python -Arguments @(
    (Join-Path $RepoRoot "tools\validate_project.py")
)

if ($ValidateOnly) {
    Write-Host ""
    Write-Host "Validation completed successfully." -ForegroundColor Green
    exit 0
}

$ResolvedEngineRoot = Find-EngineRoot -ExplicitRoot $EngineRoot
Write-Host "Using Unreal Engine: $ResolvedEngineRoot" -ForegroundColor Green

$BuildBat = Join-Path $ResolvedEngineRoot "Engine\Build\BatchFiles\Build.bat"

Write-Step "Building WarriorParadiseEditor Win64 $Configuration"
Invoke-Checked -Executable $BuildBat -Arguments @(
    "WarriorParadiseEditor",
    "Win64",
    $Configuration,
    $ProjectFile,
    "-WaitMutex",
    "-NoHotReloadFromIDE"
)

if ($Package) {
    $RunUAT = Join-Path $ResolvedEngineRoot "Engine\Build\BatchFiles\RunUAT.bat"

    if ([string]::IsNullOrWhiteSpace($ArchiveDirectory)) {
        $ArchiveDirectory = Join-Path $RepoRoot "LocalBuilds\Win64"
    }

    New-Item -ItemType Directory -Force -Path $ArchiveDirectory | Out-Null

    Write-Step "Packaging Win64 Development build"
    Invoke-Checked -Executable $RunUAT -Arguments @(
        "BuildCookRun",
        "-project=$ProjectFile",
        "-noP4",
        "-platform=Win64",
        "-clientconfig=Development",
        "-build",
        "-cook",
        "-stage",
        "-pak",
        "-archive",
        "-archivedirectory=$ArchiveDirectory",
        "-unattended",
        "-utf8output"
    )

    Write-Host ""
    Write-Host "Packaged build: $ArchiveDirectory" -ForegroundColor Green
}

Write-Host ""
Write-Host "Warrior's Paradise local build pipeline completed successfully." -ForegroundColor Green
