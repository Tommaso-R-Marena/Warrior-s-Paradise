"""Offline tests for the project's original mobile material tiles."""
from __future__ import annotations

import importlib.util
import json
import struct
import tempfile
import unittest
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class OriginalMobileMaterialsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.materials = load("wp_materials_test", ROOT / "tools/assets/generate_original_materials.py")
        cls.validate = load("wp_validate_material_test", ROOT / "tools/assets/validate_asset_provenance.py")

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def decode_rgb(self, payload, size):
        self.assertEqual(payload[:8], b"\x89PNG\r\n\x1a\n")
        pos = 8
        decoded = bytearray()
        while pos < len(payload):
            length = struct.unpack(">I", payload[pos:pos+4])[0]
            kind = payload[pos+4:pos+8]
            data = payload[pos+8:pos+8+length]
            crc = struct.unpack(">I", payload[pos+8+length:pos+12+length])[0]
            self.assertEqual(zlib.crc32(kind+data)&0xFFFFFFFF, crc)
            if kind == b"IHDR":
                self.assertEqual(struct.unpack(">II", data[:8]), (size, size))
            elif kind == b"IDAT":
                decoded.extend(zlib.decompress(data))
            pos += 12 + length
        self.assertEqual(len(decoded), size * (1+3*size))
        rows = [decoded[y * (1+3*size):(y+1)*(1+3*size)] for y in range(size)]
        for row in rows:
            self.assertEqual(row[0], 0)
            self.assertEqual(row[1:4], row[-3:])
        self.assertEqual(rows[0], rows[-1])
        return rows

    def test_all_surface_maps_are_reproducible_and_tileable(self):
        self.assertGreaterEqual(len(self.materials.PALETTE), 9)
        for surface in self.materials.PALETTE:
            images = self.materials.maps_for(surface, size=32)
            self.assertEqual(set(images), {"diff", "nor_gl", "rough"})
            for image in images.values():
                self.decode_rgb(image, 32)
            self.assertEqual(images, self.materials.maps_for(surface, size=32))

    def test_nonidentical_surface_colors(self):
        grass = self.materials.maps_for("ground_grass", size=32)["diff"]
        mud = self.materials.maps_for("ground_mud", size=32)["diff"]
        self.assertNotEqual(grass, mud)

    def test_checked_source_art_passes_provenance(self):
        self.materials.create(self.root, size=32)
        result = self.validate.validate_root(self.root)
        self.assertEqual(result["assets"], len(self.materials.PALETTE))
        self.assertEqual(result["importable_files"], len(self.materials.PALETTE)*3)

    def test_second_run_is_identical(self):
        self.materials.create(self.root, size=32)
        lock = self.root / "ExternalAssets/asset-lock.json"
        first = lock.read_bytes()
        self.materials.create(self.root, size=32)
        self.assertEqual(first, lock.read_bytes())

    def test_modified_texture_is_rejected(self):
        self.materials.create(self.root, size=32)
        texture = self.root / "ExternalAssets/wp_original/materials/ground_mud/ground_mud_diff.png"
        texture.write_bytes(texture.read_bytes()+b"tamper")
        with self.assertRaises(ValueError):
            self.materials.create(self.root, size=32)
        with self.assertRaises(self.validate.AssetIntegrityError):
            self.validate.validate_root(self.root)

    def test_untrusted_resolution_is_rejected(self):
        with self.assertRaises(ValueError):
            self.materials.create(self.root, size=30)
        with self.assertRaises(ValueError):
            self.materials.create(self.root, size=1024)

    def test_existing_lock_is_extended_not_reset(self):
        self.materials.create(self.root, size=32)
        lock = self.root / "ExternalAssets/asset-lock.json"
        assets = json.loads(lock.read_text())["assets"]
        self.assertTrue(all(f"original-material:{name}" in assets for name in self.materials.PALETTE))


if __name__ == "__main__":
    unittest.main()
