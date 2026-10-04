#!/usr/bin/env python3
"""License-aware free-asset bootstrapper for Warrior's Paradise.

This script intentionally distinguishes redistributable CC0 content from assets
that must be acquired locally under marketplace/sample/audio licenses.

Examples:
  python tools/assets/bootstrap_free_assets.py --dry-run
  python tools/assets/bootstrap_free_assets.py --polyhaven --resolution 2k
  python tools/assets/bootstrap_free_assets.py --direct-cc0
  python tools/assets/bootstrap_free_assets.py --all

Downloaded files go to ExternalAssets/, which is git-ignored by default.
A reproducibility/provenance lock is written to ExternalAssets/asset-lock.json.
"""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import sys
import tempfile
import urllib.error
import urllib.parse
import urllib.request
from typing import Any, Iterable

ROOT = Path(__file__).resolve().parents[2]
CATALOG_PATH = ROOT / "tools" / "assets" / "free_asset_catalog.json"
EXTERNAL_ROOT = ROOT / "ExternalAssets"
LOCK_PATH = EXTERNAL_ROOT / "asset-lock.json"
USER_AGENT = "WarriorsParadise-FreeAssetBootstrap/1.0 (+GitHub: Warrior-s-Paradise)"

TEXTURE_EXTENSIONS = {".jpg", ".jpeg", ".png", ".tif", ".tiff", ".exr"}
MODEL_EXTENSIONS = {".fbx", ".glb", ".gltf", ".obj"}
HDRI_EXTENSIONS = {".hdr", ".exr"}


def read_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(value, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


def http_json(url: str) -> Any:
    request = urllib.request.Request(
        url,
        headers={
            "User-Agent": USER_AGENT,
            "Accept": "application/json",
        },
    )
    with urllib.request.urlopen(request, timeout=45) as response:
        return json.load(response)


def stream_download(
    url: str,
    destination: Path,
    *,
    expected_md5: str | None = None,
    dry_run: bool = False,
) -> dict[str, Any]:
    destination.parent.mkdir(parents=True, exist_ok=True)

    if dry_run:
        return {
            "path": str(destination.relative_to(ROOT)),
            "url": url,
            "md5": expected_md5,
            "downloaded": False,
            "dry_run": True,
        }

    request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    md5 = hashlib.md5()
    sha256 = hashlib.sha256()

    with tempfile.NamedTemporaryFile(
        prefix=destination.name + ".",
        suffix=".part",
        dir=destination.parent,
        delete=False,
    ) as temp:
        temp_path = Path(temp.name)
        try:
            with urllib.request.urlopen(request, timeout=90) as response:
                while True:
                    chunk = response.read(1024 * 1024)
                    if not chunk:
                        break
                    temp.write(chunk)
                    md5.update(chunk)
                    sha256.update(chunk)
        except Exception:
            temp_path.unlink(missing_ok=True)
            raise

    actual_md5 = md5.hexdigest()
    if expected_md5 and actual_md5.lower() != expected_md5.lower():
        temp_path.unlink(missing_ok=True)
        raise RuntimeError(
            f"MD5 mismatch for {url}: expected {expected_md5}, got {actual_md5}"
        )

    shutil.move(str(temp_path), destination)
    return {
        "path": str(destination.relative_to(ROOT)),
        "url": url,
        "md5": actual_md5,
        "sha256": sha256.hexdigest(),
        "bytes": destination.stat().st_size,
        "downloaded": True,
    }


def normalized_words(value: str) -> set[str]:
    return {
        word
        for word in re.split(r"[^a-z0-9]+", value.lower())
        if len(word) >= 2
    }


def asset_search_text(metadata: dict[str, Any]) -> str:
    tags = metadata.get("tags") or []
    attributes = metadata.get("attributes") or {}
    flattened_attributes: list[str] = []
    for key, value in attributes.items():
        flattened_attributes.append(str(key))
        if isinstance(value, list):
            flattened_attributes.extend(map(str, value))
        else:
            flattened_attributes.append(str(value))

    return " ".join(
        [
            str(metadata.get("name", "")),
            str(metadata.get("description", "")),
            str(metadata.get("category", "")),
            " ".join(map(str, tags)),
            " ".join(flattened_attributes),
        ]
    ).lower()


def score_asset(metadata: dict[str, Any], query: str) -> float:
    haystack = asset_search_text(metadata)
    words = normalized_words(query)
    score = 0.0

    if query.lower() in haystack:
        score += 25.0

    name = str(metadata.get("name", "")).lower()
    tags = {str(tag).lower() for tag in metadata.get("tags") or []}

    for word in words:
        if word in name:
            score += 7.0
        if word in tags:
            score += 5.0
        elif word in haystack:
            score += 1.5

    score += min(float(metadata.get("download_count", 0)) / 100000.0, 3.0)
    return score


def choose_polyhaven_asset(
    assets: dict[str, dict[str, Any]],
    bucket: dict[str, Any],
) -> tuple[str, dict[str, Any], str, float] | None:
    expected_type = int(bucket["type"])
    best: tuple[str, dict[str, Any], str, float] | None = None

    for query in bucket["queries"]:
        for asset_id, metadata in assets.items():
            if int(metadata.get("type", -1)) != expected_type:
                continue

            score = score_asset(metadata, query)
            if score <= 0:
                continue

            candidate = (asset_id, metadata, query, score)
            if best is None or candidate[3] > best[3]:
                best = candidate

    return best


def walk_file_records(node: Any, path: tuple[str, ...] = ()) -> Iterable[dict[str, Any]]:
    if isinstance(node, dict):
        if isinstance(node.get("url"), str):
            yield {
                "key_path": path,
                "url": node["url"],
                "size": node.get("size"),
                "md5": node.get("md5"),
            }

        for key, value in node.items():
            if key in {"url", "size", "md5"}:
                continue
            yield from walk_file_records(value, path + (str(key),))

    elif isinstance(node, list):
        for index, value in enumerate(node):
            yield from walk_file_records(value, path + (str(index),))


def suffix_from_url(url: str) -> str:
    path = urllib.parse.urlparse(url).path
    return Path(urllib.parse.unquote(path)).suffix.lower()


def record_score(
    record: dict[str, Any],
    asset_type: int,
    resolution: str,
) -> float:
    joined = "/".join(record["key_path"]).lower()
    url = record["url"].lower()
    extension = suffix_from_url(url)
    score = 0.0

    if resolution.lower() in joined or f"_{resolution.lower()}" in url:
        score += 20.0

    if asset_type == 0:
        if extension == ".hdr":
            score += 30.0
        elif extension == ".exr":
            score += 20.0

    elif asset_type == 1:
        if extension in TEXTURE_EXTENSIONS:
            score += 10.0
        for token, bonus in {
            "diff": 15.0,
            "albedo": 15.0,
            "basecolor": 15.0,
            "nor_gl": 12.0,
            "normal": 10.0,
            "rough": 10.0,
            "disp": 7.0,
            "ao": 5.0,
        }.items():
            if token in joined or token in url:
                score += bonus

    elif asset_type == 2:
        if extension == ".fbx":
            score += 40.0
        elif extension == ".glb":
            score += 35.0
        elif extension == ".gltf":
            score += 30.0
        elif extension == ".obj":
            score += 15.0
        elif extension in TEXTURE_EXTENSIONS:
            score += 6.0

    return score


def select_polyhaven_files(
    files_payload: Any,
    asset_type: int,
    resolution: str,
) -> list[dict[str, Any]]:
    records = list(walk_file_records(files_payload))
    allowed_extensions = (
        HDRI_EXTENSIONS
        if asset_type == 0
        else TEXTURE_EXTENSIONS
        if asset_type == 1
        else MODEL_EXTENSIONS | TEXTURE_EXTENSIONS
    )

    candidates = [
        record
        for record in records
        if suffix_from_url(record["url"]) in allowed_extensions
    ]

    if not candidates:
        return []

    if asset_type == 0:
        candidates.sort(
            key=lambda record: record_score(record, asset_type, resolution),
            reverse=True,
        )
        return candidates[:1]

    if asset_type == 1:
        selected: list[dict[str, Any]] = []
        seen_semantics: set[str] = set()

        semantic_tokens = [
            "diff",
            "albedo",
            "basecolor",
            "nor_gl",
            "normal",
            "rough",
            "disp",
            "ao",
        ]

        for record in sorted(
            candidates,
            key=lambda item: record_score(item, asset_type, resolution),
            reverse=True,
        ):
            joined = ("/".join(record["key_path"]) + " " + record["url"]).lower()
            semantic = next(
                (token for token in semantic_tokens if token in joined),
                "other",
            )
            if semantic in seen_semantics:
                continue
            if resolution.lower() not in joined and f"_{resolution.lower()}" not in joined:
                continue
            selected.append(record)
            seen_semantics.add(semantic)
            if len(selected) >= 6:
                break

        return selected

    model_candidates = [
        record
        for record in candidates
        if suffix_from_url(record["url"]) in MODEL_EXTENSIONS
    ]
    texture_candidates = [
        record
        for record in candidates
        if suffix_from_url(record["url"]) in TEXTURE_EXTENSIONS
    ]

    model_candidates.sort(
        key=lambda item: record_score(item, asset_type, resolution),
        reverse=True,
    )
    texture_candidates.sort(
        key=lambda item: record_score(item, asset_type, resolution),
        reverse=True,
    )

    selected = model_candidates[:1]
    selected.extend(
        record
        for record in texture_candidates
        if resolution.lower()
        in ("/".join(record["key_path"]) + " " + record["url"]).lower()
    )
    return selected[:7]


def safe_filename(record: dict[str, Any], index: int) -> str:
    path = urllib.parse.unquote(urllib.parse.urlparse(record["url"]).path)
    name = Path(path).name
    if not name:
        name = f"asset_{index:02d}"
    return re.sub(r"[^A-Za-z0-9._-]+", "_", name)


def load_lock() -> dict[str, Any]:
    if LOCK_PATH.exists():
        return read_json(LOCK_PATH)
    return {
        "schema_version": 1,
        "generated_by": USER_AGENT,
        "assets": {},
    }


def save_lock(lock: dict[str, Any]) -> None:
    lock["updated_utc"] = (
        dt.datetime.now(dt.timezone.utc)
        .replace(microsecond=0)
        .isoformat()
    )
    write_json(LOCK_PATH, lock)


def bootstrap_polyhaven(
    catalog: dict[str, Any],
    *,
    resolution: str,
    dry_run: bool,
    slots: set[str] | None,
) -> list[dict[str, Any]]:
    config = catalog["polyhaven"]
    api_base = config["api_base"].rstrip("/")
    assets = http_json(f"{api_base}/assets")
    lock = load_lock()
    results: list[dict[str, Any]] = []

    for bucket in config["buckets"]:
        slot = bucket["slot"]
        if slots and slot not in slots:
            continue

        choice = choose_polyhaven_asset(assets, bucket)
        if choice is None:
            print(f"[warn] no Poly Haven match for {slot}", file=sys.stderr)
            continue

        asset_id, metadata, query, score = choice
        print(
            f"[polyhaven] {slot}: {asset_id} / {metadata.get('name')} "
            f"(query={query!r}, score={score:.1f})"
        )

        files_payload = http_json(f"{api_base}/files/{urllib.parse.quote(asset_id)}")
        selected = select_polyhaven_files(
            files_payload,
            int(bucket["type"]),
            resolution,
        )

        if not selected:
            print(f"[warn] no downloadable files selected for {asset_id}", file=sys.stderr)
            continue

        asset_folder = (
            EXTERNAL_ROOT
            / "polyhaven"
            / slot
            / asset_id
        )

        downloaded: list[dict[str, Any]] = []
        for index, record in enumerate(selected):
            destination = asset_folder / safe_filename(record, index)
            if destination.exists() and not dry_run:
                downloaded.append(
                    {
                        "path": str(destination.relative_to(ROOT)),
                        "url": record["url"],
                        "md5": record.get("md5"),
                        "bytes": destination.stat().st_size,
                        "downloaded": False,
                        "reason": "already_exists",
                    }
                )
                continue

            downloaded.append(
                stream_download(
                    record["url"],
                    destination,
                    expected_md5=record.get("md5"),
                    dry_run=dry_run,
                )
            )

        entry = {
            "id": asset_id,
            "name": metadata.get("name"),
            "slot": slot,
            "type": int(bucket["type"]),
            "query": query,
            "license": "CC0-1.0",
            "source": "Poly Haven",
            "source_url": f"https://polyhaven.com/a/{asset_id}",
            "api_attribution": config.get("api_attribution"),
            "resolution": resolution,
            "files": downloaded,
        }

        lock["assets"][f"polyhaven:{slot}"] = entry
        results.append(entry)

    if not dry_run:
        save_lock(lock)

    return results


def bootstrap_direct_cc0(
    catalog: dict[str, Any],
    *,
    dry_run: bool,
) -> list[dict[str, Any]]:
    lock = load_lock()
    results: list[dict[str, Any]] = []

    for asset in catalog.get("direct_cc0", []):
        destination = EXTERNAL_ROOT / asset["destination"]
        print(f"[direct-cc0] {asset['name']} -> {destination.relative_to(ROOT)}")

        file_record = stream_download(
            asset["download_url"],
            destination,
            dry_run=dry_run,
        )

        entry = {
            "id": asset["id"],
            "name": asset["name"],
            "source": asset["source"],
            "license": asset["license"],
            "source_url": asset["page_url"],
            "files": [file_record],
            "purpose": asset.get("purpose"),
        }
        lock["assets"][f"direct:{asset['id']}"] = entry
        results.append(entry)

    if not dry_run:
        save_lock(lock)

    return results


def print_manual_sources(catalog: dict[str, Any]) -> None:
    print("\nManual/local-only free sources:")
    for asset in catalog.get("manual_free", []):
        print(f"  - {asset['name']} [{asset.get('license', 'provider terms')}]")
        print(f"    {asset['page_url']}")
        print(f"    acquire: {asset['acquisition']}")
        print(f"    use: {asset['purpose']}")
        print(f"    distribution: {asset['distribution']}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--polyhaven", action="store_true")
    parser.add_argument("--direct-cc0", action="store_true")
    parser.add_argument("--all", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--resolution", default=None)
    parser.add_argument(
        "--slot",
        action="append",
        dest="slots",
        help="Only process one named Poly Haven slot. Repeatable.",
    )
    parser.add_argument("--list-manual", action="store_true")
    args = parser.parse_args()

    catalog = read_json(CATALOG_PATH)
    do_polyhaven = args.polyhaven or args.all
    do_direct = args.direct_cc0 or args.all

    if not do_polyhaven and not do_direct and not args.list_manual:
        do_polyhaven = True
        args.dry_run = True

    resolution = (
        args.resolution
        or catalog["polyhaven"].get("default_resolution")
        or "2k"
    )

    try:
        if do_polyhaven:
            bootstrap_polyhaven(
                catalog,
                resolution=resolution,
                dry_run=args.dry_run,
                slots=set(args.slots) if args.slots else None,
            )

        if do_direct:
            bootstrap_direct_cc0(catalog, dry_run=args.dry_run)

        if args.list_manual or args.all:
            print_manual_sources(catalog)

    except (urllib.error.URLError, TimeoutError, RuntimeError) as exc:
        print(f"asset bootstrap failed: {exc}", file=sys.stderr)
        return 1

    if args.dry_run:
        print("\nDry run only; no files were written.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
