# GPU-less and integrated-graphics modes

Warrior's Paradise supports two deliberately different low-hardware paths.

## 1. GPU-less headless simulation

This is a real **no-rendering** mode for development, CI and gameplay simulation. It uses Unreal Engine's `-nullrhi` option, so the engine runs without a rendering hardware interface.

It is not visually playable. It exists so a machine without a usable GPU can still exercise:

- project startup and GameMode initialization;
- battlefield construction and collision;
- objectives and capture state;
- friendly and enemy squad spawning;
- tactical AI;
- ballistics and damage;
- vehicles and emplacements;
- victory/defeat state;
- timers and the combined-arms simulation.

Presentation-only work is skipped in this mode: production presentation assets, environment decoration, atmospheric rendering, environment audio and ambient aircraft flybys are not loaded.

### One-command smoke test

After UE 5.8 is installed:

```powershell
.\tools\build_local.ps1 -Clean -HeadlessSmoke
```

The default simulation window is 15 seconds. To change it:

```powershell
.\tools\build_local.ps1 -Clean -HeadlessSmoke -HeadlessSmokeSeconds 30
```

The command compiles the editor target, launches `UnrealEditor-Cmd.exe` with `-nullrhi`, runs the battle, prints `WP_HEADLESS_SMOKE` telemetry, and exits with code 0 only if the smoke contract passes.

The self-hosted GitHub Actions compile gate also runs this test after compiling both Unreal targets.

### Direct engine launch

The underlying runtime flags are:

```text
-game -nullrhi -nosound -unattended -WPHeadlessSmoke -WPSmokeSeconds=15
```

`-WPHeadless` can be used for an open-ended renderless simulation without the timed smoke-test exit.

## 2. Minimal / Integrated visual mode

A visually playable Unreal game still requires a graphics adapter. For a laptop with no **discrete** GPU but a usable integrated GPU, Warrior's Paradise now has a `Minimal` preset.

Minimal uses:

- scalability level 0;
- 50% internal resolution;
- a 30 FPS cap;
- no Lumen dynamic global illumination;
- no Lumen reflections;
- no virtual shadow maps;
- no volumetric clouds;
- FXAA instead of TSR;
- the existing Low foliage density rather than removing gameplay-relevant concealment.

Core combat, AI, smoke sight blocking, armor, objectives and ballistics are unchanged.

### One-command integrated launch

```powershell
.\tools\build_local.ps1 -LaunchIntegrated
```

This builds the editor target and launches the game at 1280x720 with the Minimal preset. Resolution can be changed:

```powershell
.\tools\build_local.ps1 -LaunchIntegrated -IntegratedResX 960 -IntegratedResY 540
```

The same visual mode can be requested directly with:

```text
-WPIntegratedGraphics
```

The F10 graphics-preset cycle now includes **Minimal** before Low.

## What these modes do not prove

A NullRHI smoke pass proves that the simulation can initialize and remain alive without rendering. It does not validate visual assets, shader compilation, rendering correctness, animation quality or frame rate.

The Minimal preset is intended to make weak integrated graphics viable, but its actual performance target must still be measured on real hardware.
