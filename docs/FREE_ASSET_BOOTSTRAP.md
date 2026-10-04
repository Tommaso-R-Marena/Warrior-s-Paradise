# Free Asset Bootstrap

Warrior's Paradise separates **redistributable free assets** from **free but provider-licensed local dependencies**.

## One-command CC0 discovery/download

From the repository root:

```bash
python tools/assets/bootstrap_free_assets.py --dry-run
python tools/assets/bootstrap_free_assets.py --polyhaven --resolution 2k
python tools/assets/bootstrap_free_assets.py --direct-cc0
python tools/assets/bootstrap_free_assets.py --all
```

The downloader uses only the Python standard library.

Downloaded raw files go to:

```text
ExternalAssets/
```

and provenance/hashes are recorded in:

```text
ExternalAssets/asset-lock.json
```

`ExternalAssets/` is intentionally ignored by Git. CC0 files can later be deliberately vendored or moved to Git LFS, but downloading them must not silently enlarge the repository.

## Poly Haven

The bootstrapper queries the live Poly Haven API for a curated WWII/rural-environment material set:

- mud/dirt;
- gravel;
- cobblestone;
- weathered plaster;
- brick;
- stone masonry;
- roof tile;
- weathered wood;
- rusted metal;
- worn concrete;
- tree/foliage models;
- crates/barrels;
- overcast and sunset HDRIs.

The selected asset ID, source URL, requested resolution, download URL and file hashes are written to the lockfile.

Because the live API is being used, the project should retain a visible **Powered by Poly Haven** credit even though the downloaded assets themselves are CC0.

## Direct CC0

The catalog currently contains the OpenGameArt rigged WWII soldier as a small, legal development/retargeting target. More direct CC0 downloads can be added to `free_asset_catalog.json` only after verifying the source page and license.

## Quaternius

The catalog tracks these CC0 packs for manual download:

- Universal Animation Library;
- Universal Animation Library 2;
- Universal Base Characters;
- Animated Tanks Pack;
- Ultimate Guns Pack.

The packs provide FBX/GLB exports and are good for retargeting, animation coverage and immediate visual replacement, but they should be treated as a development/secondary art layer rather than the final photoreal target.

## Epic / Fab

The following should be acquired through the Epic/Fab UI and kept local/private:

- Game Animation Sample Project;
- MetaHuman;
- Modular Rural Cabins;
- free Niagara explosion packs;
- free muzzle-flash Niagara content.

Do **not** mirror raw Fab assets into a public repository. Fab permits commercial use/incorporation into a project but does not permit standalone raw redistribution.

## Sonniss

Download the free GDC Game Audio bundle locally.

Use it for synchronized game audio such as:

- explosions;
- bullet impacts;
- debris;
- machinery;
- battlefield ambience;
- foley.

Do not commit or republish the raw bundle as a standalone sound library.

## Import into Unreal

After CC0 files have been downloaded, open the project in Unreal Engine 5.8 and run:

```text
Content/Python/wp_import_external_assets.py
```

through **Tools -> Execute Python Script**.

The script imports supported FBX/OBJ/GLB/glTF, image/HDRI and WAV files into `/Game/External/...` while preserving source-folder grouping.

## Production binding

Raw import is only the first step. Imported skeletal/vehicle/weapon assets should be bound through Warrior's Paradise presentation/visual profiles and through the gameplay delegates documented in `PRESENTATION_BINDINGS.md`.

Gameplay logic must never be copied into marketplace Blueprints.
