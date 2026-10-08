# Third-Party / Free Asset Register

This document is a human-readable companion to `tools/assets/free_asset_catalog.json`.

## Self-hosted Cloudflare browser scenery — Kenney CC0

The browser `/3d/` edition now vendors five original **Kenney Nature Kit v2.1** meshes (oak, pine, rock, bush and fence) as canonical Base64-encoded GLB files under `browser/3d/assets/kenney/`. They are decoded on the device and embedded into existing WebGL2 static geometry. Credit is optional under **CC0 1.0**, but we preserve it:

- Original publisher and license: https://kenney.nl/assets/nature-kit
- Public redistribution mirror: https://github.com/Hidencod/tge-assets
- Immutable per-file upstream Git blob hashes, byte lengths and exact paths: `browser/3d/assets/kenney/MANIFEST.json`
- Integrity test: `node tools/tests/browser_cc0_glb_smoke.js`
- Integration/fallback test: `node tools/tests/browser_cc0_scene_smoke.js`

This distinct browser source register complements the Unreal `ExternalAssets/asset-lock.json`; nothing from Fab, Sonniss, or unclear-license Sketchfab uploads has been vendored. The current geometry importer supports only static triangle meshes and uniform color materials, not skinned soldiers, weapon animations or photorealistic PBR textures.

## Redistributable-first sources

### Poly Haven

- Content license: CC0.
- Use: PBR materials, HDRIs, selected props/foliage.
- Acquisition: automated through `bootstrap_free_assets.py`.
- Live API condition: the bootstrap tooling identifies Poly Haven as the source and the project should retain a visible "Powered by Poly Haven" credit.
- Raw download provenance/hashes: `ExternalAssets/asset-lock.json`.

### Quaternius

Tracked CC0 packs:

- Universal Animation Library;
- Universal Animation Library 2;
- Universal Base Characters;
- Animated Tanks Pack;
- Ultimate Guns Pack.

Use: development/retargeting animation coverage and immediate visual replacement. These packs are engine-compatible and commercially usable under CC0, but their stylization means they are not automatically the final photoreal art direction.

### OpenGameArt WWII soldier

The catalog contains the CC0 rigged low-poly WWII soldier for development retargeting and silhouette testing.

### OpenGameArt combined-arms candidates

Tracked CC0 candidates include:

- rigged WWII airplane (Blender source; export to FBX/glTF before Unreal import);
- low-poly GAZ-67 Soviet WWII jeep.

These are legal development/fallback candidates; their art fidelity should be evaluated in-engine before treating them as final production content.

### MetaWorldOS WWII Tank Pack

CC0 pack containing Panther, Panzer IV, Sherman and IS-2 models plus battlefield props. The pack is a strong period-correct replacement candidate for the runtime cube tanks.

## Free provider-licensed dependencies

These are free to use under their provider license but should **not** be mirrored as raw files in a public source repository.

### Epic Game Animation Sample

Use as the primary high-fidelity humanoid locomotion/motion-matching reference and migration source.

### MetaHuman

Use as the high-fidelity human/rig foundation where appropriate.

### Fab Modular Rural Cabins

Use for realistic rural buildings, interiors and foliage.

### Free Fab transport

The catalog tracks the free **Military Field Truck Depo** listing as a WWII-inspired transport candidate. Keep the raw provider asset local/private under the applicable Fab terms.

### Fab VFX

Tracked free Niagara sources:

- FREE! Realistic Niagara Explosions Pack;
- Realistic Fire & Explosion VFX Vol.1 - FREE Starter Pack;
- Muzzle Flash (Niagara System).

### Free period weapons on Fab

Tracked candidates:

- free Kar98k;
- free MP40 (Maschinenpistole 40).

Keep the original Fab listing/license record with the local project acquisition.

### Sonniss GDC Game Audio

Use professional free audio for explosions, impacts, machinery, debris, ambience and foley. Raw sound files are local/private dependencies and are not redistributed as a standalone library.

## Rule

A resource being "free" is not sufficient. Every asset integrated into the project must have:

1. a source URL;
2. a recorded license/provider terms;
3. an acquisition date/version where possible;
4. a known redistribution policy;
5. a project destination;
6. a documented gameplay/presentation purpose.

If any of these are unknown, the asset remains unapproved.
