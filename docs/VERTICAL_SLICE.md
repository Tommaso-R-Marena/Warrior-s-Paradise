# Warrior's Paradise — Enlisted-Quality Vertical Slice

## Product target

Build one complete, polished WWII battle before expanding the content surface.

The target is not a clone of Enlisted. The target is an original game whose **single vertical slice** reaches a comparable bar for battlefield density, presentation, weapon feel, and combined-arms atmosphere.

## Slice definition

The first battle is a rural European village assault with three sequential phases:

1. break the outer defensive line;
2. capture the village center;
3. survive a counterattack.

Target session length: 15–20 minutes.

## Mature slice content budget

- 1 battlefield.
- 2 infantry factions.
- 8–12 weapons.
- 4 core infantry roles.
- 30–60 simultaneously active combatants after optimization.
- 1 armored vehicle per side.
- sequential capture objectives.
- artillery, smoke, suppression, destruction, and reinforcement pressure.

## M0 — executable combat kernel

Implemented in the repository foundation:

- first-person locomotion;
- sprint, crouch, jump, and mouse look;
- automatic hitscan weapon;
- magazines and reloads;
- damage and death;
- hostile soldiers with movement, facing, line-of-sight checks, and burst fire;
- runtime battlefield blockout;
- directional light, skylight, atmospheric fog;
- wave spawning and scoring;
- code HUD;
- structural CI validation.

This milestone intentionally has no production art dependency. It proves the combat loop before expensive content enters the project.

## M1 — production weapon feel

- data-driven weapon definitions;
- animation-driven ADS;
- recoil curves and recovery;
- weapon inertia and procedural sway;
- camera impulse;
- rifle projectile ballistics;
- material-aware penetration;
- muzzle flash, smoke, shell ejection, impact VFX, decals, and spatial audio;
- per-surface impact response;
- weapon inspection/debug overlay.

## M2 — squad warfare

- squad membership and command state;
- cover queries;
- suppression and morale;
- target sharing;
- tactical intent;
- formation movement;
- flanking;
- grenade decisions;
- medic/ammo behaviors;
- reinforcement ownership.

## M3 — production battlefield

- authored terrain;
- Nanite environment set;
- dense foliage and debris;
- Lumen lighting pass;
- physically based material library;
- decals and battlefield damage;
- interiors;
- smoke volumes and atmospheric depth;
- production character meshes;
- locomotion and weapon animation sets.

## M4 — battle orchestration

- objective state machine;
- tickets and reinforcement waves;
- spawn-front movement;
- artillery;
- vehicle combat;
- squad replacement;
- victory/defeat flow;
- battle telemetry.

## M5 — optimization and shipping slice

- scalability tiers;
- Unreal Insights capture;
- AI significance manager;
- animation budget allocator;
- pooling where profiling justifies it;
- HLOD/streaming pass;
- packaged Windows build;
- fixed benchmark route and frame-time acceptance test.

## Acceptance bar

The vertical slice is ready only when all of the following are true:

- combat is fun without relying on spectacle;
- a 10-minute unedited capture looks coherent and production-quality;
- bots create credible pressure rather than simply rushing the player;
- no major gameplay system depends on editor-only manual intervention;
- the build can be reproduced from the repository plus documented content dependencies;
- performance targets are measured, not assumed.
