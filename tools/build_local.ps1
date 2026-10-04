[CmdletBinding()]
param(
    [string]$EngineRoot = "",
    [ValidateSet("Development", "DebugGame")]
    [string]$Configuration = "Development",
    [switch]$ValidateOnly,
    [switch]$Clean,
    [switch]$BuildGameTarget,
    [switch]$HeadlessSmoke,
    [ValidateRange(5, 300)]
    [int]$HeadlessSmokeSeconds = 15,
    [switch]$LaunchIntegrated,
    [ValidateRange(640, 3840)]
    [int]$IntegratedResX = 1280,
    [ValidateRange(360, 2160)]
    [int]$IntegratedResY = 720,
    [switch]$Package,
    [string]$ArchiveDirectory = ""
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$ProjectFile = Join-Path $RepoRoot "WarriorParadise.uproject"
$BuildStartedAt = Get-Date

function Write-Step([string]$Message) {
    Write-Host ""
    Write-Host "==> $Message" -ForegroundColor Cyan
}

function Add-Candidate {
    param(
        [System.Collections.Generic.List[string]]$Candidates,
        [string]$Path
    )

    if (-not [string]::IsNullOrWhiteSpace($Path) -and -not $Candidates.Contains($Path)) {
        $Candidates.Add($Path)
    }
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

    Add-Candidate -Candidates $candidates -Path $ExplicitRoot
    Add-Candidate -Candidates $candidates -Path $env:UE_5_8_ROOT

    $launcherManifest = Join-Path $env:ProgramData "Epic\UnrealEngineLauncher\LauncherInstalled.dat"
    if (Test-Path $launcherManifest) {
        try {
            $launcherData = Get-Content -Raw $launcherManifest | ConvertFrom-Json
            foreach ($installation in @($launcherData.InstallationList)) {
                $appName = [string]$installation.AppName
                $artifactId = [string]$installation.ArtifactId
                $appVersion = [string]$installation.AppVersion
                if ($appName -match "UE_5\.8" -or $artifactId -match "UE_5\.8" -or $appVersion -match "^5\.8") {
                    Add-Candidate -Candidates $candidates -Path ([string]$installation.InstallLocation)
                }
            }
        }
        catch {
            Write-Warning "Could not parse Epic Launcher install manifest: $($_.Exception.Message)"
        }
    }

    $registryLocations = @(
        "HKLM:\SOFTWARE\EpicGames\Unreal Engine\5.8",
        "HKLM:\SOFTWARE\WOW6432Node\EpicGames\Unreal Engine\5.8"
    )
    foreach ($registryLocation in $registryLocations) {
        if (Test-Path $registryLocation) {
            try {
                $installedDirectory = (Get-ItemProperty $registryLocation -ErrorAction Stop).InstalledDirectory
                Add-Candidate -Candidates $candidates -Path ([string]$installedDirectory)
            }
            catch {
                Write-Warning "Could not read Unreal Engine registry entry at $registryLocation."
            }
        }
    }

    $customBuildsRegistry = "HKCU:\Software\Epic Games\Unreal Engine\Builds"
    if (Test-Path $customBuildsRegistry) {
        $properties = Get-ItemProperty $customBuildsRegistry
        foreach ($property in $properties.PSObject.Properties) {
            if ($property.Name -match "^PS") {
                continue
            }

            $value = [string]$property.Value
            if ($value -match "5\.8" -or $property.Name -match "5\.8") {
                Add-Candidate -Candidates $candidates -Path $value
            }
        }
    }

    $commonRoots = @(
        "C:\Program Files\Epic Games\UE_5.8",
        "C:\Epic Games\UE_5.8",
        "D:\Program Files\Epic Games\UE_5.8",
        "D:\Epic Games\UE_5.8",
        "E:\Program Files\Epic Games\UE_5.8",
        "E:\Epic Games\UE_5.8"
    )
    foreach ($root in $commonRoots) {
        Add-Candidate -Candidates $candidates -Path $root
    }

    foreach ($candidate in $candidates) {
        if (Test-EngineRoot $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    $candidateList = if ($candidates.Count -gt 0) {
        ($candidates | ForEach-Object { "  - $_" }) -join [Environment]::NewLine
    }
    else {
        "  (no candidate paths discovered)"
    }

    throw @"
Unreal Engine 5.8 was not found.

Checked:
$candidateList

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

Write-Step "Running Unreal compile-readiness audit"
Invoke-Checked -Executable $Python -Arguments @(
    (Join-Path $RepoRoot "tools\check_compile_readiness.py")
)

if ($ValidateOnly) {
    Write-Host ""
    Write-Host "Validation completed successfully." -ForegroundColor Green
    exit 0
}

$ResolvedEngineRoot = Find-EngineRoot -ExplicitRoot $EngineRoot
Write-Host "Using Unreal Engine: $ResolvedEngineRoot" -ForegroundColor Green

if ($Clean) {
    Write-Step "Removing project build intermediates for a clean compile"
    foreach ($relativePath in @("Binaries", "Intermediate")) {
        $path = Join-Path $RepoRoot $relativePath
        if (Test-Path $path) {
            Remove-Item -Recurse -Force $path
            Write-Host "Removed $path" -ForegroundColor DarkGray
        }
    }
}

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

if ($BuildGameTarget) {
    Write-Step "Building WarriorParadise Win64 $Configuration"
    Invoke-Checked -Executable $BuildBat -Arguments @(
        "WarriorParadise",
        "Win64",
        $Configuration,
        $ProjectFile,
        "-WaitMutex"
    )
}

if ($HeadlessSmoke) {
    $EditorCmd = Join-Path $ResolvedEngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

    if (-not (Test-Path $EditorCmd)) {
        throw "UnrealEditor-Cmd.exe was not found: $EditorCmd"
    }

    Write-Step "Running GPU-less NullRHI gameplay smoke test"
    Invoke-Checked -Executable $EditorCmd -Arguments @(
        $ProjectFile,
        "/Engine/Maps/Entry",
        "-game",
        "-nullrhi",
        "-nosound",
        "-unattended",
        "-nopause",
        "-stdout",
        "-FullStdOutLogOutput",
        "-WPHeadlessSmoke",
        "-WPSmokeSeconds=$HeadlessSmokeSeconds"
    )

    Write-Host "GPU-less gameplay smoke test passed." -ForegroundColor Green
}

if ($LaunchIntegrated) {
    $EditorExe = Join-Path $ResolvedEngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"

    if (-not (Test-Path $EditorExe)) {
        throw "UnrealEditor.exe was not found: $EditorExe"
    }

    Write-Step "Launching Minimal / Integrated visual mode"
    Invoke-Checked -Executable $EditorExe -Arguments @(
        $ProjectFile,
        "/Engine/Maps/Entry",
        "-game",
        "-WPIntegratedGraphics",
        "-windowed",
        "-ForceRes",
        "-ResX=$IntegratedResX",
        "-ResY=$IntegratedResY",
        "-nosplash"
    )
}

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

$duration = (Get-Date) - $BuildStartedAt
Write-Host ""
Write-Host ("Warrior's Paradise build pipeline completed successfully in {0:hh\:mm\:ss}." -f $duration) -ForegroundColor Green

if ($env:GITHUB_ACTIONS -eq "true") {
    Write-Host "::notice title=Warrior's Paradise compile gate::UE 5.8 build completed successfully."
}
