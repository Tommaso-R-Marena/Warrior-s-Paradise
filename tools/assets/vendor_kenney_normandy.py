#!/usr/bin/env python3
"""Reproducibly vendor six small Kenney CC0 GLB assets as Base64 transport files.

No trust in arbitrary GitHub branch HEADs: pinned commit, raw Git-blob hashes,
GLB magic/version/size checks, and strict byte caps. No dependency install.
"""
from __future__ import annotations
import base64
import hashlib
import json
import struct
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LOCK = ROOT / "tools/assets/kenney_normandy_lock.json"
TARGET = ROOT / "browser/3d/assets/kenney"
MANIFEST = TARGET / "NORMANDY_MANIFEST.json"


def main():
    lock = json.loads(LOCK.read_text(encoding="utf-8"))
    assert lock["license"] == "CC0-1.0"
    assert len(lock["models"]) == 6
    revision = lock["mirror_revision"]
    assert len(revision) == 40 and all(c in "0123456789abcdef" for c in revision)
    assets = []
    TARGET.mkdir(parents=True, exist_ok=True)
    for item in lock["models"]:
        name = item["name"]
        assert name in {"log-stacklarge", "stump-old", "rock-largeb", "fence-gate", "grass-large", "log-large"}
        url = (f"https://raw.githubusercontent.com/Hidencod/tge-assets/{revision}/"
               f"packs/nature-kit/{name}.glb")
        request = urllib.request.Request(url, headers={"User-Agent": "WarriorsParadise-CC0-Importer/1.0"})
        with urllib.request.urlopen(request, timeout=35) as response:
            raw = response.read(150_001)
        assert len(raw) == item["bytes"], f"{name}: expected {item['bytes']} bytes, got {len(raw)}"
        git_sha1 = hashlib.sha1(b"blob " + str(len(raw)).encode("ascii") + b"\x00" + raw).hexdigest()
        assert git_sha1 == item["sha1"], f"{name}: source blob differs from audit lock"
        assert len(raw) >= 28 and raw[:4] == b"glTF", f"{name}: invalid GLB magic"
        version, declared_length = struct.unpack_from("<II", raw, 4)
        assert version == 2 and declared_length == len(raw), f"{name}: invalid GLB header"
        encoded = base64.b64encode(raw).decode("ascii") + "\n"
        dest = TARGET / (name + ".glb.b64")
        dest.write_text(encoded, encoding="ascii")
        assets.append({
            "name": name,
            "repository_path": dest.relative_to(ROOT).as_posix(),
            "source_path": f"packs/nature-kit/{name}.glb",
            "source_git_blob_sha1": git_sha1,
            "source_bytes": len(raw),
            "source_sha256": hashlib.sha256(raw).hexdigest()
        })
        print(f"VERIFIED_CC0_ASSET {name} {len(raw)} B {git_sha1}")
    manifest = {
        "schema_version": 1,
        "title": lock["title"],
        "creator": "Kenney",
        "license": "CC0-1.0",
        "original_source": lock["source_page"],
        "mirror_source": "https://github.com/Hidencod/tge-assets",
        "mirror_revision": revision,
        "assets": assets
    }
    MANIFEST.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print("VERIFIED_CC0_NORMANDY_BUNDLE", len(assets), sum(a["source_bytes"] for a in assets))


if __name__ == "__main__":
    main()
