#!/usr/bin/env python3
"""Inventory shipped UE mobile content; --strict fails if production art is absent.

This validates presence and conventional filenames, NOT artistic fidelity,
animation correctness, asset licensing, polygon budgets, or cook success.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CONTENT = ROOT / "Content"
SLOTS = {
    "3d_playable_levels": ("*.umap",),
    "skeletal_characters": ("SK_*.uasset",),
    "infantry_animation_blueprints": ("ABP_*.uasset",),
    "infantry_animation_sequences": ("A_*.uasset", "Anim_*.uasset"),
    "weapons_or_vehicles": ("SM_*.uasset",),
    "materials": ("M_*.uasset", "MI_*.uasset"),
    "sound_assets": ("S_*.uasset", "SC_*.uasset", "USound_*.uasset"),
}


def inventory() -> dict:
    all_assets = tuple(CONTENT.rglob("*.uasset")) if CONTENT.exists() else ()
    maps = tuple(CONTENT.rglob("*.umap")) if CONTENT.exists() else ()
    categories = {}
    for label, patterns in SLOTS.items():
        found = {
            p.relative_to(ROOT).as_posix()
            for pattern in patterns
            for p in CONTENT.rglob(pattern)
        } if CONTENT.exists() else set()
        categories[label] = {"count": len(found), "examples": sorted(found)[:5]}

    external = ROOT / "ExternalAssets"
    lock = external / "asset-lock.json"
    return {
        "repository": "Warrior-s-Paradise",
        "native_3d_assets": len(all_assets),
        "native_maps": len(maps),
        "external_asset_lock_present": lock.is_file(),
        "categories": categories,
        "production_content_ready": (
            len(maps) >= 1
            and categories["skeletal_characters"]["count"] >= 1
            and categories["infantry_animation_blueprints"]["count"] >= 1
            and categories["weapons_or_vehicles"]["count"] >= 1
            and categories["materials"]["count"] >= 1
            and categories["sound_assets"]["count"] >= 1
            and lock.is_file()
        ),
        "qualification": (
            "Filename and inventory checks only. Real engine import, licensing, "
            "animation, cook and visual QA remain mandatory."
        ),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--strict", action="store_true", help="Fail when minimum art categories are absent")
    parser.add_argument("--json", action="store_true", help="Emit machine-readable JSON")
    args = parser.parse_args()
    report = inventory()
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print(
            f"WP native mobile content: {report['native_3d_assets']} .uasset, "
            f"{report['native_maps']} .umap, "
            f"asset lock={'yes' if report['external_asset_lock_present'] else 'no'}"
        )
        for name, info in report["categories"].items():
            print(f"  {name}: {info['count']}")
        print("Minimum art inventory ready:", report["production_content_ready"])
        print(report["qualification"])
    return 0 if report["production_content_ready"] or not args.strict else 2


if __name__ == "__main__":
    raise SystemExit(main())
