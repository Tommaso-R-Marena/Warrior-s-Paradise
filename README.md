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

- runtime battlefield fallback with buildings, cover and atmosphere;
- damageable cover that breaks into temporary physics debris;
- penetrable thin cover;
- UE 5.8 Local Fog Volume tactical smoke;
- one Allied armored vehicle during the inner-village push;
- one Axis armored vehicle in the final counterattack;
- autonomous vehicle movement, turret tracking, cannon fire and coaxial fire;
- smoke-aware vehicle sight;
- rifle-caliber armor immunity;
- **directional front/side/rear shell damage**;
- tank shells with blast damage/suppression;
- victory gating on surviving hostile armor.

## Production-facing event hooks

Gameplay exposes presentation events for:

- shot fired / projectile spawned / bullet impact;
- reload start / complete / cancel / dry fire;
- health and damage source;
- downed / revive / death;
- suppression;
- melee and bandaging;
- grenade explosion and smoke deployment;
- artillery impacts;
- cannon / coax fire, armor damage and vehicle destruction.

See [Presentation Bindings](docs/PRESENTATION_BINDINGS.md).

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
| E | Revive nearby ally |
| X | Switch to next living squadmate |
| 1 | Squad Follow |
| 2 | Squad Attack objective |
| 3 | Squad Hold |
| T | Designate artillery |

## Engine and run

Target: **Unreal Engine 5.8**.

1. Install UE 5.8 and Visual Studio 2022 with Game development with C++.
2. Clone the repository and check out the vertical-slice branch.
3. Generate project files if requested.
4. Build `WarriorParadiseEditor`.
5. Open `WarriorParadise.uproject`.
6. Press Play.

The current fallback uses Unreal's built-in Entry map and constructs the battle at runtime. That keeps gameplay code testable without committing production binary map assets.

## Validation

```bash
python3 tools/validate_project.py
```

The contract checks UE 5.8 targeting plus the implemented stance, medicine, perception, squad-switching, smoke, objective, destruction, artillery and armor hooks.

A **full Unreal C++ compile/playtest has still not been executed from this ChatGPT environment**. The connected GitHub hosted runner has been failing before job execution, so the workflow remains manually dispatchable rather than producing repeated false-red automatic runs.

## What remains before this is visually Enlisted-quality

The remaining gap is dominated by production content and engine validation, not missing combat-rule scaffolding:

- production skeletal soldiers, hands, weapons and vehicles;
- animation blueprints, locomotion, transitions, reloads, recoil and additive poses;
- authentic WWII environment/terrain/interior asset pass;
- Niagara muzzle/impact/explosion/dust/debris effects;
- spatial weapon, explosion, vehicle, voice and foley audio;
- physical materials, decals and per-surface response;
- authored level/navigation pass and Chaos destruction where useful;
- UE 5.8 compile fixes, playtesting, tuning, Unreal Insights profiling and packaged-build validation.

See [Vertical Slice](docs/VERTICAL_SLICE.md), [Architecture](docs/ARCHITECTURE.md), [Asset Pipeline](docs/ASSET_PIPELINE.md), and [Performance Target](docs/PERFORMANCE_TARGET.md).
