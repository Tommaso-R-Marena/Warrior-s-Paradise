# Mobile Shooter Quality — Parity Scorecard

Date: 2026-10-08

Reference: [World War Heroes — official Google Play listing](https://play.google.com/store/apps/details?id=com.gamedevltd.wwh). Benchmark *publicly advertised feature scope*, not copied assets, branding or code. The reference advertises 12+ warzones, 100+ weapons, multiple multiplayer modes, playable tanks and commercial 3D graphics. It represents an existing large-scale production; our status below is strictly based on this repository.

## Reality check

| Area | Benchmark | Browser Field Edition currently | Native UE project | Acceptance needed |
|---|---|---|---|---|
| Graphics | Authored, full 3D mobile FPS | **2.5D** textured raycast walls, perspective ground, billboards, atmospheric effects; no skeletal mesh rendering | 3D combat/vehicle architecture with optional visual profiles; actual UE5.8 cook/playtest **not run** | Real mobile-ready meshes, LODs, animation, high-quality materials, baked/probed light, full 3D camera and shadows |
| Battlefields | 12+ advertised | **3** distinct authored tile layouts (Normandy Farm, Occupied Town, Forest Outpost) | Vertical-slice environment/fallback | One polished, genuinely navigable 3D map first; expand after device profiling |
| Modes | Multiple PvP modes | **2 offline modes**: three-sector Assault and 5-wave Last Stand | Single-player capture | Ship quality single-player modes, then separate multiplayer design |
| Weapons | 100+ types advertised | **3 role weapons**: Lee-Enfield, Sten, Bren, with ballistics/recoil/ammo | More ambitious configurable combat components | Prioritize 6–10 historically coherent fully animated/high-quality firearms before a shallow 100-weapon catalog |
| Vehicles | Player-operable tanks | Not implemented in browser | Tank/light vehicle/emplacements code exists; no UE compile verification | Drivable tank and usable emplacements on-device |
| Multiplayer | Competitive online | No backend, intentionally local-only | Deferred | Requires authoritative network server, latency and security tests; not provided by static hosting |
| AI | PvP opponents in reference | Offline squad and enemy AI; finite ammo, reloads, strafing, suppression, investigation, medic, waypoint navigation | More sophisticated coordinated tactical AI source | Test AI navigation, friendly fire, cover, reaction fairness in actual UE |
| Controls | Mobile shooter touch system | Move and look joysticks/drag, ADS, fire, heal, grenade, squad orders, adjustable sensitivity and vertical look | Touch HUD not yet built | Full customizable/repositionable native HUD, sensitivity curves, control calibration |
| Performance | Claimed optimization across devices | ECO/Balanced/Sharp/Ultra with dynamic render scaling; **no device frame-time results** | Android forward shading and tier presets staged | Measure FPS, p95 frame time, memory, temperature, crashes, input latency across real devices |
| Offline/free access | App downloads / online sessions | Free HTTPS static Cloudflare Pages; client-only play and cache fallback | Native builds still require packaging | Verify offline install on iOS/Android, network disconnect, fresh install and update |

## What changed in this push

- Introduced perspective-sampled mud/grass/road ground plane to replace mostly flat-colored surfaces.
- Added three selectable battlefield layouts, enemy placements, thematic scenery and difficulty modes.
- Added five-wave Last Stand with escalating enemy count and ammo/health/grenade resupply.
- Added magazine-limited squad and enemy weapons, medic healing, target memory without unseen-position cheating, strafing/suppression behavior and follow/hold fireteams.
- Added composite procedural weapon-report audio, camera shake, touch haptics, mouse/touch vertical look, aim-sensitivity selector and an approximate vertical target hit region.
- Added the optional Ultra graphics tier and capped dynamic-resolution scaling.
- Expanded Node/VM smoke checks to cover portrait/landscape, maps, modes, input, and a complete five-wave victory cycle.

## Critical distinction: parity needs a new renderer and content pipeline

Field Edition is a browser-compatible **software 2.5D game**, not a complete commercial-quality 3D engine. Procedural raycast textures and static billboards can make it more pleasant and playable, but they cannot produce World War Heroes–style 3D character animation, complex vertical architecture and live tank gameplay. Further investments in this rendering architecture have diminishing returns.

The preferred high-fidelity track is **Unreal Engine 5.8 mobile** with efficient Mobile Forward/Deferred and authored, legally usable assets. Experiment with high-end Vulkan only after the regular mobile renderer has been proven. Do not make Lumen/desktop Android rendering a shipping requirement; Epic calls Lumen on Android experimental.

## Objective milestone gates

**G1 — dependable browser release:** real visual QA on recent Android Chrome, iPhone Safari and an integrated-graphics laptop, with legible controls and no blocking input bugs. Record FPS/thermal/battery for a ten-minute session. Source regression tests alone do not satisfy this gate.

**G2 — first native Android build:** successful UE5.8 ARM64 package launches into a controllable first-person map with six functional inputs and verified device-profile selection; no crashes or missing shader materials.

**G3 — mobile art benchmark:** a 3D farm/town map with PBR textures, multiple soldier rigs/animations, consistent gun sight and audio, three types of cover, atmospheric lighting and measured budgets. Pass a sustained 15-minute session at 30 fps (including a p95 frame time under 50 ms) on a nominated mid-range phone.

**G4 — gameplay benchmark:** 3 class-based weapons with animation/sound, usable capture mode, 10–15 engaging bots, tested suppression/medic roles, enemy fairness, bullets/smoke/grenades, integrated cover navigation.

**G5 — feature expansion:** add tank support, map variants and additional guns only after G2–G4 are proven. Multiplayer requires a separately planned backend; static Pages cannot provide real authoritative PvP hosting.

A feature or performance target above is a **plan**, not a benchmark already passed.
