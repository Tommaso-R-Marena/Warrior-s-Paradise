#!/usr/bin/env python3
"""Rebuild CC0 ambientCG albedo, OpenGL normal and roughness JPEGs.

Produces self-hosted base64 transport files for Cloudflare Pages (not a binary
manifest or external play-time CDN). Original media always comes from the
CC0 ambientCG mirror; SHA-256 provenance is recorded for each downloaded
source and exact optimized output. Run on standard GitHub Actions with Pillow.

Usage: python tools/scripts/build_cc0_pbr_assets.py
"""
from __future__ import annotations

import base64
import hashlib
import io
import json
from pathlib import Path
from urllib.request import Request, urlopen

from PIL import Image, ImageOps, UnidentifiedImageError

ROOT = Path(__file__).resolve().parents[2]
TARGET = ROOT / "browser" / "3d" / "assets" / "materials"
BASE = (
    "https://raw.githubusercontent.com/petroulacl/fps-buildings-env-kit/"
    "main/environment/ground-textures/ambientcg"
)
SOURCES = {
    "grass": ("Grass004", "Grass004_2K-JPG"),
    "mud": ("Ground037", "Ground037_2K-JPG"),
    "brick": ("Bricks066", "Bricks066_2K-JPG"),
}
KINDS = {"photo": "Color", "normal": "NormalGL", "roughness": "Roughness"}
RESOLUTION = 512
MAX_SOURCE_BYTES = 32_000_000


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def fetch_url(url: str) -> bytes:
    request = Request(url, headers={"User-Agent": "WarriorsParadise-CC0-AssetBuild/1.0"})
    with urlopen(request, timeout=35) as response:
        payload = response.read(MAX_SOURCE_BYTES + 1)
    if not 100 < len(payload) <= MAX_SOURCE_BYTES:
        raise ValueError(f"source empty or over size limit ({len(payload)} bytes): {url}")
    if not payload.startswith(b"\xff\xd8") or not payload.endswith(b"\xff\xd9"):
        raise ValueError(f"not a complete JPEG: {url}")
    return payload


def optimize(original: bytes, kind: str, size: int = RESOLUTION) -> bytes:
    with Image.open(io.BytesIO(original)) as image:
        image.load()
        if image.width < size or image.height < size:
            raise ValueError("source smaller than target resolution")
        image = ImageOps.exif_transpose(image)
        # Normal maps must preserve RGB channels. Roughness is a scalar.
        image = image.convert("L" if kind == "roughness" else "RGB")
        image = image.resize((size, size), Image.Resampling.LANCZOS)
        output = io.BytesIO()
        image.save(output, "JPEG", quality=87 if kind == "normal" else 82,
                   optimize=True, subsampling=0 if kind == "normal" else 2)
        data = output.getvalue()
    if not data.startswith(b"\xff\xd8") or not data.endswith(b"\xff\xd9"):
        raise ValueError("image encoder emitted an incomplete JPEG")
    return data


def main() -> None:
    TARGET.mkdir(parents=True, exist_ok=True)
    manifest_path = TARGET / "MANIFEST.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    manifest["source"] = "ambientCG CC0 assets, bundled from petroulacl/fps-buildings-env-kit"
    manifest["license"] = "CC0-1.0"
    manifest["texture_resolution"] = [RESOLUTION, RESOLUTION]
    manifest["medium_texture_resolution"] = [256, 256]
    manifest["processing"] = (
        "Original CC0 ambientCG JPEGs are downloaded directly from the source "
        "repository, independently verified by SHA-256, and downsampled with "
        "Pillow 11.3.0 to 512x512 optimized JPEGs. Avoid browser screenshots."
    )
    manifest["warning"] = (
        "Albedo, OpenGL tangent normal, and scalar roughness are original "
        "source-derived maps. The browser lighting approximation is not "
        "a calibrated metallic/roughness PBR implementation."
    )
    manifest["pbr_notes"] = (
        "Real artist-authored OpenGL normal and roughness maps accompany each "
        "albedo image. Shader uses approximate non-tangent frame; this is not "
        "a calibrated full metallic/roughness lighting implementation."
    )
    assets = {item["name"]: item for item in manifest["assets"]}
    for name, (source_id, pack) in SOURCES.items():
        entry = assets[name]
        entry["size"] = [RESOLUTION, RESOLUTION]
        entry["original_id"] = source_id
        output_records = {}
        for kind, suffix in KINDS.items():
            remote = f"{BASE}/{pack}/{pack}_{suffix}.jpg"
            raw = fetch_url(remote)
            if kind == "photo":
                photo_source = raw
            previously = entry.get("maps", {}).get(kind, {})
            prior_hash = previously.get("source_sha256")
            if prior_hash and sha256(raw) != prior_hash:
                raise ValueError(f"Upstream CC0 source changed unexpectedly: {remote}")
            optimized = optimize(raw, kind)
            dest_name = f"{name}_{kind}.jpg.b64"
            (TARGET / dest_name).write_text(
                base64.b64encode(optimized).decode("ascii"), encoding="ascii"
            )
            output_records[kind] = {
                "asset": dest_name,
                "source_url": remote,
                "source_sha256": sha256(raw),
                "optimized_sha256": sha256(optimized),
                "optimized_bytes": len(optimized),
            }
            print(f"{name:6} {kind:9}: {len(optimized):7} bytes  {dest_name}")
        # A separate 256px albedo is cheaper for Medium; High alone loads
        # the 512px high-fidelity albedo/normal/roughness triplet.
        medium = optimize(photo_source, "photo", size=256)
        medium_name = f"{name}_medium.jpg.b64"
        (TARGET / medium_name).write_text(
            base64.b64encode(medium).decode("ascii"), encoding="ascii"
        )
        entry["medium_map"] = {
            "asset": medium_name,
            "source_url": output_records["photo"]["source_url"],
            "source_sha256": output_records["photo"]["source_sha256"],
            "optimized_sha256": sha256(medium),
            "optimized_bytes": len(medium),
        }
        entry["asset"] = output_records["photo"]["asset"]
        entry["normal_asset"] = output_records["normal"]["asset"]
        entry["roughness_asset"] = output_records["roughness"]["asset"]
        entry["maps"] = output_records
    manifest_path.write_text(
        json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )


if __name__ == "__main__":
    main()
