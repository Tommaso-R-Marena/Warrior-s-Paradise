#!/usr/bin/env python3
"""Fail-closed checksum and provenance gate for local Unreal art imports.

The downloader's lock is a list of *specific files*, not permission to import
everything in ExternalAssets. This standalone tool verifies that each importable
file is tracked, inside the expected tree, and byte-identical to the lock.

Run: python tools/assets/validate_asset_provenance.py
     python tools/assets/validate_asset_provenance.py --root /path/to/project

This is a technical integrity check, NOT a legal license determination.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path

IMPORTABLE = {
    ".fbx", ".obj", ".glb", ".gltf", ".png", ".jpg", ".jpeg",
    ".tga", ".tif", ".tiff", ".exr", ".hdr", ".wav",
}
DIGESTS = {"sha256": (64, hashlib.sha256), "md5": (32, hashlib.md5)}
DEFAULT_ROOT = Path(__file__).resolve().parents[2]


class AssetIntegrityError(ValueError):
    pass


def hash_file(path: Path, digest_name: str) -> str:
    digest = DIGESTS[digest_name][1]()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def validate_root(root: Path) -> dict:
    root = root.resolve()
    external = root / "ExternalAssets"
    lock_path = external / "asset-lock.json"
    if not lock_path.is_file() or lock_path.is_symlink():
        raise AssetIntegrityError("Missing or linked asset-lock.json: record asset provenance before importing")

    try:
        lock = json.loads(lock_path.read_text(encoding="utf-8"))
    except (ValueError, UnicodeError) as exc:
        raise AssetIntegrityError(f"Invalid provenance JSON: {exc}") from exc

    assets = lock.get("assets") if isinstance(lock, dict) else None
    if lock.get("schema_version") != 1 or not isinstance(assets, dict) or not assets:
        raise AssetIntegrityError("Provenance lock requires schema_version=1 and nonempty assets object")

    errors: list[str] = []
    recorded: set[Path] = set()
    validated = 0
    importable_tracked = 0

    for key, asset in sorted(assets.items()):
        if not isinstance(asset, dict):
            errors.append(f"{key}: asset record must be an object")
            continue
        license_name = asset.get("license")
        source_url = asset.get("source_url")
        if not isinstance(license_name, str) or license_name.strip().lower() in {"", "unknown", "none"}:
            errors.append(f"{key}: explicit applicable license is required")
        if not isinstance(source_url, str) or not source_url.startswith("https://"):
            errors.append(f"{key}: HTTPS source URL is required")
        records = asset.get("files", []) + asset.get("extracted_files", [])
        if not isinstance(records, list) or not records:
            errors.append(f"{key}: no recorded files")
            continue

        for record in records:
            if not isinstance(record, dict) or not isinstance(record.get("path"), str):
                errors.append(f"{key}: malformed file record")
                continue
            relative = Path(record["path"])
            if relative.is_absolute() or ".." in relative.parts or not relative.parts or relative.parts[0] != "ExternalAssets":
                errors.append(f"{key}: file path must remain under ExternalAssets: {relative}")
                continue
            path = root / relative
            if path.is_symlink() or not path.is_file() or external.resolve() not in path.resolve().parents:
                errors.append(f"{key}: missing, linked, or escaped source: {relative}")
                continue
            canonical = path.resolve()
            if canonical in recorded:
                errors.append(f"{key}: duplicate provenance file {relative}")
                continue
            recorded.add(canonical)

            if "bytes" in record and (type(record["bytes"]) is not int or path.stat().st_size != record["bytes"]):
                errors.append(f"{key}: size mismatch {relative}")
                continue
            selected = [
                name for name in DIGESTS
                if isinstance(record.get(name), str) and
                re.fullmatch(r"[0-9a-fA-F]{" + str(DIGESTS[name][0]) + r"}", record[name])
            ]
            if not selected:
                errors.append(f"{key}: no valid SHA-256 or MD5 for {relative}")
                continue
            for name in selected:
                if hash_file(path, name) != record[name].lower():
                    errors.append(f"{key}: {name} mismatch for {relative}")
            if path.suffix.lower() in IMPORTABLE:
                importable_tracked += 1
            validated += 1

    if external.exists():
        for file in sorted(external.rglob("*")):
            if file.is_symlink():
                errors.append(f"Linked external asset forbidden: {file.relative_to(root)}")
            elif file.is_file() and file.suffix.lower() in IMPORTABLE and file.resolve() not in recorded:
                errors.append(f"Untracked importable source: {file.relative_to(root)}")

    if importable_tracked == 0:
        errors.append("Provenance lock contains no importable model, texture or audio file")
    if errors:
        raise AssetIntegrityError("\n".join(errors[:50]))

    return {
        "assets": len(assets),
        "validated_files": validated,
        "importable_files": importable_tracked,
        "status": "PASS",
        "notice": "Checksum/provenance only; independently review licenses before distribution.",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=DEFAULT_ROOT)
    args = parser.parse_args()
    try:
        report = validate_root(args.root)
    except (AssetIntegrityError, OSError) as exc:
        print(f"WP ASSET PROVENANCE FAIL: {exc}")
        return 1
    print("WP ASSET PROVENANCE PASS:", json.dumps(report, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
