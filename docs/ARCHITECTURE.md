# Architecture

Warrior's Paradise is C++-first. Core battle rules stay reviewable in source; Blueprints, Data Assets, animation, VFX and audio bind onto explicit presentation seams.

## Runtime layers

### AWPSoldierCharacter

Owns player-capable soldier state:

- first-person camera/input;
- sprint/stamina, crouch and prone;
- guarded fire/reload input;
- melee;
- grenades/smoke;
- bandaging;
- squad commands/switch input;
- recoil/sway/suppression camera response;
- health/weapon/suppression components.

Every playable body uses the AI-capable subclass `AWPEnemySoldier`, so possession can move between squad bodies without leaving inert characters.

### UWPWeaponComponent / UWPWeaponDefinition

Own data-driven weapon simulation:

- cadence;
- magazine/reserve state;
- reload;
- ADS state;
- spread;
- muzzle velocity/range;
- recoil values;
- projectile spawn.

It exposes presentation events for fire, reload lifecycle, dry fire and projectile spawn.

### AWPBallisticProjectile

Owns physical bullet travel, gravity, damage, hit-location scaling, suppression and thin-cover penetration. Bullet impact is exposed as a presentation event.

### UWPHealthComponent / UWPSuppressionComponent

Health owns damage, partial healing, downed/bleedout/revive/final death and damage-source events.

Suppression owns accumulation/decay and a normalized presentation/AI signal.

### AWPEnemySoldier / AWPCoverPoint

The tactical infantry layer provides:

- LOS and smoke occlusion;
- close awareness;
- location-based shared contacts;
- gunshot investigation;
- cover reservation;
- suppression reactions;
- flanking;
- role behaviors;
- grenades/smoke;
- melee.

The AI intentionally does not receive a continuously updating hidden target pointer from squad memory.

### AWPBattlefieldDirector / AWPCaptureObjective

The director owns battle-scale authority:

- objectives and contested state;
- squad orders;
- contact/sound memory;
- tickets and replacements;
- spawn safety/front movement;
- artillery;
- vehicle deployment;
- score/victory/defeat;
- player possession transfers.

### AWPArmoredVehicle / AWPTankShell

Own combined-arms fallback simulation:

- vehicle movement and visible-target acquisition;
- objective advance;
- turret/cannon/coax;
- smoke occlusion;
- small-arms armor immunity;
- directional armor shell response;
- vehicle destruction.

### AWPDestructibleCover / smoke / artillery

World-interaction actors keep damage/destruction/support logic independent of art assets. Production Geometry Collections/Niagara/audio can replace presentation while preserving the same battle rules.

## Presentation and free-content boundary

Gameplay remains authoritative in C++, while production content is selected through Data Assets:

- `UWPBattlePresentationSet` maps faction/role loadouts, vehicles, environment, smoke and explosion profiles;
- `UWPInfantryLoadoutProfile` combines weapon definition, soldier visuals, weapon presentation and soldier-feedback presentation;
- `UWPInfantryVisualProfile` supports third-person bodies, first-person arms, static or skeletal weapon meshes, sockets and animation classes;
- `UWPWeaponPresentationComponent` handles muzzle/shot/reload, tracer, material-specific impact VFX/audio and decals;
- `UWPSoldierPresentationComponent` handles hit/downed/revive/death/melee/bandage VFX, spatial audio and optional montages;
- `UWPVehicleVisualProfile` binds tank meshes plus cannon/coax/impact/destruction presentation;
- `UWPEnvironmentVisualProfile` binds PBR surfaces, scenery, building overlays, HDRI/cloud and ambience resources;
- explosion/smoke profiles keep support-weapon presentation separate from damage/sight rules.

`AWPBattlefieldDirector` optionally loads `/Game/WarriorParadise/Profiles/DA_WP_BattlePresentation_Free`. If absent, the code-only fallback still runs.

The free-content toolchain is deliberately outside core simulation: acquisition/provenance → Unreal import → material generation → profile generation. Marketplace/sample assets never become a second source of gameplay truth.

See `PRESENTATION_BINDINGS.md` and `FREE_CONTENT_WORKFLOW.md`.

## Multiplayer posture

The current slice is single-player first. Authority boundaries are deliberately centralized in components/directors so later replication can be designed explicitly rather than scattering arbitrary replicated state through presentation Blueprints.
