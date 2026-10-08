# Warrior's Paradise

**Warrior's Paradise** is an Unreal Engine 5.8 WWII first-person squad-combat project targeting one **Enlisted-quality vertical slice** before expanding content.

The repository now contains a substantial code-side combined-arms battle rather than a simple wave shooter.

## Play on a phone or low-power laptop — Field Edition

**Playable browser edition:** [warriors-paradise-field.pages.dev](https://warriors-paradise-field.pages.dev/) (separate free-hosted static skirmish; no Unreal download or dedicated GPU).

A smaller, fully client-side browser adaptation lives in `browser/`. The latest major upgrade adds **three selectable battlefields (Normandy Farm / Occupied Town / Forest Outpost)**, **Assault or five-wave Last Stand**, Recruit/Regular/Veteran AI, perspective-sampled roads/grass/mud, detailed procedural stone/timber/soldier art, three distinct WWII weapons, two-axis mouse/touch aiming with sensitivity settings, ADS, smoke/frags, finite-magazine combatants, allied medic care, battlefield orders, synthetic weapon audio, touch haptics and an adaptive **ECO/BALANCED/SHARP/ULTRA** renderer. All gameplay runs on-device; no dedicated GPU, Unreal installation or paid server required. This remains a **2.5D** browser shooter, not the complete Unreal 3D game or graphics/multiplayer parity with a mature mobile title. Real phone performance is not yet measured. See the [mobile parity scorecard](docs/MOBILE_PARITY_SCORECARD.md).

Play locally from the repository by opening `browser/index.html`; or with Python 3 run `python tools/serve_field_edition.py` and visit `http://127.0.0.1:8000/`. Use `--lan` to play on a phone on the same trusted Wi-Fi. Full instructions: [browser/README.md](browser/README.md).

The UE5.8 game remains the higher-fidelity native project below; it requires an actual UE build and suitable rendering hardware (integrated graphics may work at Minimal, but has not yet been benchmarked). Android Mobile Forward overrides and Android Low/Mid/High device profiles are now staged under `Config/`; native packaging, mobile touch HUD and device profiling remain open (see [mobile release plan](docs/MOBILE_RELEASE_PLAN.md)).

## Native 3D Android development — commercial-quality target

The real Unreal Engine 5.8 project now includes a **native multitouch HUD and input controller** for infantry, tanks, light vehicles and gun emplacements. It has 13 context-sensitive infantry actions, twin-thumb movement/look, tank turret and cannon/MG controls, an Android ARM64 landscape setup, mobile-specific rendering profile handling and a dedicated Android packaging/preflight script.

From a configured UE5.8 Windows machine:

```powershell
.\tools\package_android.ps1 -ValidateOnly
.\tools\package_android.ps1 -TextureFormat ETC2 -Configuration Development
```

**Current hard limitation:** the GitHub repository still has **zero checked-in Unreal maps/assets** (`.umap`/`.uasset`), 3D models, audio or textures. Its visuals remain generated primitive stand-ins until the existing licensed-asset import system is populated. This source has **not** yet been compiled, cooked or tested on a phone. A native APK or commercial-grade presentation is *not* ready.

See [native Android production and build guide](docs/NATIVE_ANDROID_PRODUCTION.md), [art inventory audit](tools/assets/audit_mobile_content.py), and [mobile parity scorecard](docs/MOBILE_PARITY_SCORECARD.md) for the exact graphics, animation, gameplay and quality gates required before parity can be claimed. The separately hosted browser Field Edition is a deliberately smaller 2.5D game, not this native mobile build.

**New art-to-map workflow:** run `tools/prepare_native_mobile.ps1` on a UE5.8 machine. It generates **eight original deterministic low-poly 3D scenery meshes** (trees, bushes, rubble, crates, fences, barrels, stumps and sandbags), plus **nine seamless base-color/normal/roughness material sets** for roads, mud, grass, brick, stone, plaster, roofs, timber and metal. It checks locally imported files against source/license provenance, imports assets, builds Unreal PBR materials and presentation profiles, and saves `/Game/WarriorParadise/Maps/WP_NormandyMobile`. Additional licensed models and textures are still needed for commercial visual quality. After inspecting the saved level, run `python tools/assets/activate_mobile_map.py --apply`. The generated meshes and textures are **starter art**, not production-quality infantry, animation, or a finished mobile shooter; this pipeline has not yet run successfully inside Unreal here.

**Android release checks:** Android SDK, ARM64 and landscape settings now live in `Config/Android/AndroidEngine.ini`, where Unreal consumes Engine-category Android settings. After producing an APK, run `tools/android_device_smoke.ps1 -ApkPath <your-built-APK>` on a connected Android phone for an early launch/process-survival check. This is not an FPS or graphics-quality benchmark. Imported Unreal asset binaries use Git LFS; initialize LFS and check hosting quota before committing large art files.

## Implemented gameplay

### Infantry and weapon feel

- first-person movement, sprint, crouch, **true prone stance**, jump and stamina;
- ADS with FOV interpolation;
- physical projectiles with gravity and configurable muzzle velocity;
- movement/stance/suppression-dependent spread;
- player suppression states feed recoil/sway/HUD feedback and prevent sprinting while fully pinned;
- procedural weapon sway, recoil, recovery and camera impulse;
- skeletal-bone hit multipliers ready for production character rigs;
- thin-cover penetration with retained velocity/damage;
- data-driven weapon definitions through `UWPWeaponDefinition`;
- automatic, semi-automatic and manual-action weapon mechanics;
- authentic Lee-Enfield/Kar98k bolt cycling with per-weapon cycle timing, HUD state and presentation hooks;
- magazines, reload timing, dry fire, reload cancellation and engineer resupply;
- melee combat;
- frag grenades and tactical smoke grenades;
- traumatic and low-health hits can cause active bleeding with severity-scaled health loss;
- bandaging stops bleeding before restoring health and is interrupted by fresh damage;
- nearby conscious bleeding allies can be field-dressed with `E`;
- timed bandaging/field medicine.

### Wounds and squad lives

- health, active bleeding, downed state, bleedout, finishing damage and revive;
- medics prioritize downed soldiers and can stabilize conscious bleeding allies;
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
- explicit **Steady / Pressured / Pinned** suppression states with hysteresis;
- suppression degrades AI shot dispersion as well as fire confidence;
- pressured troops seek cover sooner; pinned troops break contact, stop exposed firing and make only sparse return fire from cover;
- deterministic **base-of-fire / maneuver** coordination around shared visible targets;
- Assault troops are preferred as the maneuver element while Riflemen anchor covering fire;
- maneuver troops flank toward a standoff position while base-of-fire troops seek/hold cover and increase firing pressure;
- elected maneuver troops now wait for **fire superiority** before leaving cover: either the target must already be Pressured/Pinned with at least one viable shooter delivering recent covering fire, or at least two same-target shooters must have recently fired with loaded, clear lanes;
- only Steady, range-valid troops can be elected as the maneuver element, preventing a pressured or already-too-close soldier from deadlocking the fireteam plan;
- if covering support collapses mid-maneuver, the mover pauses instead of continuing a solo flank;
- when an elected **Assault** flanker cannot establish enough fire superiority, it can deploy smoke laterally onto the maneuver corridor rather than charging unsupported;
- smoke-assisted movement waits for grenade deployment and verifies a real smoke cloud exists near the intended screen; throwing the soldier's **last** smoke grenade no longer disables the already-active screen;
- while smoke obscures the enemy, the flanker retains its original contact only for the bounded screen window and moves toward a **frozen waypoint planned before the throw**, never chasing an updated hidden enemy position;
- smoke timing is bound to the specific original contact, and the headless test distinguishes smoke grenades thrown from soldiers that actually advanced at least 140 cm under smoke;
- the smoke screen is placed off the direct target line so the base of fire can retain its lane whenever geometry permits;
- reserved-cover troops now use actual stance cycles: crouch while protected/reloading/pinned, rise for short firing windows, and drop back down between exposures;
- cover selection now distinguishes **fighting cover** from mere concealment: it rewards positions that hide a crouched soldier but expose a standing firing line, penalizes uncovered approaches, and mildly favors retaining a good current position to reduce cover thrashing;
- fully concealed/dead-end positions remain usable when necessary, but score below peek-capable cover; exposed pseudo-cover receives a heavy penalty;
- pinned troops without cover go prone and crawl away from the threat instead of remaining upright in the open;
- AI LOS, fire-lane checks and cover evaluation now use stance-aware pawn view locations, so crouching/prone materially changes exposure instead of being cosmetic;
- short cover-contact memory lets a crouched soldier rise to reacquire a recently seen target without granting continuous hidden-position tracking;
- AI checks a body-width safety corridor before firing, immediately holds fire when a friendly crosses the lane, and sidesteps to reopen the lane when exposed;
- downed friendlies also block AI fire lanes, preventing covering fire from being dumped through casualties;
- AI vetoes frag throws when any friendly is within a 650 cm safety bubble around the predicted impact, protecting maneuver troops and casualties from friendly grenades;
- low-magazine AI performs tactical reloads only when contact is lost, when pinned safely in cover, or when a nearby same-target teammate has ammunition, LOS and a clear fire lane to provide cover;
- close visible threats interrupt non-empty tactical reloads so soldiers do not die finishing an unnecessary magazine change;
- basic independent lateral flanking remains as a fallback when coordinated tactics are unavailable;
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
| E | Revive/treat ally / use friendly tank, vehicle or emplacement |
| X | Switch to next living squadmate |
| 1 | Squad Follow |
| 2 | Squad Attack objective |
| 3 | Squad Hold |
| T | Designate artillery |
| Y | Designate air support |
| F10 | Cycle Minimal / Low / Medium / High / Ultra graphics |
| F9 | Auto-detect graphics from hardware benchmark |

## Graphics scalability

Warrior's Paradise now treats performance tiers as a core product requirement rather than a post-launch optimization.

- **Auto** runs UE 5.8's hardware benchmark once on first launch and stores its per-group recommendation.
- **Minimal / Integrated** targets machines without a discrete GPU: UE Low scalability, 50% internal resolution, 30 FPS, Lumen GI/reflections off, virtual shadow maps off, volumetric clouds off and FXAA.
- **Low** targets performance-first older dedicated GPUs at 67% internal resolution and UE Low rendering groups.
- **Medium** targets mainstream PCs at 78%.
- **High** is the main high-fidelity/60-FPS design target at 90%; UE 5.8's High Lumen tier is designed around a 60-FPS budget.
- **Ultra** uses Unreal Epic-quality groups at 100% internal resolution for high-end systems.

Low deliberately retains partial foliage density so quality settings do not create a gameplay concealment advantage. Core ballistics, cover, smoke occlusion, AI, armor and objective simulation are unchanged across presets.

See [Scalability and Performance Presets](docs/SCALABILITY.md).

## GPU-less simulation

Warrior's Paradise now has a true renderless development/test path using Unreal Engine's NullRHI. It keeps the battlefield simulation, AI, objectives, ballistics, vehicles, emplacements and combat state running while skipping presentation-only work.

```powershell
.\tools\build_local.ps1 -Clean -HeadlessSmoke
```

This launches UE 5.8 with `-nullrhi`, runs a timed gameplay smoke contract and returns a process-level PASS/FAIL code. The self-hosted Unreal compile workflow runs the same smoke test after compilation.

For a visually playable laptop with integrated graphics but no discrete GPU:

```powershell
.\tools\build_local.ps1 -LaunchIntegrated
```

A truly GPU-less run is intentionally non-visual; visual gameplay still requires some compatible graphics adapter. See [GPU-less and Integrated Graphics Modes](docs/GPU_LESS_MODE.md).

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

The validator now checks the **73-file** source surface plus Python tooling, plugin requirements, free-asset/license metadata, smoke/soldier/weapon/vehicle presentation, enterable buildings, material-specific impacts, tracers/decals and the free-content bootstrap contract.

`main` now includes a manual **UE 5.8 compile gate** at `.github/workflows/unreal-compile.yml`. It is designed for a Windows x64 self-hosted GitHub Actions runner so it does not consume GitHub-hosted Actions minutes. The gate runs repository validation, removes project build intermediates when requested, builds both `WarriorParadiseEditor` and the `WarriorParadise` Win64 game target, runs a GPU-less NullRHI gameplay smoke test, prints Unreal Build Tool diagnostics on failure, and publishes a PASS/FAIL job summary. A **full Unreal C++ compile/playtest has still not been executed**; connecting a Windows runner with UE 5.8 installed is the next hard validation gate.

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
