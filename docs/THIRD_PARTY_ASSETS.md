# Third-Party / Free Asset Register

This document is a human-readable companion to `tools/assets/free_asset_catalog.json`.

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
