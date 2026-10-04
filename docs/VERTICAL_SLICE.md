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
- magazines, reload/dry-fire/cancel events and resupply;
- melee;
- timed bandaging.

Production skeletal rigs, AnimBPs, authentic weapon data, material response, audio and VFX remain content work.

### Squad warfare — implemented baseline

- explicit factions and four infantry roles;
- five AI squadmates plus the player;
- Follow / Attack / Hold orders;
- manual squad switching and death-driven control transfer;
- unified AI-capable player/squad pawn;
- suppression and cover reservations;
- LOS + close awareness;
- location-based shared contacts;
- gunshot investigation;
- flanking;
- frag/smoke decisions;
- medic revive/smoke logic;
- engineer ammunition support.

### Wounds / reinforcements — implemented

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
- final counterattack;
- battle score, victory and defeat.

### Battlefield interaction — implemented fallback

- runtime village/cover blockout;
- reservable cover points;
- penetrable thin cover;
- destructible cover with physics-debris fallback;
- tactical Local Fog Volume smoke used by visual and AI sight;
- sun/skylight/volumetric atmosphere.

### Combined arms — implemented fallback

- one Allied and one Axis armored vehicle;
- autonomous movement/targeting;
- visible-target-only acquisition;
- objective advance when no target is visible;
- turret/cannon/coax behavior;
- tank shells;
- front/side/rear armor response;
- small-arms armor immunity;
- smoke-aware sight;
- vehicle destruction and victory accounting.

## Production presentation contract

The C++ layer now exposes events for the expensive presentation systems rather than hard-coding temporary effects into simulation. See `docs/PRESENTATION_BINDINGS.md`.

## Production content still required

These are necessary before the phrase “Enlisted-quality graphics/presentation” is justified:

- production skeletal characters, hands and weapons;
- authored animation sets and AnimBPs;
- authentic vehicles;
- high-density terrain/buildings/interiors/foliage;
- Nanite/HLOD/streaming setup where appropriate;
- PBR physical-material library;
- Niagara combat VFX;
- weapon/explosion/vehicle/voice/foley audio;
- decals, dirt, damage and battlefield dressing;
- authored navigation and cover pass;
- Chaos destruction for hero structures;
- UMG presentation polish.

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
