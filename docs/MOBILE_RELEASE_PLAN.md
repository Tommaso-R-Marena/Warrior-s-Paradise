# Warrior's Paradise — Mobile release and quality strategy

Status: **browser mobile build implemented and deployed; native UE 5.8 Android/iOS builds NOT yet compiled or device-tested.**

## The reference target

World War Heroes is a mature 3D multiplayer WWII mobile FPS with numerous weapons, vehicles, maps, modes and a large content pipeline. It is a *benchmark*, not a level of parity already delivered by this repository.

The Field Edition is a smaller single-player 2.5D raycasting game: its goals are touch usability, offline play and near-zero hosting cost. It cannot reach contemporary full 3D shooter fidelity from its current Canvas 2D architecture alone. To pursue comparable graphics, ship a native Unreal mobile renderer or a purpose-built WebGL2/3D scene with authored licensed models, animations, materials, baked lighting and calibrated device LODs.

## Changes already in this repository

**Browser edition**

- Procedurally textured stone/wood cover, atmospheric sky, distant ridges, trees/crates, soldier textures and improved first-person gun.
- Lee-Enfield, Sten and Bren role loadouts with differing damage, spread, firing/reload rates and magazine sizes.
- Touch ADS, frags, smoke (including line-of-sight blocking), tactical follow/hold orders, casualty switching, enemy suppression and cached grid waypoint navigation.
- Three selectable battlefields, Assault and five-wave Last Stand, Recruit/Regular/Veteran bots; perspective-sampled terrain and more mature line-of-sight AI with finite ammo and combat medics.
- User-selectable ECO, Balanced, Sharp and Ultra tiers with adaptive internal resolution; two-axis camera look, custom aiming sensitivity, hit-zone approximation, synthesized gunshot layers and touch haptics. No downloaded JS libraries or paid backend.
- PWA network-first caching with offline fallback.
- `node tools/tests/browser_runtime_smoke.js` tests initialization, ADS/shoot/reload/grenades/squad control, vertical aim, map/mode combinations and survival victory in portrait and landscape. Source contract tests are in `tools/tests/test_browser_field.py`.

**Native UE Android prep**

- `Config/Android/AndroidEngine.ini` overrides the desktop renderer for Android with Mobile Forward, traditional shadows, no desktop Lumen, Nanite dependency or Virtual Shadow Maps.
- `Config/DefaultDeviceProfiles.ini` assigns conservative texture, view distance, effects and render scale targets to standard `Android_Low`, `Android_Mid`, `Android_High` classes, retaining desktop graphics separately.
- These are **proposed settings awaiting UE/UHT/Android packaging**, and device-profile inheritance/cook output must be inspected on first engine run.

## First native Android acceptance gates (not yet completed)

1. On an Unreal-ready Windows machine, install UE 5.8, Android SDK/NDK/JDK via Epic's Turnkey tool. Build a clean `WarriorParadiseEditor Win64 Development` target first.
2. Inspect Android Project Settings for ARM64, package identifiers, shipping signing and supported rendering API (ES3.2/Vulkan compatibility); use Mobile Forward, not the experimental Android desktop renderer, for initial shipments.
3. Create a native touch UMG HUD / virtual joystick with configurable sensitivity, ADS, fire, reload, frag/smoke, stance, interaction and squad switching. The existing desktop keyboard controller bindings do not by themselves make the UE project mobile-playable.
4. Cook/package an ARM64 Android Development build. Identify desktop-only shaders, materials, Niagara emitters, Python editor plugins and imported meshes that break mobile cook; replace with mobile LODs and mobile-friendly materials.
5. Profile 3 tiers across 2–3 actual Android devices. Aspirational targets (NOT measurements): 30 fps at reduced internal resolution for Low, 30–45 fps Mid, 60 fps High under crowded combat. Track thermal throttling, memory peaks, frame variance and crashes.
6. Test genuine battle with several AI squads, ballistics, smoke, tanks, emplacements, touch controls, map traversal and 20 minutes sustained combat. Fix regressions before claiming a public native build.
7. iOS requires compatible Apple hardware/signing/Xcode and iPhone device testing; the mobile browser edition remains the zero-install alternative.

## Content and gameplay parity roadmap

**Priority A — make one excellent map:** authored WWII town/farm scene, real cover geometry, atmospheric baked mobile lighting, foliage occlusion, detailed architecture, destruction LODs and measured performance.

**Priority B — combat feel:** professionally rigged infantry animations, hit reactions, weapon audio and sights, predictable recoil, reload transitions, improved grenade throws, fluid camera movement and custom touch layouts.

**Priority C — meaningful tactics:** squad AI navigation/cover usage, class roles, command wheel, suppression, medics/revives, vehicle infantry interplay and fair objective modes.

**Priority D — expand after baseline proof:** more maps and modes, tank control, cosmetics and progression, optional online multiplayer with an authoritative backend. Real-time multiplayer servers and streamed UE rendering are *not* provided by free static Pages.

## Hosting reality

- Cloudflare Pages hosts the static browser edition; all physics and rendering run on players' devices. No server-side GPU is provisioned.
- Native Android builds can eventually be distributed through controlled direct APK testing before store submission. App-store publication/signing/fees are separate issues.
- Browser fidelity/performance and native fidelity/performance need **real device benchmarks** before making comparative claims.

## Quantified parity scorecard

See [MOBILE_PARITY_SCORECARD.md](MOBILE_PARITY_SCORECARD.md) for a truthful feature-by-feature comparison against the advertised commercial game and the first five engineering/QA acceptance gates. The high-fidelity native Android path is distinct from the offline browser edition.
