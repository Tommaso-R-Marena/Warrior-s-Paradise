#!/usr/bin/env python3
"""Generate original seamless, lightweight PBR-like surface maps without Pillow.

Produces base-color, tangent-space normal and linear roughness PNGs using only
Python stdlib. These starter surfaces make procedural terrain less uniform;
photogrammetry-quality mobile PBR requires actual authored assets and UE QA.

All files use deterministic seeds and join the same checksummed source lock.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import struct
import zlib

ROOT = Path(__file__).resolve().parents[2]
TILE_SIZE = 192
PALETTE = {
    "ground_mud": (91, 77, 54),
    "ground_grass": (79, 91, 59),
    "road_gravel": (116, 112, 98),
    "wall_stone": (130, 123, 110),
    "wall_brick": (135, 87, 74),
    "wall_plaster": (185, 177, 156),
    "wood_weathered": (126, 100, 73),
    "roof_tile": (137, 82, 63),
    "metal_rusted": (100, 87, 68),
}


def chunk(kind: bytes, data: bytes) -> bytes:
    return struct.pack(">I", len(data)) + kind + data + struct.pack(
        ">I", zlib.crc32(kind + data) & 0xFFFFFFFF)


def png(rows: list[bytes], size: int) -> bytes:
    if len(rows) != size or any(len(row) != 3 * size for row in rows):
        raise ValueError("Expected RGB rows of consistent size")
    data = b"".join(b"\x00" + row for row in rows)
    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", size, size, 8, 2, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(data, 8))
        + chunk(b"IEND", b"")
    )


def _clamp_channel(value: float) -> int:
    return int(max(0, min(255, round(value))))


def maps_for(surface: str, size: int = TILE_SIZE) -> dict[str, bytes]:
    if surface not in PALETTE:
        raise ValueError(surface)
    base = PALETTE[surface]
    seed = int(hashlib.sha256(surface.encode()).hexdigest()[:8], 16)
    rnd = random.Random(seed)
    grains = [rnd.uniform(-1, 1) for _ in range(size * size)]
    heights = [0.0] * (size * size)
    colors = [[0] * (size * 3) for _ in range(size)]
    rough = [[0] * (size * 3) for _ in range(size)]

    def noise(x: int, y: int) -> float:
        return grains[(y % size) * size + (x % size)]

    for y in range(size):
        for x in range(size):
            # Tileable frequencies make repeats more natural in an editor map.
            frequency = (
                noise(x // 5, y // 5) * .42
                + noise(x // 17, y // 17) * .29
                + noise(x, y) * .19
            )
            relief = frequency * 15.0
            darken = 0.0
            if surface == "wall_brick":
                mortar_y = y % 30
                stagger = 24 if (y // 30) % 2 else 0
                mortar_x = (x + stagger) % 48
                if mortar_y < 2 or mortar_x < 2:
                    darken = -53
                    relief -= 7
            elif surface == "wall_stone":
                mortar_y = y % 41
                mortar_x = (x + (y // 41 % 2) * 43) % 70
                if mortar_y < 3 or mortar_x < 3:
                    darken = -45
                    relief -= 9
            elif surface == "wood_weathered":
                if x % 36 <= 2:
                    darken = -36
                    relief -= 6
                relief += 7 * math.sin(x * .75 + noise(0, y // 8) * 3)
            elif surface == "roof_tile":
                if y % 31 < 3 or (x + (y // 31 % 2) * 27) % 54 < 3:
                    darken = -27
                    relief -= 6
            elif surface == "metal_rusted":
                if frequency > .13:
                    darken += 20
                    relief += 4
            elif surface == "ground_grass":
                relief += 10 * abs(noise(x, y))
            elif surface == "road_gravel":
                if noise(x, y) > .45:
                    relief += 16
                    darken += 21
            elif surface == "ground_mud":
                if frequency < -.14:
                    darken -= 18
                    relief -= 9

            heights[y * size + x] = relief
            rgb = [
                _clamp_channel(base[k] + relief * (.75 if k else 1.0) + darken)
                for k in range(3)
            ]
            colors[y][3*x:3*x+3] = rgb
            rough_value = _clamp_channel(
                (208 if surface not in {"metal_rusted", "roof_tile"} else 176)
                + frequency * 19
            )
            rough[y][3*x:3*x+3] = [rough_value] * 3

    normal = [[0] * (size * 3) for _ in range(size)]
    for y in range(size):
        for x in range(size):
            left = heights[y * size + (x-1) % size]
            right = heights[y * size + (x+1) % size]
            top = heights[((y-1) % size) * size + x]
            bottom = heights[((y+1) % size) * size + x]
            nx, ny, nz = -(right-left)*.019, -(bottom-top)*.019, 1.
            length = math.sqrt(nx*nx + ny*ny + nz*nz)
            rgb = [
                _clamp_channel(127.5 + (nx / length) * 127.5),
                _clamp_channel(127.5 + (ny / length) * 127.5),
                _clamp_channel(127.5 + (nz / length) * 127.5),
            ]
            normal[y][x*3:x*3+3] = rgb

    return {
        "diff": png([bytes(row) for row in colors], size),
        "nor_gl": png([bytes(row) for row in normal], size),
        "rough": png([bytes(row) for row in rough], size),
    }


def create(root: Path, *, size: int = TILE_SIZE) -> dict:
    if size < 32 or size > 512 or size % 8:
        raise ValueError("Texture tile size must be divisible by 8 and in 32..512")
    root = root.resolve()
    source = root / "ExternalAssets"
    lock_path = source / "asset-lock.json"
    if lock_path.exists():
        if lock_path.is_symlink():
            raise ValueError("Refusing symlinked provenance lock")
        lock = json.loads(lock_path.read_text(encoding="utf-8"))
        if not isinstance(lock, dict) or lock.get("schema_version") != 1 or not isinstance(lock.get("assets"), dict):
            raise ValueError("Incompatible source-art lock")
    else:
        lock = {"schema_version": 1, "assets": {}}
    written = {}
    for surface in PALETTE:
        outputs = maps_for(surface, size)
        records = []
        for channel, payload in outputs.items():
            filename = f"{surface}_{channel}.png"
            path = source / "wp_original" / "materials" / surface / filename
            if path.is_symlink():
                raise ValueError(f"Linked texture is forbidden: {path}")
            if path.exists() and path.read_bytes() != payload:
                raise ValueError(f"Refusing to overwrite modified original texture: {path}")
            path.parent.mkdir(parents=True, exist_ok=True)
            if not path.exists():
                path.write_bytes(payload)
            records.append({
                "path": path.relative_to(root).as_posix(),
                "bytes": len(payload),
                "sha256": hashlib.sha256(payload).hexdigest(),
            })
        item = {
            "id": f"wp_original_mat_{surface}",
            "name": f"Original procedural PBR-style {surface}",
            "source": "Warrior's Paradise original texture generator",
            "source_url": "https://github.com/Tommaso-R-Marena/Warrior-s-Paradise",
            "license": "Project-owned-original",
            "purpose": "Mobile-friendly tiling surface; edit/replace before high-fidelity release",
            "files": records,
        }
        prior = lock["assets"].get(f"original-material:{surface}")
        if prior is not None and prior != item:
            raise ValueError("Source material provenance changed unexpectedly: " + surface)
        lock["assets"][f"original-material:{surface}"] = item
        written[surface] = {k: len(v) for k,v in outputs.items()}
    lock_path.parent.mkdir(parents=True, exist_ok=True)
    lock_path.write_text(json.dumps(lock, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return written


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--size", type=int, default=TILE_SIZE)
    options = parser.parse_args()
    print(json.dumps({"status": "PASS", "surfaces": create(options.root, size=options.size)}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
