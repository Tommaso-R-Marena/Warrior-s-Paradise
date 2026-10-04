# Self-hosted UE 5.8 compile gate

Warrior's Paradise keeps its source private while using a Windows self-hosted GitHub Actions runner for real Unreal Engine compilation. Self-hosted jobs do not consume GitHub-hosted Actions minutes.

## One-time runner requirements

Install on the Windows x64 machine:

- Unreal Engine 5.8;
- Visual Studio 2022 with **Game development with C++** and the Windows SDK selected by Unreal;
- Git;
- Python 3;
- the GitHub Actions self-hosted runner registered to this repository.

The workflow targets the standard runner labels:

```text
self-hosted
Windows
X64
```

No custom label is required.

## Engine discovery

`tools/build_local.ps1` searches for UE 5.8 in this order:

1. the explicit `-EngineRoot` argument;
2. `UE_5_8_ROOT`;
3. Epic Games Launcher's `LauncherInstalled.dat`;
4. standard Epic Games registry entries;
5. Unreal custom-build registry entries;
6. common `C:`, `D:`, and `E:` Epic Games install locations.

If automatic detection fails, set a machine-level `UE_5_8_ROOT` environment variable to the UE 5.8 installation directory and restart the runner service/process.

## Running the compile gate

In GitHub:

1. Open **Actions**.
2. Select **UE 5.8 compile gate**.
3. Select **Run workflow** on `main`.
4. Leave **Clean** enabled for an authoritative verdict.
5. Use **Development** unless debugging a configuration-specific problem.

The workflow:

1. checks out the exact requested commit;
2. verifies Python and the project/build scripts;
3. runs `tools/validate_project.py`;
4. deletes project `Binaries` and `Intermediate` for a clean build when requested;
5. builds `WarriorParadiseEditor Win64`;
6. builds the shipping-relevant `WarriorParadise Win64` game target;
7. prints recent Unreal Build Tool logs automatically if either target fails;
8. publishes a final **PASS** or **FAIL** summary.

A PASS means the checked commit compiled successfully against the UE 5.8 installation on that runner. It does not by itself prove that gameplay, assets, cooking, packaging, rendering, or runtime behavior are correct.

## Local equivalent

The same authoritative source compile can be run directly on the Windows machine:

```powershell
.\tools\build_local.ps1 -Clean -BuildGameTarget
```

To package after compilation:

```powershell
.\tools\build_local.ps1 -Clean -BuildGameTarget -Package
```

## Security

Keep the runner attached to the private repository. If the repository is ever made public, do not allow arbitrary fork pull requests to execute on a persistent self-hosted machine.
