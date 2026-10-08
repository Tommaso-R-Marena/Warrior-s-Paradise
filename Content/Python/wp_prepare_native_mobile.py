"""Run the existing licensed-content workflow and author a mobile UE level.

Executed inside the Unreal Editor with its Python plugin installed. Each
stage raises on failure. No downloads, new licenses, secret credentials or
automatic project startup-map replacement occur here.
"""
from __future__ import annotations

from pathlib import Path
import runpy

import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
SCRIPTS = ROOT / "Content" / "Python"


def stage(filename: str, *, guarded: bool = False) -> None:
    path = SCRIPTS / filename
    if not path.is_file():
        raise FileNotFoundError(path)
    unreal.log(f"[WP Native Art Pipeline] BEGIN {filename}")
    # Existing legacy scripts execute main() on import/runpy. The new level
    # generator uses a __main__ guard so invoke it explicitly instead.
    result = runpy.run_path(str(path), run_name="__wp_build__")
    if guarded:
        result["main"]()
    unreal.log(f"[WP Native Art Pipeline] DONE {filename}")


def main() -> None:
    lock = ROOT / "ExternalAssets" / "asset-lock.json"
    if not lock.is_file():
        raise RuntimeError(
            "No ExternalAssets/asset-lock.json. Run the CC0 downloader or "
            "record the licensed asset provenance before editor import."
        )

    # Import the tracked source art, compile CC0/Poly Haven texture materials,
    # and connect available meshes to the existing gameplay presentation.
    stage("wp_import_external_assets.py")
    stage("wp_build_cc0_materials.py")
    stage("wp_setup_free_profiles.py")
    stage("wp_build_mobile_village.py", guarded=True)

    output = ROOT / "Content" / "WarriorParadise" / "Maps" / "WP_NormandyMobile.umap"
    if not output.is_file():
        raise RuntimeError(f"Native level did not appear on disk: {output}")
    unreal.log(
        "[WP Native Art Pipeline] Level exists. Open it in the Unreal Editor, "
        "inspect character/weapon/vehicle animations, then use "
        "tools/assets/activate_mobile_map.py --apply to enable game startup."
    )


if __name__ == "__main__":
    main()
