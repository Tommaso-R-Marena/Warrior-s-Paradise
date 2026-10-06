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

1. validates the project and asset catalog with `tools/validate_project.py`;
2. runs `tools/check_compile_readiness.py` to audit Unreal reflection, includes and delegate definitions;
3. runs dependency-free `unittest` regression tests for the checker;
4. discovers an Unreal Engine 5.8 installation;
5. invokes Unreal Build Tool through Epic's `Build.bat`;
6. builds `WarriorParadiseEditor Win64 Development`.

## Engine discovery

The script checks:

- `-EngineRoot` supplied explicitly;
- `UE_5_8_ROOT`;
- Epic Games Launcher's install manifest;
- standard Epic Games registry entries;
- the current-user Unreal Engine custom-build registry;
- common Epic Launcher install locations.

Example:

```powershell
.\tools\build_local.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.8"
```

## Clean authoritative compile

To remove project build intermediates and compile both the editor and Win64 game targets:

```powershell
.\tools\build_local.ps1 -Clean -BuildGameTarget
```

This is the same core verification used by the self-hosted GitHub Actions compile gate.

## GPU-less gameplay smoke

Once UE 5.8 is installed, the same machine can run the battlefield simulation without a rendering device:

```powershell
.\tools\build_local.ps1 -Clean -HeadlessSmoke
```

This uses Unreal's NullRHI and returns a non-zero process code if the initialization/runtime smoke contract fails.

For a visually playable machine with integrated graphics but no discrete GPU:

```powershell
.\tools\build_local.ps1 -LaunchIntegrated
```

See `docs/GPU_LESS_MODE.md`.

## Validate without Unreal

```powershell
.\tools\build_local.ps1 -ValidateOnly
```

This checks C++ source contracts, project/plugin configuration, Python syntax, the free-asset catalog, license labels, acquisition/distribution metadata, import/material/profile scripts, gameplay/presentation integration and regression-tested dynamic-delegate handler declarations/implementations. It needs **Python 3 only**; neither Unreal nor GitHub Actions is required.

From a Windows PowerShell terminal at the repository root:

```powershell
.\tools\build_local.ps1 -ValidateOnly
```

On macOS/Linux or any machine with Python 3, run the equivalent directly:

```bash
python3 tools/validate_project.py
python3 tools/check_compile_readiness.py
python3 -m unittest discover -s tools/tests -p 'test_*.py' -v
```

The source-only check is not a C++ compilation or gameplay test.

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

The local build and the Windows self-hosted GitHub Actions compile gate are the authoritative validation paths while hosted CI minutes are unavailable. A source-contract pass is useful but is **not** a substitute for Unreal Header Tool, C++ compilation, asset loading and PIE/package validation. The first clean local UE 5.8 build should be treated as a milestone and any compiler/runtime defect should be fixed before expanding game systems again.

## Automatic GitHub source checks

`.github/workflows/validate.yml` now triggers on relevant pushes to `main`, pull requests into `main`, or a manual dispatch. Multiple quick commits on the same branch cancel older in-flight checks to conserve GitHub-hosted runner minutes. Documentation-only edits do not trigger static CI.

GitHub can reject a hosted job **before any script runs** because of runner/account/Actions availability. In this case the run may show a failure with no steps or logs. The local `-ValidateOnly` command above remains the independent, no-Unreal fallback. Inspect the GitHub Actions run banner and account Actions/billing settings if hosted checks never start. Do not label source verification as a UE compile pass.
