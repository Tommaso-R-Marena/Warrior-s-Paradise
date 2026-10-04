# Warrior's Paradise

**Warrior's Paradise** is an Unreal Engine 5.8 WWII first-person squad-combat project targeting one **Enlisted-quality vertical slice** before expanding content.

The repository now contains a substantial code-side combined-arms battle rather than a browser prototype or simple wave shooter.

## Implemented gameplay

### Infantry and weapon feel

- first-person movement, sprint, crouch, **true prone stance**, jump and stamina;
- ADS with FOV interpolation;
- physical projectiles with gravity and configurable muzzle velocity;
- movement/stance/suppression-dependent spread;
- procedural weapon sway, recoil, recovery and camera impulse;
- skeletal-bone hit multipliers ready for production character rigs;
- thin-cover penetration with retained velocity/damage;
- data-driven weapon definitions through `UWPWeaponDefinition`;
- magazines, reload timing, dry fire, reload cancellation and engineer resupply;
- melee combat;
- frag grenades and tactical smoke grenades;
- timed bandaging/field medicine.

### Wounds and squad lives

- health, downed state, bleedout, finishing damage and revive;
- medics revive wounded soldiers;
- player-issued Follow / Attack / Hold commands;
- **manual squad-member switching** with `X`;
- automatic control transfer after final death;
- all player/squad bodies use the same AI-capable soldier class, so an unpossessed soldier resumes autonomous combat;
- Allied replacement tickets and delayed reinforcement soldiers;
- spawn-front movement and spawn-safety checks.

### Tactical AI

- Allied/Axis teams;
- Rifleman, Assault, Medic and Engineer roles;
- direct LOS and close-range awareness;
- short-lived last-known-position sharing rather than live target omniscience;
- gunshot hearing/investigation;
- tactical smoke blocks AI and vehicle sight;
- reservable cover positions;
- suppression-sensitive movement and fire confidence;
- basic lateral flanking;
- grenade decisions;
- medic smoke/revive behavior;
- engineer ammunition support;
- point-blank AI melee.

### Battle orchestration

- three sequential village capture objectives;
- contested-objective force counts and HUD feedback;
- reinforcement escalation;
- final counterattack;
- Allied/Axis reserve state, score, victory and defeat;
- artillery support with charges/cooldown;
- artillery suppression, cover occlusion, infantry damage, armor damage and destruction effects.

### Battlefield interaction and combined arms

- runtime battlefield fallback with **enterable building collision shells**, cover and atmosphere;
- optional free cabin/building visual overlays that preserve safe gameplay collision;
- Poly Haven PBR surface slots for dirt, plaster, brick, stone, wood, metal, roof and concrete;
- deterministic free tree/crate/barrel dressing plus optional HDRI/volumetric-cloud/ambience presentation;
- damageable cover that breaks into temporary physics debris;
- penetrable thin cover;
- UE 5.8 Local Fog Volume tactical smoke with optional Niagara/deploy/hiss presentation;
- **player-crewable and autonomous tanks**;
- AP / HE ammunition with target-aware AI shell selection;
- directional front/side/rear armor response;
- engine, tracks, turret and ammo-rack subsystem damage;
- mobility/turret degradation, immobilization and catastrophic ammo-rack damage;
- player tank driving, turret/gun control, cannon and coax MG;
- squad transport truck with automatic nearby-squad embark/disembark;
- player-usable and autonomous armored cars;
- player-usable and autonomous HMG / anti-tank emplacements;
- smoke-aware tanks, armored cars and emplacements;
- physical AT shells shared with tank combat;
- scripted/AI aircraft flybys, strafing and bombing;
- physical air-dropped bombs that damage/suppress infantry and damage vehicles/emplacements/cover;
- limited player-requested air support plus artillery;
- recurring ambient aircraft passes and existing battlefield/wind/distant-battle audio;
- victory gating on surviving hostile armor/light vehicles.

## Production presentation layer

The project no longer stops at raw gameplay delegates. It includes data-driven presentation components/profiles for:

- muzzle flash, weapon audio, reload/dry-fire audio;
- live bullet tracers;
- dirt/wood/stone/metal/flesh impact Niagara and audio;
- persistent material-specific impact decals;
- soldier hit, pain, downed, revive, death, melee and bandage VFX/audio/montage hooks;
- frag and artillery explosion Niagara/audio;
- tactical-smoke Niagara plus deploy/hiss audio;
- tank cannon/coax muzzle presentation, shell impacts and destruction;
- spatialized world audio with runtime attenuation/occlusion fallbacks for infantry presentation;
- production soldier/weapon/tank/environment visual profiles;
- battle-wide faction/role loadout selection.

See [Presentation Bindings](docs/PRESENTATION_BINDINGS.md).

## Free-content bootstrap

A license-aware local pipeline now turns free resources into usable Unreal content:

```powershell
.\tools\setup_free_content.ps1
```

It can download approved redistributable CC0 content, record hashes/provenance, compile the UE 5.8 editor target, import external assets, build PBR materials, discover installed free content and generate the optional battle preset:

```text
/Game/WarriorParadise/Profiles/DA_WP_BattlePresentation_Free
```

The generator recognizes period weapons, soldiers, Motion Matching AnimBPs, tanks, transport/light vehicles, HMG/AT emplacements, Allied/Axis aircraft, rural buildings, Niagara effects, audio, HDRI/ambience and environment materials. Provider-gated Epic/Fab/Sonniss content is intentionally acquired through the provider rather than scraped or mirrored publicly.

See [Free Content Workflow](docs/FREE_CONTENT_WORKFLOW.md), [Free Asset Bootstrap](docs/FREE_ASSET_BOOTSTRAP.md) and [Third-Party Assets](docs/THIRD_PARTY_ASSETS.md).

## Controls

| Input | Action |
|---|---|
| W / A / S / D | Move |
| Mouse | Look |
| Left Mouse | Fire |
| Right Mouse | Aim down sights |
| Left Shift | Sprint |
| Left Control | Crouch |
| Z | Prone |
| Space | Jump / rise from low stance |
| R | Reload |
| F | Melee |
| V | Bandage |
| G | Frag grenade |
| H | Smoke grenade |
| E | Revive ally / use friendly tank, vehicle or emplacement |
| X | Switch to next living squadmate |
| 1 | Squad Follow |
| 2 | Squad Attack objective |
| 3 | Squad Hold |
| T | Designate artillery |
| Y | Designate air support |
| F10 | Cycle Low / Medium / High / Ultra graphics |
| F9 | Auto-detect graphics from hardware benchmark |

## Graphics scalability

Warrior's Paradise now treats performance tiers as a core product requirement rather than a post-launch optimization.

- **Auto** runs UE 5.8's hardware benchmark once on first launch and stores its per-group recommendation.
- **Low** targets performance-first older dedicated GPUs at 67% internal resolution and UE Low rendering groups.
- **Medium** targets mainstream PCs at 78%.
- **High** is the main high-fidelity/60-FPS design target at 90%; UE 5.8's High Lumen tier is designed around a 60-FPS budget.
- **Ultra** uses Unreal Epic-quality groups at 100% internal resolution for high-end systems.

Low deliberately retains partial foliage density so quality settings do not create a gameplay concealment advantage. Core ballistics, cover, smoke occlusion, AI, armor and objective simulation are unchanged across presets.

See [Scalability and Performance Presets](docs/SCALABILITY.md).

## Engine and run

Target: **Unreal Engine 5.8**.

1. Install UE 5.8 and Visual Studio 2022 with Game development with C++.
2. Clone the repository. The full vertical slice is now on the default `main` branch.
3. Generate project files if requested.
4. Build `WarriorParadiseEditor`.
5. Open `WarriorParadise.uproject`.
6. Press Play.

The current fallback uses Unreal's built-in Entry map and constructs the battle at runtime. That keeps gameplay code testable without committing production binary map assets.

## Validation and local build

Fast local source/content-contract validation:

```bash
python3 tools/validate_project.py
python3 tools/check_compile_readiness.py
```

The compile-readiness audit checks Unreal Header Tool generated-header discipline, reflected type safety, project include resolution, dynamic-delegate handler reflection, target definitions, and module dependencies before an Unreal build is attempted.

Windows UE 5.8 editor build:

```powershell
.\tools\build_local.ps1
```

Clean verification of both the editor target and the Win64 game target:

```powershell
.\tools\build_local.ps1 -Clean -BuildGameTarget
```

Optional packaged Win64 build:

```powershell
.\tools\build_local.ps1 -Package
```

The validator now checks the **71-file** source surface plus Python tooling, plugin requirements, free-asset/license metadata, smoke/soldier/weapon/vehicle presentation, enterable buildings, material-specific impacts, tracers/decals and the free-content bootstrap contract.

`main` now includes a manual **UE 5.8 compile gate** at `.github/workflows/unreal-compile.yml`. It is designed for a Windows x64 self-hosted GitHub Actions runner so it does not consume GitHub-hosted Actions minutes. The gate runs repository validation, removes project build intermediates when requested, builds both `WarriorParadiseEditor` and the `WarriorParadise` Win64 game target, prints Unreal Build Tool diagnostics on failure, and publishes a PASS/FAIL job summary. A **full Unreal C++ compile/playtest has still not been executed**; connecting a Windows runner with UE 5.8 installed is the next hard validation gate.

## What remains before this is visually Enlisted-quality

The remaining gap is now dominated by **content acquisition, art-direction coherence and engine validation**, not missing presentation plumbing:

- acquire/install the chosen free Epic/Fab/Sonniss content locally;
- retarget the Game Animation Sample/Motion Matching stack to the final soldier skeletons where needed;
- create/select historically coherent high-fidelity uniforms, hands, weapons and vehicles;
- replace fallback village dressing with a coherent authored WWII environment/interior pass while keeping the existing safe collision/navigation shell;
- tune Niagara/audio/decal profiles to one consistent visual and sonic language;
- author any hero destruction/animation not supplied by the free sources;
- compile in UE 5.8, fix UHT/C++/asset-load issues, playtest, tune AI/combat, profile with Unreal Insights and validate a packaged Win64 build.

See [Vertical Slice](docs/VERTICAL_SLICE.md), [Combined Arms](docs/COMBINED_ARMS.md), [Architecture](docs/ARCHITECTURE.md), [Asset Pipeline](docs/ASSET_PIPELINE.md), and [Performance Target](docs/PERFORMANCE_TARGET.md), and [Scalability](docs/SCALABILITY.md).
