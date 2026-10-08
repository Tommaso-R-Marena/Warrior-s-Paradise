# Warrior's Paradise — Native 3D Mobile Production Track

**Status: implementation foundation, not a finished World War Heroes–quality game.**
Last updated: October 8, 2026.

## What is implemented in UE source

- First-person infantry combat with physical bullets, historically oriented loadouts, ADS, recoil, reloads, stance/prone/sprint, stamina, medkits, frags and smoke.
- AI teammates, squad orders, enemy tactical cover/fire, suppression, capture objectives, vehicles, tanks, mounted machine guns and air support.
- Asset-profile and editor import pipelines for replacing procedural visuals with licensed character/weapon/vehicle art.
- **Native mobile touchscreen controls** driven by `AWPPlayerController` and `AWPCombatHUD` with one authoritative `WPMobileControls.h` touch layout.
- Responsive multi-finger movement/look/fire, ADS, reload, grenades, health care, crouch, prone, squad switching, orders and interaction; vehicle steering, tank turret/gun aiming, cannon, MG, shell switching, exit, and mounted-emplacement firing.
- Android ARM64 Landscape / Mobile Forward configuration, tiered low/mid/high rendering profiles, mobile-specific graphics settings that do **not** execute the desktop benchmark.
- Development-only Android packaging wrapper and imported-content inventory gate.

## Hard blockers to a commercial-quality native release

The checked-in repository currently contains **zero** `.uasset` / `.umap` files. It has source code and Python art pipelines, not a packaged, artistically complete world. This must be addressed with appropriate licenses, asset files, rigged animation, high-quality PBR materials, mobile meshes/LODs, Niagara, physical audio, and real map lighting.

**None of the native UE gameplay, touch or Android packaging changes have received a successful UE 5.8 compile, Android cook or device test.** Do not label the project production-ready or store-ready.

Run an actual inventory after asset import:

```powershell
python tools/assets/audit_mobile_content.py
python tools/assets/audit_mobile_content.py --json
python tools/assets/audit_mobile_content.py --strict
```

The `--strict` check is intentionally expected to fail until the art inventory is populated, and cannot independently judge the quality or licenses of imported models. The source assets should be introduced through `tools/setup_free_content.ps1`, `Content/Python/wp_import_external_assets.py` and the existing presentation binding profile workflow. Follow `docs/THIRD_PARTY_ASSETS.md` and keep the source/license lock.

## Original mobile starter scenery (new)

`tools/assets/generate_original_props.py` creates **eight actual, deterministic OBJ meshes**, each with a basic material palette: trees, bushes/hedges, rubble, wooden crates, fences, barrels, stumps and stacked sandbags. `tools/assets/generate_original_materials.py` additionally creates **nine tileable RGB/normal/roughness sets** for ground mud, grass, gravel, masonry, brick, plaster, old timber, roof tile and rusted metal. The Unreal material builder now supports the original texture folders alongside Poly Haven. These assets are original code-generated starter art, not photorealistic soldiers, vehicles or commercial parity.

```powershell
python tools/assets/generate_original_props.py
python tools/assets/generate_original_materials.py
python tools/assets/validate_asset_provenance.py
```

Both generators populate `ExternalAssets/wp_original/` and merge provenance records and file checksums into `ExternalAssets/asset-lock.json`. Existing unrelated records are kept. Re-running is deterministic; previously hand-edited generated meshes or textures are **not silently overwritten**. The integrity check rejects changes, symlinks, escaping paths and untracked importable files. Normal and roughness source textures are generated as standards-compliant PNGs with edge-matched repeating boundaries.

`tools/prepare_native_mobile.ps1` now runs original-art generation and validation automatically before launching Unreal. Newly obtained manual art also needs explicit lock entries containing license information, an HTTPS source URL, a relative file path, file size and a SHA-256 or MD5 digest. This **technical** validation cannot independently determine whether a third-party license actually permits use, so licenses must still be reviewed.

## End-to-end licensed 3D level assembly (new)

The editor pipeline now prepares a real Unreal **Normandy mobile map** from locally obtained, license-tracked environmental meshes. The generator is non-destructive: it refuses to replace an existing map, leaves the actual battlefield's collision, spawn points, objectives and lighting in the runtime `AWPBattlefieldDirector`, and places decorative imported meshes around the playable combat routes.

1. Start with the generated original meshes, or add higher-quality CC0/free models, textures and environmental sounds under their actual licenses. The existing tool supports `python tools/assets/bootstrap_free_assets.py --polyhaven --resolution 2k` and `--direct-cc0` as appropriate, and writes `ExternalAssets/asset-lock.json`. Poly Haven's CC0 license permits commercial use; other Fab/Epic content may have different redistribution conditions.
2. On a Windows machine with **Unreal 5.8 compiled successfully**, run:

   ```powershell
   .\tools\prepare_native_mobile.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.8"
   ```

   This creates the project-original meshes, verifies every source file, and runs the Unreal Editor Python workflow: imported source files → PBR CC0 materials → existing presentation bindings → saved `/Game/WarriorParadise/Maps/WP_NormandyMobile`. It is designed to operate without third-party model downloads for a low-poly test map. **It has not been executed in a real Unreal 5.8 editor here**, and still lacks production soldiers, vehicle art, animations and professional materials.

3. Open the generated level in Unreal Editor and correct mesh scale, pivot placement, material assignments, animation references, mobile shader/collision bugs and lighting as necessary. `WP_ALLOW_BLOCKOUT=1` is **only** for a deliberately incomplete no-art experiment.
4. Preview the level activation:

   ```powershell
   python tools/assets/activate_mobile_map.py
   ```

   Once inspected and ready to use as the startup map:

   ```powershell
   python tools/assets/activate_mobile_map.py --apply
   ```

   For a production art gate add `--require-production-art`. `--reset --apply` restores Unreal's Entry map if needed. Commit the actual `.umap` through Git LFS after evaluating licensing and packaging.
5. Use `tools/package_android.ps1` to attempt a Development cook and install on a physical Android device. The production-content audit and independent QA remain separate acceptance gates.

**Unreal Editor Python scripts are source-complete but have not executed in UE5.8 here.** Editor API/property compatibility and specific mesh imports must be verified in the real editor.

## How to attempt a real Android development package

**Prerequisite:** Windows computer with a working UE 5.8 installation, supported Android SDK/NDK/JDK through **UE Turnkey**, USB-connected Android device (USB debugging), and enough storage/RAM for Unreal's cook and shader compilation. In Unreal's Project Settings → Platforms → Android, use **Configure Now**, accept SDK licenses and verify the intended package name.

With PowerShell from the repository root, run:

```powershell
.\tools\package_android.ps1 -ValidateOnly
.\tools\package_android.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.8" -TextureFormat ETC2 -Configuration Development
```

The first command runs Python contract tests, source audit, regressions and a non-strict inventory without building Unreal. The second invokes `RunUAT.bat BuildCookRun` for Android ARM64 and archives the result in `Saved/AndroidBuilds/ETC2` by default.

For a compatible high-end phone, `-TextureFormat ASTC` can provide a different compressed texture target. Never claim that either package succeeds until AutomationTool exits with zero and the APK actually installs and runs.

**Shipping gate:**

```powershell
.\tools\package_android.ps1 -Configuration Shipping -TextureFormat ASTC -RequireProductionArt
```

Shipping additionally requires a real package ID you control, signing credentials, Google Play requirements and source/license QA. The checked-in `com.example.warriorsparadise` package name is **development-only** and must be replaced. The strict art inventory gate stops empty-content Shipping builds, but does not certify artistic quality.

## Physical Android launch and early stability verification

After Unreal has compiled/cooked a real Android Development build, connect a phone through USB debugging and use Android SDK platform-tools:

```powershell
.\tools\android_device_smoke.ps1 -ApkPath "Saved\AndroidBuilds\ETC2\YourActualBuild.apk" -PackageName "com.example.warriorsparadise" -DurationSeconds 90
```

Provide the **actual** produced APK path and package name. The script installs the app using adb, launches it using Android's launcher intent (without hardcoding an Unreal Java activity name), samples the process for early exits and writes JSON diagnostics plus logcat to `Saved/AndroidDeviceSmoke/`. On a machine with several connected phones, supply `-Serial`. If the game closes unexpectedly, the test fails. Generic unrelated device-wide crash lines are only warnings, not proof that this game crashed.

**This is not an FPS, shader correctness, UI or thermal benchmark.** A successful launch must be followed by human gameplay tests and a sustained profiling session on representative Android hardware.

## Native mobile control scheme

| Gesture/button | Infantry | Tank | Transport / armored car | Mounted gun |
|---|---|---|---|---|
| Left drag | Walk / strafe | Drive / steer hull | Drive / steer | Slew left/right |
| Right drag | Yaw / pitch camera | Turret yaw / gun elevation | Steer camera/vehicle | Gun yaw / elevation |
| FIRE | Fire personal weapon (held) | Fire cannon (tap) | Fire MG (held when equipped) | Fire emplacement (held) |
| ADS | Hold to aim | Hold machine gun | — | — |
| RLD | Reload | Switch AP / HE | — | — |
| USE | Interact/revive/enter vehicle | Exit tank | Exit vehicle | Exit emplacement |
| Other buttons | Frag, smoke, healing, stance, jump, sprint, squad orders | Hidden | Hidden | Hidden |

The same normalized button rectangle table renders the HUD and resolves hit events. Actions preserve native ammo/cooldowns, not separate simplified weapon rules. The stock Unreal virtual joystick is disabled to prevent duplicate overlays.

**Still needed:** device-specific ergonomic iteration, safe-area handling around cutouts, gamepad accessibility, mobile main/pause menus, touch-rebinding, gyroscope option, input latency profiling, and vehicle control QA.

## Production visual quality target

A credible comparison with World War Heroes requires at least one map with:

1. **Art:** modeled/detailed farmhouses, streets, interiors, trenches, foliage, signage and clutter, backed by clean mobile PBR materials and authored LODs.
2. **Characters:** game-ready WWII infantry meshes, skeletons, locomotion animation blueprints, transitions, procedural aiming, ragdolls, reloads, grenades, sprint, crouch and prone.
3. **Weapons/vehicles:** correctly modeled firearms with sights and animations; mobile-optimized drivable tank(s), light vehicles and weapon emplacements.
4. **Presentation:** baked/probed mobile lighting, shadows, sky, terrain decals, Niagara muzzle/explosions/smoke, layered spatial audio, impact feedback and readable HUD.
5. **Gameplay:** refined cover and navigation, capture rules, friendly-fire-safe squads, fair enemies, tank/infantry interplay, encounter design, objectives and accessibility.
6. **Validation:** shipping cook/launch on physical mid-range Android, sustained 15-minute battle at playable stable frame rate, device thermal and memory profiling, screenshots/video review and independent game QA.
7. **Online:** authoritative servers, player accounts, matchmaking, anti-cheat and network prediction are separate major projects; a free static Cloudflare Pages site is not a host for this live 3D multiplayer game.

**Suggested acceptance baseline:** real mobile 3D battle with 3 distinct weapon classes, a tank, 8–12 active infantry units, one fully authored map, usable touch controls, no blocker issues, and 30 stable fps on a defined mid-range device. That baseline does **not** equal full parity; expand art/maps/modes/weapons only after it works.

Useful Epic references:

- https://dev.epicgames.com/documentation/unreal-engine/android-quick-start
- https://dev.epicgames.com/documentation/unreal-engine/packaging-android-projects-in-unreal-engine
- https://dev.epicgames.com/documentation/unreal-engine/customizing-device-profiles-and-scalability-in-unreal-engine-projects-for-android

The separately hosted `browser/` Field Edition is a small 2.5D skirmish, useful for access and feedback. It is not the final native mobile product.
