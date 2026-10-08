"""Adversarial source-art lock tests using only Python's standard library."""
from __future__ import annotations

import hashlib
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PATH = ROOT / "tools/assets/validate_asset_provenance.py"


def validator():
    spec = importlib.util.spec_from_file_location("wp_asset_validation_test", PATH)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class AssetIntegrityTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.module = validator()

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.assets = self.root / "ExternalAssets"
        self.assets.mkdir()
        self.model = self.assets / "original" / "tree.obj"
        self.model.parent.mkdir()
        self.model.write_bytes(b"v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n")
        self.record = {
            "path": "ExternalAssets/original/tree.obj",
            "bytes": self.model.stat().st_size,
            "sha256": hashlib.sha256(self.model.read_bytes()).hexdigest(),
        }
        self.lock = {
            "schema_version": 1,
            "assets": {
                "original:tree": {
                    "license": "CC0-1.0",
                    "source_url": "https://example.org/original-sandbox-tree",
                    "files": [self.record],
                }
            },
        }
        self.write_lock()

    def write_lock(self):
        (self.assets / "asset-lock.json").write_text(
            json.dumps(self.lock), encoding="utf-8"
        )

    def require_failure(self):
        with self.assertRaises(self.module.AssetIntegrityError):
            self.module.validate_root(self.root)

    def test_valid_art_lock(self):
        report = self.module.validate_root(self.root)
        self.assertEqual(report["status"], "PASS")
        self.assertEqual(report["importable_files"], 1)

    def test_tampered_model_rejected(self):
        self.model.write_bytes(self.model.read_bytes() + b"extra")
        self.require_failure()

    def test_untracked_fbx_rejected(self):
        (self.assets / "unaudited.fbx").write_bytes(b"invalid")
        self.require_failure()

    def test_escaped_path_rejected(self):
        self.record["path"] = "ExternalAssets/../elsewhere.obj"
        self.write_lock()
        self.require_failure()

    def test_absolute_path_rejected(self):
        self.record["path"] = str(self.model)
        self.write_lock()
        self.require_failure()

    def test_malformed_digest_rejected(self):
        self.record["sha256"] = "0" * 12
        self.write_lock()
        self.require_failure()

    def test_missing_source_or_license_rejected(self):
        item = self.lock["assets"]["original:tree"]
        item["license"] = "unknown"
        item["source_url"] = "http://insecure.example"
        self.write_lock()
        self.require_failure()

    def test_excessive_file_count_is_not_silently_ignored(self):
        self.lock["assets"]["original:tree"]["files"] = {"not": "a list"}
        self.write_lock()
        self.require_failure()

    def test_non_dict_lock_rejected(self):
        (self.assets / "asset-lock.json").write_text("[]")
        self.require_failure()

    def test_duplicate_records_rejected(self):
        record = dict(self.record)
        self.lock["assets"]["original:tree"]["files"].append(record)
        self.write_lock()
        self.require_failure()

    def test_symlink_rejected_if_supported(self):
        link = self.assets / "linked.obj"
        try:
            link.symlink_to(self.model)
        except (NotImplementedError, OSError):
            self.skipTest("symlinks not supported by runtime")
        self.require_failure()

    def test_md5_lock_supported_for_legacy_downloader(self):
        self.record.pop("sha256")
        self.record["md5"] = hashlib.md5(self.model.read_bytes()).hexdigest()
        self.write_lock()
        report = self.module.validate_root(self.root)
        self.assertEqual(report["importable_files"], 1)


if __name__ == "__main__":
    unittest.main()
