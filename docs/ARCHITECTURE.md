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
- automatic / semi-automatic / manual-action behavior;
- per-weapon bolt/pump cycle timing and cycle presentation events;
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

Health owns damage, partial healing, **active bleeding**, downed/bleedout/revive/final death and damage-source events. Severe hits or sufficiently low health can start a severity-scaled bleeding timer; field dressings stop that timer, while entering the downed state hands authority to the existing bleedout lifecycle so the two death-pressure systems never stack.

Suppression owns accumulation/decay plus a hysteretic state machine: **Steady → Pressured → Pinned**. The normalized value still drives camera/accuracy effects, while the discrete state prevents AI behavior from oscillating around a single threshold. Pinned AI abandons offensive movement, prioritizes/holds cover, degrades shot dispersion and only attempts sparse return fire from cover.

### AWPEnemySoldier / AWPCoverPoint

The tactical infantry layer provides:

- LOS and smoke occlusion;
- close awareness;
- location-based shared contacts;
- gunshot investigation;
- cover reservation;
- suppression-state reactions with cover priority, degraded accuracy and pinned return-fire limits;
- deterministic shared-target maneuver election;
- Base Of Fire and Maneuver tactical postures, with Assault role preference for the maneuver element;
- target-centered election so soldiers fighting the same contact converge on one mover rather than independently self-electing;
- fire-superiority gating: the elected mover remains in/near cover until either a suppressed target has at least one viable shooter that fired recently or an unsuppressed target has at least two same-target shooters that fired recently and still have loaded, clear lanes;
- maneuver election excludes Pressured/Pinned or already-too-close candidates so every elected mover can actually execute the assigned flank;
- support is continuously reevaluated, so a maneuver pauses if those covering conditions collapse;
- an Assault maneuver element that remains unsupported can fall back to a target-bound smoke tactic: it waits before committing a smoke grenade, throws laterally onto the planned maneuver corridor, waits through deployment, and advances only after an actual `AWPSmokeCloud` is detected near the intended screen;
- smoke authorization is contact-specific and time-bounded; a screen remains usable after throwing the last grenade, and an expired screen must be deliberately re-established;
- smoke retains the original contact during deployment and verified concealment without granting live hidden-target tracking: the maneuver waypoint is snapped at throw time, and movement under smoke follows only that frozen waypoint;
- headless telemetry counts smoke throws separately from verified under-smoke travel (over 140 cm from the throwing location);
- the corridor target is laterally offset from the direct shooter-target line to reduce the chance of blinding the base of fire while still covering the mover;
- cover occupation has an explicit posture cycle: protected crouch/reload phases alternate with brief standing firing windows, while pinned troops without reserved cover go prone;
- cover selection explicitly scores crouched protection, standing peek exposure, approach-route exposure, desired engagement range, authored cover quality and current-cover retention;
- the preferred fighting-cover profile is crouch-occluded / stand-visible: enough geometry to protect the soldier between bursts without turning the position into a dead-end firing hole;
- fully occluded cover receives a modest concealment penalty, fully exposed positions a large penalty, and an exposed approach receives an additional cost; a retention bonus reduces oscillation between nearly equivalent reserved points;
- a short cover-contact memory exists only to permit a hidden soldier to stand and reacquire; hidden target position is not continuously consumed for firing/movement;
- LOS, fire-lane and cover-exposure traces use actual pawn view positions so stance changes simulation geometry;
- body-width friendly fire-lane sweeps before every AI firing decision;
- immediate ceasefire when a friendly enters a live lane, with exposed shooters sidestepping to regain a safe lane while covered shooters hold position;
- downed friendlies remain physical fire-lane blockers;
- frag throws are vetoed when friendlies occupy a safety bubble extending beyond the grenade's lethal radius;
- tactical reloads are staggered under visible contact: a low-magazine soldier reloads only with same-target covering fire unless empty or pinned safely in cover;
- cover providers must be loaded, not reloading/cycling, unpinned, within range, on the same contact and have both LOS and a clear fire lane;
- close threats can interrupt a non-empty reload, preserving immediate self-defense;
- independent flanking as a fallback when coordinated tactics are unavailable;
- role behaviors, including medic prioritization of downed and bleeding allies;
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
- artillery and limited air support;
- heavy/light vehicle deployment;
- defensive emplacements;
- ambient/scripted aircraft missions;
- score/victory/defeat;
- player possession transfers.

### AWPArmoredVehicle / AWPTankShell

Own heavy-armor simulation:

- autonomous or player-crewed tank operation;
- hull movement, turret traverse/elevation, cannon and coax;
- AP / HE shell selection;
- directional armor;
- engine / track / turret / ammo-rack subsystem damage;
- mobility/turret degradation and catastrophic rack damage;
- smoke-aware target acquisition;
- combined-arms targeting of infantry, tanks, light vehicles and gun positions.

### AWPLightVehicle

Owns transport and light-vehicle gameplay:

- transport truck / armored-car configurations;
- player possession;
- nearby squad embark/disembark;
- emergency ejection;
- autonomous armored-car movement/targeting;
- smoke-aware MG combat.

### AWPGunEmplacement

Owns usable battlefield weapons:

- HMG and AT-gun configurations;
- player possession;
- autonomous unoccupied operation;
- constrained yaw/pitch traverse;
- smoke-aware acquisition;
- ballistic HMG and AP/HE AT fire.

### AWPAirSupportAircraft / AWPAirBomb

Own scripted/AI air missions:

- flyby / strafe / bombing mission modes;
- real moving aircraft actors;
- ballistic strafing projectiles;
- physical falling bombs;
- shared infantry suppression and combined-arms blast damage.

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
- `UWPLightVehicleVisualProfile`, `UWPEmplacementVisualProfile` and `UWPAirSupportVisualProfile` bind transport, gun-position and aircraft presentation;
- `UWPEnvironmentVisualProfile` binds PBR surfaces, scenery, building overlays, HDRI/cloud and ambience resources;
- explosion/smoke profiles keep support-weapon presentation separate from damage/sight rules.

`AWPBattlefieldDirector` optionally loads `/Game/WarriorParadise/Profiles/DA_WP_BattlePresentation_Free`. If absent, the code-only fallback still runs.

The free-content toolchain is deliberately outside core simulation: acquisition/provenance → Unreal import → material generation → profile generation. Marketplace/sample assets never become a second source of gameplay truth.

See `PRESENTATION_BINDINGS.md` and `FREE_CONTENT_WORKFLOW.md`.

## Multiplayer posture

The current slice is single-player first. Authority boundaries are deliberately centralized in components/directors so later replication can be designed explicitly rather than scattering arbitrary replicated state through presentation Blueprints.
