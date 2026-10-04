"""Import downloaded external assets into /Game/External.

Run from Unreal Editor:
  Tools / Execute Python Script -> Content/Python/wp_import_external_assets.py

The script only imports files that already exist under ExternalAssets/. It never
downloads marketplace/Epic/Sonniss content and never makes licensing decisions.
"""

from __future__ import annotations

import json
from pathlib import Path
import re

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir()).resolve()
EXTERNAL_ROOT = PROJECT_ROOT / "ExternalAssets"
LOCK_PATH = EXTERNAL_ROOT / "asset-lock.json"
DESTINATION_ROOT = "/Game/External"

SUPPORTED = {
    ".fbx",
    ".obj",
    ".glb",
    ".gltf",
    ".png",
    ".jpg",
    ".jpeg",
    ".tga",
    ".tif",
    ".tiff",
    ".exr",
    ".hdr",
    ".wav",
}


def log(message: str) -> None:
    unreal.log(f"[WP Asset Import] {message}")


def sanitize(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]+", "_", value).strip("_")


def destination_for(source: Path) -> str:
    relative = source.relative_to(EXTERNAL_ROOT)
    parts = [sanitize(part) for part in relative.parts[:-1] if part]
    if not parts:
        return DESTINATION_ROOT
    return DESTINATION_ROOT + "/" + "/".join(parts)


def import_file(source: Path) -> list[str]:
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = destination_for(source)
    task.automated = True
    task.replace_existing = False
    task.replace_existing_settings = False
    task.save = True

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return list(task.imported_object_paths)


def collect_files() -> list[Path]:
    if not EXTERNAL_ROOT.exists():
        return []

    files = []
    for path in EXTERNAL_ROOT.rglob("*"):
        if not path.is_file():
            continue
        if path.name == "asset-lock.json":
            continue
        if path.suffix.lower() in SUPPORTED:
            files.append(path)

    return sorted(files)


def main() -> None:
    files = collect_files()
    if not files:
        log(
            "No importable files found. Run "
            "python tools/assets/bootstrap_free_assets.py --all first."
        )
        return

    if LOCK_PATH.exists():
        lock = json.loads(LOCK_PATH.read_text(encoding="utf-8"))
        log(f"Loaded provenance lock with {len(lock.get('assets', {}))} entries.")
    else:
        log("No asset-lock.json found; imports will proceed without provenance lock.")

    imported_count = 0
    for source in files:
        try:
            imported = import_file(source)
        except Exception as exc:
            unreal.log_error(f"[WP Asset Import] Failed {source}: {exc}")
            continue

        if imported:
            imported_count += len(imported)
            log(f"{source.relative_to(PROJECT_ROOT)} -> {', '.join(imported)}")
        else:
            unreal.log_warning(
                f"[WP Asset Import] No assets created for {source.relative_to(PROJECT_ROOT)}"
            )

    unreal.EditorAssetLibrary.save_directory(DESTINATION_ROOT, only_if_is_dirty=True)
    log(f"Finished. Imported {imported_count} Unreal assets.")


main()
