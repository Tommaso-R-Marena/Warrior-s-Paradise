#!/usr/bin/env python3
"""Activate a cooked, generated Warrior's Paradise UE map safely.

Does not generate art or levels. Refuses to modify the default map unless
the requested .umap *exists* on disk. A production promotion also requires
the normal art inventory to be complete.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
ENGINE = ROOT / "Config" / "DefaultEngine.ini"
DEFAULT_LEVEL = "/Game/WarriorParadise/Maps/WP_NormandyMobile"
EMPTY_LEVEL = "/Engine/Maps/Entry"


def asset_path_to_umap(asset_path: str) -> Path:
    if not re.fullmatch(r"/Game/WarriorParadise/Maps/[A-Za-z0-9_]+", asset_path):
        raise ValueError(
            "Map must belong to /Game/WarriorParadise/Maps with a simple asset name."
        )
    return ROOT / "Content" / (asset_path.removeprefix("/Game/") + ".umap")


def promote(current: str, new_map: str) -> str:
    # Change only the game/editor startup slots in GameMapsSettings.
    # Refuse silently appending duplicate keys or overwriting unrelated sections.
    match = re.search(
        r"(?m)^\[/Script/EngineSettings.GameMapsSettings\]\s*$",
        current,
    )
    if match is None:
        raise ValueError("GameMapsSettings section not found")
    next_section = re.search(r"(?m)^\[[^\]]+\]\s*$", current[match.end():])
    end = match.end() + next_section.start() if next_section else len(current)
    section = current[match.end():end]
    for key in ("EditorStartupMap", "GameDefaultMap"):
        pattern = rf"(?m)^{key}=[^\r\n]*$"
        if len(re.findall(pattern, section)) != 1:
            raise ValueError(f"Expected exactly one {key} entry")
        section = re.sub(pattern, f"{key}={new_map}", section)
    return current[:match.end()] + section + current[end:]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--map", dest="level", default=DEFAULT_LEVEL)
    parser.add_argument("--apply", action="store_true", help="Actually update Config/DefaultEngine.ini")
    parser.add_argument("--reset", action="store_true", help="Restore /Engine/Maps/Entry")
    parser.add_argument("--require-production-art", action="store_true")
    args = parser.parse_args()

    if not ENGINE.is_file():
        parser.error("Config/DefaultEngine.ini does not exist")
    destination = EMPTY_LEVEL if args.reset else args.level
    if not args.reset:
        target = asset_path_to_umap(destination)
        if not target.is_file():
            parser.error(f"Missing real authored Unreal map: {target}")
        if args.require_production_art:
            from audit_mobile_content import inventory

            if not inventory()["production_content_ready"]:
                parser.error("Art readiness inventory is incomplete; refusing promotion")

    original = ENGINE.read_text(encoding="utf-8")
    updated = promote(original, destination)
    changed = updated != original
    print(f"Active level: {destination}")
    print(f"Config change required: {changed}")
    if args.apply and changed:
        ENGINE.write_text(updated, encoding="utf-8")
        print(f"Updated {ENGINE}")
    elif not args.apply:
        print("DRY RUN: use --apply after inspecting the level in Unreal Editor.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
