"""Deterministic geometry and provenance tests for original 3D scenery."""
from __future__ import annotations

import hashlib
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]


def load(name: str, file: Path):
    spec=importlib.util.spec_from_file_location(name, file)
    assert spec and spec.loader
    module=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class OriginalPropsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.generator=load("wp_original_props_test",ROOT/"tools/assets/generate_original_props.py")
        cls.validator=load("wp_original_validator_test",ROOT/"tools/assets/validate_asset_provenance.py")

    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root=Path(self.tmp.name)

    def test_eight_unique_meshes_with_valid_faces(self):
        self.assertEqual(len(self.generator.SLOTS),8)
        for slot in self.generator.SLOTS:
            mesh=self.generator.build(slot)
            self.assertGreater(len(mesh.vertices),8,slot)
            self.assertGreater(len(mesh.faces),10,slot)
            for a,b,c,material in mesh.faces:
                for idx in (a,b,c):
                    self.assertGreaterEqual(idx,1)
                    self.assertLessEqual(idx,len(mesh.vertices))
                self.assertIn(material,self.generator.PALETTE)
            obj=mesh.obj(slot)
            self.assertIn(f"mtllib wp_{slot}.mtl",obj)
            self.assertIn("\nf ",obj)

    def test_generated_source_art_passes_provenance_verification(self):
        details=self.generator.generate(self.root)
        self.assertEqual(set(details),set(self.generator.SLOTS))
        report=self.validator.validate_root(self.root)
        self.assertEqual(report["status"],"PASS")
        self.assertEqual(report["importable_files"],8)
        self.assertEqual(report["assets"],8)

    def test_generation_is_byte_stable(self):
        self.generator.generate(self.root)
        lock=self.root/"ExternalAssets/asset-lock.json"
        original=lock.read_bytes()
        self.generator.generate(self.root)
        self.assertEqual(original,lock.read_bytes())

    def test_preserves_existing_non_project_lock_assets(self):
        root=self.root/"ExternalAssets"
        root.mkdir()
        external=root/"custom"/"other.obj"
        external.parent.mkdir()
        external.write_bytes(b"v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n")
        lock={
            "schema_version":1,
            "assets":{"manual:other":{
                "license":"CC0-1.0",
                "source_url":"https://example.org/source",
                "files":[{
                    "path":"ExternalAssets/custom/other.obj",
                    "sha256":hashlib.sha256(external.read_bytes()).hexdigest(),
                    "bytes":external.stat().st_size,
                }]
            }}
        }
        (root/"asset-lock.json").write_text(json.dumps(lock),encoding="utf-8")
        self.generator.generate(self.root)
        updated=json.loads((root/"asset-lock.json").read_text())
        self.assertIn("manual:other",updated["assets"])
        self.assertEqual(self.validator.validate_root(self.root)["importable_files"],9)

    def test_rejects_modification_to_owned_generated_source(self):
        self.generator.generate(self.root)
        tree=self.root/"ExternalAssets/wp_original/tree/wp_tree.obj"
        tree.write_bytes(tree.read_bytes()+b"# hacked\n")
        with self.assertRaises(RuntimeError):
            self.generator.generate(self.root)
        with self.assertRaises(self.validator.AssetIntegrityError):
            self.validator.validate_root(self.root)

    def test_does_not_replace_incompatible_lock(self):
        lock=self.root/"ExternalAssets/asset-lock.json"
        lock.parent.mkdir()
        lock.write_text('{"schema_version":99,"assets":{}}')
        with self.assertRaises(ValueError):
            self.generator.generate(self.root)


if __name__=="__main__":
    unittest.main()
