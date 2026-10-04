# Local UE 5.8 Build and Validation

GitHub Actions is not required to validate or build Warrior's Paradise.

The repository contains a Windows build wrapper:

```powershell
.\tools\build_local.ps1
```

or:

```bat
tools\build_local.bat
```

## What the default command does

1. runs `python tools/validate_project.py`;
2. discovers an Unreal Engine 5.8 installation;
3. invokes Unreal Build Tool through Epic's `Build.bat`;
4. builds `WarriorParadiseEditor Win64 Development`.

## Engine discovery

The script checks:

- `-EngineRoot` supplied explicitly;
- `UE_5_8_ROOT`;
- common Epic Launcher install locations;
- the current-user Unreal Engine build registry.

Example:

```powershell
.\tools\build_local.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.8"
```

## Validate without Unreal

```powershell
.\tools\build_local.ps1 -ValidateOnly
```

This checks C++ source contracts, project/plugin configuration, Python syntax, the free-asset catalog, license labels, acquisition/distribution metadata, import/material/profile scripts and the gameplay/presentation integration.

## Package a local Windows build

```powershell
.\tools\build_local.ps1 -Package
```

The default archive is:

```text
LocalBuilds/Win64/
```

You can choose another destination:

```powershell
.\tools\build_local.ps1 -Package -ArchiveDirectory "D:\WarriorParadiseBuild"
```

Packaging uses Unreal Automation Tool `BuildCookRun` with Win64 Development, build, cook, stage and pak enabled.

## Why this exists

The local build is the authoritative validation gate while hosted CI minutes are unavailable. A source-contract pass is useful but is **not** a substitute for Unreal Header Tool, C++ compilation, asset loading and PIE/package validation. The first clean local UE 5.8 build should be treated as a milestone and any compiler/runtime defect should be fixed before expanding game systems again.
