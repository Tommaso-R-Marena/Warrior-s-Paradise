# Scalability and Performance Presets

Warrior's Paradise is designed to keep one gameplay simulation while scaling rendering cost across a broad range of PC hardware.

The project defines five user-facing graphics modes:

| Mode | UE base quality | Internal resolution target | Frame cap | Intended use |
|---|---:|---:|---:|---|
| Auto | Hardware benchmark | Benchmark result, clamped to 67–100% | Benchmark/default | First launch and recommended default |
| Low | 0 / Low | 67% | 60 FPS | Older dedicated GPUs / performance-first |
| Medium | 1 / Medium | 78% | 60 FPS | Mainstream lower/midrange PCs |
| High | 2 / High | 90% | 60 FPS | Primary Enlisted-quality performance target |
| Ultra | 3 / Epic | 100% | 120 FPS | Modern high-end GPUs / maximum presentation |

"Ultra" is the Warrior's Paradise name for Unreal's Epic scalability tier. Cinematic is intentionally not exposed as a normal gameplay preset.

## Automatic first-run detection

`UWPGameUserSettings` is registered as the project's Game User Settings class.

On the first playable launch it:

1. loads persisted settings;
2. runs Unreal Engine's hardware benchmark;
3. lets UE choose per-group CPU/GPU scalability values;
4. clamps internal resolution to a readable 67–100% range;
5. applies and saves the result;
6. records the user-facing preset as `Auto`.

The benchmark is not rerun every launch.

## Manual controls

These controls are global because they are bound in `AWPPlayerController`, not in an infantry/vehicle pawn:

| Input | Action |
|---|---|
| F10 | Cycle Low → Medium → High → Ultra |
| F9 | Rerun automatic hardware benchmark |

They therefore continue to work while controlling infantry, a tank, a light vehicle or an emplacement.

The HUD displays the current preset in the upper-right corner.

## Project scalability policy

`Config/DefaultScalability.ini` overrides only project-relevant values while inheriting the remainder of UE 5.8's base scalability system.

### Low

- UE Low global-illumination/reflection path; this disables Lumen according to UE's standard Low scalability behavior;
- 67% internal resolution;
- shorter view distance;
- one skeletal-mesh LOD bias step;
- lower shadow resolution;
- volumetric fog disabled;
- reduced post-processing;
- reduced texture pressure;
- reduced foliage/grass density, but **never zero**.

### Medium

- UE Medium rendering groups;
- 78% internal resolution;
- moderate view distance/shadow/post-process settings;
- reduced foliage density;
- volumetric fog remains disabled.

### High

- UE High rendering groups;
- 90% internal resolution;
- Lumen High is the main 60-FPS visual target;
- richer shadows/post-processing;
- 85% foliage density;
- volumetric fog enabled.

### Ultra

- UE Epic rendering groups;
- 100% internal resolution;
- full foliage density;
- highest project shadow/post-process/texture settings;
- intended for modern high-end PCs.

## Gameplay fairness

Graphics presets must not materially change gameplay.

In particular:

- Low does not remove foliage entirely;
- tactical smoke remains gameplay-authoritative regardless of the visual smoke effect;
- cover collision, ballistics, AI sight logic, suppression and objective logic do not depend on graphics quality;
- vehicle armor, aircraft, emplacements and infantry counts are not reduced by a graphics preset.

CPU/AI simulation LOD should only be introduced after profiling demonstrates a need, and it should preserve gameplay outcomes.

## Why High is the primary quality target

Epic's UE 5.8 Lumen guidance describes High scalability as the 60-FPS Lumen target, while Epic scalability spends more of the frame budget for higher quality. Warrior's Paradise therefore treats High—not Ultra—as the normal high-fidelity gameplay target.

Ultra exists for screenshots/high-end machines rather than being the minimum intended experience.

## Profiling still required

These values are **design presets**, not measured hardware requirements yet.

Once the first UE 5.8 build runs, each tier must be profiled using representative battle scenes for:

- GPU frame time;
- game-thread time;
- render-thread time;
- VRAM;
- system RAM;
- shader compilation/stutter;
- 1% lows;
- 1080p and 1440p behavior;
- worst-case smoke/explosion/vehicle/AI combinations.

Only measured results should become published minimum/recommended specifications.
