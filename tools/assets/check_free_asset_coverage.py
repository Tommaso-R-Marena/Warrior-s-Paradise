#!/usr/bin/env python3
"""Report free-asset acquisition coverage without requiring Unreal Engine."""

from __future__ import annotations

import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
CATALOG = ROOT / "tools" / "assets" / "free_asset_catalog.json"
LOCK = ROOT / "ExternalAssets" / "asset-lock.json"


def read(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def main() -> int:
    catalog = read(CATALOG)
    expected_slots = {
        bucket["slot"]
        for bucket in catalog.get("polyhaven", {}).get("buckets", [])
    }

    print("Warrior's Paradise free-resource coverage")
    print(f"  catalog: {CATALOG.relative_to(ROOT)}")
    print(f"  Poly Haven slots planned: {len(expected_slots)}")
    print(f"  direct CC0 sources: {len(catalog.get('direct_cc0', []))}")
    print(f"  manual/provider-free sources: {len(catalog.get('manual_free', []))}")

    if not LOCK.exists():
        print("\nNo ExternalAssets/asset-lock.json yet.")
        print("Run: python tools/assets/bootstrap_free_assets.py --all")
        return 2

    lock = read(LOCK)
    acquired = lock.get("assets", {})

    acquired_slots = {
        key.split(":", 1)[1]
        for key in acquired
        if key.startswith("polyhaven:")
    }

    missing_slots = sorted(expected_slots - acquired_slots)
    direct_expected = {
        f"direct:{asset['id']}"
        for asset in catalog.get("direct_cc0", [])
    }
    missing_direct = sorted(direct_expected - set(acquired))

    print(f"\nPoly Haven slots acquired: {len(acquired_slots)}/{len(expected_slots)}")
    if missing_slots:
        print("Missing Poly Haven slots:")
        for slot in missing_slots:
            print(f"  - {slot}")

    print(
        "Direct CC0 acquired: "
        f"{len(direct_expected) - len(missing_direct)}/{len(direct_expected)}"
    )
    if missing_direct:
        print("Missing direct CC0:")
        for key in missing_direct:
            print(f"  - {key.removeprefix('direct:')}")

    incomplete = bool(missing_slots or missing_direct)

    print("\nProvider-gated/manual free sources are intentionally not treated as missing.")
    print("List them with:")
    print("  python tools/assets/bootstrap_free_assets.py --list-manual")

    if incomplete:
        return 1

    print("\nRedistributable free-resource bootstrap is complete.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
