# Warrior's Paradise — Enlisted-Quality Vertical Slice

## Product target

One original WWII village battle, approximately 15–20 minutes, at a comparable quality bar for weapon feel, squad warfare, battlefield pressure and combined-arms atmosphere.

The target is **not an Enlisted clone**.

## Battle flow

1. break the outer defensive line;
2. capture the village center;
3. push into the inner village;
4. receive Allied armored support;
5. secure the final objective;
6. survive an infantry-and-armor counterattack.

## Code-side gameplay status

### Weapon / movement feel — implemented baseline

- physical projectile ballistics and gravity;
- ADS;
- stance/movement/suppression accuracy coupling;
- sprint stamina;
- crouch and collision-aware prone;
- weapon sway;
- recoil, recovery and camera impulse;
- suppression camera response;
- hit-location multipliers;
- thin-cover penetration;
- data-driven weapon definition assets;
- distinct automatic, semi-automatic and manual-action weapon behavior;
- Lee-Enfield/Kar98k bolt-action cycling with per-weapon action timing;
- dedicated manual-cycle presentation hooks/audio seam and HUD action-state feedback;
- magazines, reload/dry-fire/cancel events and resupply;
- melee;
- timed bandaging.

The code now includes production visual/loadout profiles, Motion Matching discovery, weapon/soldier presentation components, tracers, decals, surface-specific impacts and spatial-audio hooks. Final asset selection, skeleton retargeting and art-direction tuning remain content work.

### Squad warfare — implemented baseline

- explicit factions and four infantry roles;
- five AI squadmates plus the player;
- Follow / Attack / Hold orders;
- manual squad switching and death-driven control transfer;
- unified AI-capable player/squad pawn;
- Steady / Pressured / Pinned suppression states with hysteresis;
- suppression-driven AI accuracy degradation, aggressive cover-seeking and pinned return-fire limits;
- pinned player sprint lock plus explicit HUD state feedback;
- cover reservations;
- coordinated Base Of Fire / Maneuver behavior against shared visible targets;
- Assault-biased maneuver selection with Riflemen/other troops forming the fire base;
- maneuver initiation gated on recent live covering fire / target suppression, with continuous support reevaluation and pause-on-support-loss behavior;
- Assault maneuverers can deploy a lateral smoke screen after support stalls, verify deployment, and advance through a target-bound concealment window even after expending their final grenade;
- hidden maneuver follows a position frozen before the throw (no live target tracking); regression tests protect target retention, smoke expiry and actual under-smoke movement telemetry;
- maneuver candidates must be Steady and far enough from the target to execute the assigned flank;
- stance-aware cover usage: crouched protection/reload phases, timed exposure to fire, and pinned-prone behavior when no cover is available;
- fighting-cover scoring favors crouch-protected / stand-exposed peek geometry, safe approaches and stable retention over dead-end concealment or exposed pseudo-cover;
- stance-aware LOS/fire-lane/cover evaluation plus short reacquisition memory for concealed troops;
- friendly fire-lane safety corridor, immediate hold-fire behavior and exposed-shooter lane clearing;
- downed allies block AI firing corridors as physical casualties;
- frag deconfliction veto when friendlies are inside a 650 cm predicted-impact safety bubble;
- coordinated low-magazine reloads under same-target covering fire, with close-threat reload interruption;
- LOS + close awareness;
- location-based shared contacts;
- gunshot investigation;
- flanking;
- frag/smoke decisions;
- medic revive/smoke logic plus bleeding-ally stabilization;
- engineer ammunition support.

### Wounds / reinforcements — implemented

- active severity-scaled bleeding from traumatic or low-health wounds;
- bandaging stops bleeding before healing and is interrupted by new damage;
- conscious bleeding allies can be field-dressed by nearby squadmates/medics;
- downed/bleedout/final-death lifecycle;
- manual and AI revive;
- Allied replacement tickets;
- Axis reinforcement tickets;
- delayed replacement soldiers;
- safe spawn candidate selection;
- moving front-line spawn anchors;
- victory/defeat exhaustion logic.

### Objective / support layer — implemented

- three sequential capture points;
- combat-capable presence counting;
- contested force-balance state;
- reinforcement escalation;
- artillery with charges/cooldown;
- limited player air support with charges/cooldown;
- scripted strafe/bomb missions and ambient flybys;
- final counterattack;
- battle score, victory and defeat.

### Battlefield interaction / free environment — implemented baseline

- runtime village/cover blockout with enterable building collision shells;
- optional free rural-building visual overlays that do not replace gameplay collision;
- reservable cover points;
- penetrable thin cover;
- destructible cover with physics-debris fallback;
- tactical Local Fog Volume smoke used by visual and AI sight, with optional Niagara/audio overlay;
- data-driven dirt/plaster/brick/stone/wood/metal/roof/concrete PBR material slots;
- optional CC0 tree/crate/barrel dressing;
- sun/skylight/volumetric atmosphere with optional HDRI/cloud/ambience resources.

### Combined arms — implemented code baseline

- autonomous and **player-crewable tanks**;
- AP / HE tank ammunition;
- front/side/rear armor plus engine/track/turret/ammo-rack subsystem damage;
- cannon/coax combat and smoke-aware targeting;
- transport truck with squad embark/disembark;
- autonomous/player armored-car MG combat;
- usable HMG and anti-tank gun emplacements;
- autonomous Axis gun-position behavior with traverse limits;
- physical AT shells shared with tank combat;
- scripted/AI flyby, strafing and bombing missions;
- physical air bombs with infantry suppression and combined-arms blast damage;
- player air-support designation;
- objective-triggered vehicle/air escalation and final combined-arms counterattack;
- recurring ambient aircraft flybys;
- enemy heavy/light vehicle destruction integrated with victory accounting.

## Production presentation contract — implemented

The C++ layer exposes gameplay events **and** concrete data-driven presentation components/profiles for weapons, soldiers, smoke, explosions, tanks and environment content. The free-content generator discovers installed free resources and creates `DA_WP_BattlePresentation_Free`. See `docs/PRESENTATION_BINDINGS.md` and `docs/FREE_CONTENT_WORKFLOW.md`.

## Production content still required

The branch can now *consume* the major production categories, but the final coherent asset set is not committed/generated in this environment. Before “Enlisted-quality graphics/presentation” is justified we still need:

- install/acquire the chosen free Epic/Fab/Sonniss resources under their provider terms;
- final high-fidelity soldier/uniform/hands/weapon selection;
- UE5 retargeted Motion Matching/reload/action animation on the chosen skeleton;
- coherent period-correct vehicle presentation;
- authored village/terrain/interior/foliage composition beyond the runtime fallback;
- tuned Niagara/audio/decal choices rather than heuristic first matches;
- hero destruction/UMG polish and any missing custom art;
- real UE 5.8 compilation, PIE testing, packaged-build validation and performance tuning.

## Hard validation gate

The slice is not shipping-quality until all of the following are true:

- the UE 5.8 C++ project compiles cleanly;
- a packaged Windows build is reproducible;
- a 10-minute unedited play capture has no obvious simulation/UI breakage;
- bots create credible pressure without omniscient target tracking;
- squad switching, revive, reinforcement and vehicle state survive repeated playtests;
- objective completion cannot soft-lock;
- frame-time targets are measured using Unreal Insights;
- production assets are bound without changing core simulation rules.
