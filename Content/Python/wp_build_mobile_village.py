"""Generate an *actual saved UE5 level* dressed with imported licensed meshes.

Run in Unreal Editor after importing ExternalAssets and CC0 materials:

  Tools -> Execute Python Script -> Content/Python/wp_build_mobile_village.py

Default: refuses to create a pretend production map without imported mesh assets.
For a geometry-only test map, set WP_ALLOW_BLOCKOUT=1 in the editor's environment.

This level contains only NON-COLLIDING decorative actors around the existing
runtime battlefield built by AWPBattlefieldDirector; do not duplicate its cover
collision, flag objectives, playable boundaries, spawn points or atmosphere.

The script does not download assets, certify licenses or generate skeletal meshes.
It never overwrites an existing map. A .umap must be committed through Git LFS
and the project default map updated only after an actual editor/cook playtest.
"""

from __future__ import annotations

from dataclasses import dataclass
import os
from pathlib import Path
import random
import runpy

import unreal

MAP_PATH = "/Game/WarriorParadise/Maps/WP_NormandyMobile"
ASSET_ROOT = "/Game/External"
SEED = 19440606


@dataclass(frozen=True)
class AssetSlot:
    key: str
    phrases: tuple[tuple[str, ...], ...]
    count: int


SLOTS = (
    AssetSlot("tree", (("tree",), ("oak",), ("deciduous",)), 24),
    AssetSlot("bush", (("bush",), ("shrub",), ("hedge",)), 46),
    AssetSlot("rubble", (("rubble",), ("debris",), ("rock",)), 34),
    AssetSlot("crate", (("crate",), ("supply", "box")), 15),
    AssetSlot("fence", (("wood", "fence"), ("fence",)), 18),
    AssetSlot("barrel", (("barrel",), ("drum",)), 10),
    AssetSlot("stump", (("stump",), ("log",)), 9),
    AssetSlot("sandbag", (("sandbag",), ("sand", "bag")), 28),
)


def log(message: str) -> None:
    unreal.log("[WP Native Mobile Scene] " + message)


def require_provenance() -> None:
    lock = unreal.Paths.convert_relative_path_to_full(
        unreal.Paths.project_dir() + "ExternalAssets/asset-lock.json"
    )
    if os.environ.get("WP_ALLOW_BLOCKOUT") == "1":
        log("Explicit blockout mode: this scene is NOT production-art validated.")
        return
    if not Path(lock).is_file():
        raise RuntimeError(
            "ExternalAssets/asset-lock.json is missing. Generate the original "
            "props or acquire license-tracked art before scene generation."
        )
    project_root = Path(unreal.Paths.project_dir()).resolve()
    script = project_root / "tools/assets/validate_asset_provenance.py"
    check = runpy.run_path(str(script), run_name="__map_integrity_gate__")
    report = check["validate_root"](project_root)
    log(f"Verified {report['importable_files']} source assets before authoring.")


def imported_static_meshes() -> list[tuple[str, unreal.StaticMesh]]:
    if not unreal.EditorAssetLibrary.does_directory_exist(ASSET_ROOT):
        return []
    results = []
    for path in unreal.EditorAssetLibrary.list_assets(
        ASSET_ROOT, recursive=True, include_folder=False
    ):
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if isinstance(asset, unreal.StaticMesh):
            results.append((path.lower(), asset))
    return results


def find_mesh(
    candidates: list[tuple[str, unreal.StaticMesh]],
    slot: AssetSlot,
) -> unreal.StaticMesh | None:
    scored = []
    for path, mesh in candidates:
        for phrase in slot.phrases:
            if all(token in path for token in phrase):
                scored.append((len(phrase) * 100 + sum(map(len, phrase)), path, mesh))
                break
    scored.sort(key=lambda item: (item[0], item[1]), reverse=True)
    return scored[0][2] if scored else None


def location_safe(x: float, y: float, slot: str) -> bool:
    # Avoid spawn, objectives, road corridor and the four runtime buildings.
    # A no-collision visual can still obscure a player even if not physically
    # blocking them. Aim to concentrate details on battlefield margins.
    if abs(x) < 1050 and -4400 < y < 3450:
        return False
    if abs(x) < 500 and -4500 < y < 3550:
        return False
    for bx, by in (
        (-1750, -500), (1650, -1450), (-1900, -2550), (1850, -3300)
    ):
        if (x - bx) ** 2 + (y - by) ** 2 < 1050 ** 2:
            return False
    return -5300 < x < 5300 and -5250 < y < 5250


def spawn_visual(
    mesh: unreal.StaticMesh,
    slot: str,
    index: int,
    x: float,
    y: float,
    yaw: float,
    scale: float,
) -> None:
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor.static_class(),
        unreal.Vector(x, y, 15.0),
        unreal.Rotator(0.0, yaw, 0.0),
    )
    if not actor:
        raise RuntimeError(f"Could not spawn {slot} instance {index}")

    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    if not component:
        raise RuntimeError("StaticMeshActor missing StaticMeshComponent")
    component.set_static_mesh(mesh)
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    component.set_cast_shadow(slot in {"tree", "fence"})
    actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    actor.set_actor_label(f"WP_Art_{slot}_{index:03d}")


def main() -> None:
    require_provenance()
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        raise RuntimeError(
            f"{MAP_PATH} already exists. No overwrite attempted; duplicate "
            "an existing map in the editor before regenerating."
        )

    assets = imported_static_meshes()
    selected = {
        slot.key: find_mesh(assets, slot)
        for slot in SLOTS
    }
    matched = {key: mesh for key, mesh in selected.items() if mesh}
    if len(matched) < 4 and os.environ.get("WP_ALLOW_BLOCKOUT") != "1":
        raise RuntimeError(
            f"Only {len(matched)} of {len(SLOTS)} environmental art categories "
            "are imported. Need at least four categories to build an art map. "
            "Use WP_ALLOW_BLOCKOUT=1 only for a geometry-only experiment."
        )

    editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not editor.new_level(MAP_PATH, False):
        raise RuntimeError(f"Could not create level at {MAP_PATH}")

    randomizer = random.Random(SEED)
    generated = 0
    missing = []
    for slot in SLOTS:
        mesh = selected[slot.key]
        if not mesh:
            missing.append(slot.key)
            continue

        placed = 0
        for index in range(slot.count):
            for _ in range(90):
                x = randomizer.uniform(-5100, 5100)
                y = randomizer.uniform(-5150, 5100)
                if location_safe(x, y, slot.key):
                    break
            else:
                continue
            yaw = randomizer.uniform(-180, 180)
            # Mesh authors set heterogeneous native scales. The imported
            # asset should already have reasonable UE-centimeter dimensions;
            # all scales are mild variants and can be tuned in the editor.
            scale = randomizer.uniform(.85, 1.13)
            spawn_visual(mesh, slot.key, index, x, y, yaw, scale)
            placed += 1
            generated += 1
        log(f"{slot.key}: {placed} imported mesh actors")

    if os.environ.get("WP_ALLOW_BLOCKOUT") == "1" and generated == 0:
        # An empty saved test level remains valid: the existing native
        # BattlefieldDirector will generate ground/buildings on BeginPlay.
        log("Geometry-only test map; no licensed artwork imported.")

    if not editor.save_current_level():
        raise RuntimeError("Editor did not save the newly created .umap")

    log(
        f"Saved {MAP_PATH} with {generated} dressing actors. "
        f"Missing slots: {', '.join(missing) or 'none'}. "
        "Play in Editor, check mobile shaders/collision and THEN point "
        "GameDefaultMap to this file for the Android cook."
    )


if __name__ == "__main__":
    main()
