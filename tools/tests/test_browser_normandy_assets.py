"""Verify actual bundled Kenney CC0 3D model bytes against upstream blob hashes."""
from __future__ import annotations
import base64
import hashlib
import json
import struct
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


class KenneyNormandyProvenanceTests(unittest.TestCase):
    def test_actual_cc0_models_pinned_and_integrity_checked(self):
        lock = json.loads((ROOT / "tools/assets/kenney_normandy_lock.json").read_text(encoding="utf-8"))
        manifest = json.loads((ROOT / "browser/3d/assets/kenney/NORMANDY_MANIFEST.json").read_text(encoding="utf-8"))
        self.assertEqual(lock["license"], "CC0-1.0")
        self.assertEqual(manifest["license"], "CC0-1.0")
        self.assertEqual(lock["mirror_revision"], manifest["mirror_revision"])
        self.assertEqual(lock["source_page"], "https://kenney.nl/assets/nature-kit")
        self.assertEqual(len(lock["models"]), 6)
        self.assertEqual(len(manifest["assets"]), 6)
        recorded = {x["name"]: x for x in manifest["assets"]}
        for entry in lock["models"]:
            with self.subTest(name=entry["name"]):
                asset = recorded[entry["name"]]
                self.assertEqual(asset["source_git_blob_sha1"], entry["sha1"])
                p = ROOT / asset["repository_path"]
                raw = base64.b64decode(p.read_text(encoding="ascii").strip(), validate=True)
                self.assertEqual(len(raw), entry["bytes"])
                self.assertEqual(raw[:4], b"glTF")
                version, declared_size = struct.unpack_from("<II", raw, 4)
                self.assertEqual(version, 2)
                self.assertEqual(declared_size, len(raw))
                git_blob = b"blob " + str(len(raw)).encode() + b"\x00" + raw
                self.assertEqual(hashlib.sha1(git_blob).hexdigest(), entry["sha1"])
                self.assertEqual(hashlib.sha256(raw).hexdigest(), asset["source_sha256"])

    def test_runtime_no_external_asset_deps_and_bounded_geometry(self):
        s = (ROOT / "browser/3d/normandy-dressing.js").read_text(encoding="utf-8")
        game = (ROOT / "browser/3d/game.js").read_text(encoding="utf-8")
        html = (ROOT / "browser/3d/index.html").read_text(encoding="utf-8")
        self.assertIn("LIMIT_TRIANGLES=4800", s)
        self.assertIn("LIMIT_INSTANCES=86", s)
        self.assertIn("WPNormandyDressing", game)
        self.assertIn('src="./normandy-dressing.js"', html)
        self.assertNotIn("https://", s)


if __name__ == "__main__":
    unittest.main()
