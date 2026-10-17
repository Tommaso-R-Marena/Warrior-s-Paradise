#!/usr/bin/env python3
"""Offline quantized baked-mesh compiler. Standard-library only; no server at playtime.
Input JSON from Blender exporter contains 9 floats per expanded triangle vertex:
xyz, normalized nx/ny/nz, linear RGB * precomputed diffuse AO.
The packed format uses 12 bytes per vertex with one common local bbox for all LODs.
"""
from __future__ import annotations

import argparse
import base64
import hashlib
import json
import math
import re
import struct
from pathlib import Path

LICENSES = {"CC0-1.0", "CC-BY-4.0", "CC-BY-SA-4.0", "original"}
MAX_VERTICES = 180000
MAX_BYTES = 4 * 1024 * 1024
ID = re.compile(r"^[a-z0-9][a-z0-9_-]{0,63}$")


def fnv32(data: bytes) -> str:
    h = 0x811C9DC5
    for b in data:
        h = ((h ^ b) * 0x01000193) & 0xFFFFFFFF
    return f"{h:08x}"


def fail(message: str) -> None:
    raise ValueError("baked mesh: " + message)


def check_vertices(vertices: list[float]) -> None:
    if len(vertices) % 27 or not 27 <= len(vertices) <= MAX_VERTICES * 9:
        fail("triangle list size outside cap or malformed")
    if any(not isinstance(v, (int, float)) or not math.isfinite(v) for v in vertices):
        fail("non-finite positions/normals/color")
    for i in range(0, len(vertices), 9):
        pos = vertices[i:i+3]
        norm = vertices[i+3:i+6]
        color = vertices[i+6:i+9]
        if any(abs(p) >= 250 for p in pos):
            fail("model coordinates exceed local bounds")
        if not 0.85 <= math.sqrt(sum(v*v for v in norm)) <= 1.15:
            fail("bad normal")
        if any(v < 0 or v > 1 for v in color):
            fail("color outside normalized [0,1]")


def compile_mesh(doc: dict) -> dict:
    mesh_id = doc.get("id")
    source = doc.get("source")
    if not isinstance(mesh_id, str) or not ID.fullmatch(mesh_id):
        fail("unsafe mesh ID")
    if not isinstance(source, dict) or source.get("license") not in LICENSES:
        fail("missing approved provenance/license")
    if not isinstance(source.get("author"), str) or not (0 < len(source["author"]) < 150):
        fail("missing author")
    raw = {tier: doc.get(tier) for tier in ("medium", "high")}
    for tier, values in raw.items():
        if not isinstance(values, list):
            fail("missing " + tier + " triangle vertices")
        check_vertices(values)
    all_pos = [values[i:i+3] for values in raw.values() for i in range(0, len(values), 9)]
    lower = [min(p[axis] for p in all_pos) for axis in range(3)]
    upper = [max(p[axis] for p in all_pos) for axis in range(3)]
    if any(not -250 < p < 250 for p in lower + upper):
        fail("invalid bounds")
    lods = {}
    for tier, values in raw.items():
        buf = bytearray()
        for i in range(0, len(values), 9):
            p, n, rgb = values[i:i+3], values[i+3:i+6], values[i+6:i+9]
            q = [round(65535 * (p[axis] - lower[axis]) /
                       (upper[axis] - lower[axis])) if upper[axis] != lower[axis] else 0
                 for axis in range(3)]
            nrm = math.sqrt(sum(v*v for v in n))
            ni = [max(-127, min(127, round(v / nrm * 127))) for v in n]
            ci = [max(0, min(255, round(v * 255))) for v in rgb]
            buf.extend(struct.pack("<3H3b3B", *q, *ni, *ci))
        if len(buf) != len(values) // 9 * 12 or len(buf) > MAX_BYTES:
            fail("packed mesh exceeds 4 MiB")
        lods[tier] = {"vertices": len(values)//9,
                      "data": base64.b64encode(buf).decode("ascii"),
                      "fnv32": fnv32(buf)}
    return {"schema": "wp-baked-mesh-v1", "id": mesh_id,
            "source": source, "bounds": {"min": lower, "max": upper},
            "lods": lods}


def verify_pack(doc: dict) -> dict:
    if doc.get("schema") != "wp-baked-mesh-v1":
        fail("bad pack schema")
    bounds = doc.get("bounds", {})
    lower, upper = bounds.get("min", []), bounds.get("max", [])
    if len(lower) != 3 or len(upper) != 3 or any(not math.isfinite(x) for x in lower + upper):
        fail("bad pack bounds")
    result = {"id": doc["id"], "tiers": {}}
    for tier in ("medium", "high"):
        lod = doc["lods"][tier]
        blob = base64.b64decode(lod["data"], validate=True)
        if len(blob) != lod["vertices"]*12 or lod["vertices"] % 3:
            fail("bad packed vertex count")
        if fnv32(blob) != lod["fnv32"]:
            fail("checksum mismatch")
        for i in range(0, len(blob), 12):
            a, b, c, x, y, z, red, green, blue = struct.unpack_from("<3H3b3B", blob, i)
            if not 0.2 < math.sqrt(x*x + y*y + z*z)/127 < 1.8:
                fail("invalid decoded normal")
            if any(not -250 < lower[axis] + q / 65535*(upper[axis]-lower[axis]) < 250
                   for axis, q in enumerate((a,b,c))):
                fail("decoded position out of bounds")
        result["tiers"][tier] = {"triangles": lod["vertices"]//3, "bytes": len(blob)}
    return result


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--input", type=Path, help="unpacked triangle JSON from Blender or artist")
    p.add_argument("--output", type=Path, help="packed browser mesh JSON")
    p.add_argument("--verify", type=Path, help="validate existing browser pack")
    args = p.parse_args()
    if args.verify:
        print(json.dumps(verify_pack(json.loads(args.verify.read_text(encoding="utf-8"))), sort_keys=True))
        return
    if not args.input or not args.output:
        p.error("--input and --output are both required for compilation")
    payload = args.input.read_bytes()
    doc = json.loads(payload)
    if "source" in doc:
        doc["source"]["input_sha256"] = hashlib.sha256(payload).hexdigest()
    packed = compile_mesh(doc)
    result = verify_pack(packed)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(packed, sort_keys=True, separators=(",", ":"))+"\n", encoding="utf-8")
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
