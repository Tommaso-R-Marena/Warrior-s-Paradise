"""Regression checks for authored native UE map preparation and activation."""
from __future__ import annotations

import ast
import importlib.util
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TOOLS = ROOT / "tools/assets"


def load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load source module")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class NativeMobileSceneTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.activator = load_module("wp_activation_test", TOOLS / "activate_mobile_map.py")

    def test_invalid_unreal_map_path_rejected(self) -> None:
        for name in (
            "../Something", "/Engine/Maps/Entry",
            "/Game/WarriorParadise/Maps/../../Bad", "/Game/Other/Map",
        ):
            with self.assertRaises(ValueError):
                self.activator.asset_path_to_umap(name)

    def test_map_asset_path_resolves_inside_content(self) -> None:
        path = self.activator.asset_path_to_umap(
            "/Game/WarriorParadise/Maps/WP_NormandyMobile"
        )
        self.assertEqual(
            path,
            ROOT / "Content/WarriorParadise/Maps/WP_NormandyMobile.umap",
        )

    def test_activation_updates_only_editor_and_game_default(self) -> None:
        original = (ROOT / "Config/DefaultEngine.ini").read_text(encoding="utf-8")
        edited = self.activator.promote(
            original, "/Game/WarriorParadise/Maps/WP_NormandyMobile"
        )
        self.assertIn(
            "GameDefaultMap=/Game/WarriorParadise/Maps/WP_NormandyMobile",
            edited,
        )
        self.assertIn(
            "EditorStartupMap=/Game/WarriorParadise/Maps/WP_NormandyMobile",
            edited,
        )
        self.assertIn("ServerDefaultMap=/Engine/Maps/Entry", edited)
        self.assertEqual(
            self.activator.promote(edited, "/Engine/Maps/Entry"),
            original,
        )
        self.assertEqual(
            edited.count("GameDefaultMap="), 1,
        )

    def test_duplicate_config_keys_rejected(self) -> None:
        original = (ROOT / "Config/DefaultEngine.ini").read_text(encoding="utf-8")
        duplicate = original.replace(
            "GameDefaultMap=/Engine/Maps/Entry",
            "GameDefaultMap=/Engine/Maps/Entry\nGameDefaultMap=/Engine/Maps/Entry",
        )
        with self.assertRaises(ValueError):
            self.activator.promote(duplicate, "/Engine/Maps/Entry")

    def test_editor_script_is_python_parseable(self) -> None:
        for file in (
            "wp_build_mobile_village.py", "wp_prepare_native_mobile.py",
        ):
            path = ROOT / "Content/Python" / file
            ast.parse(path.read_text(encoding="utf-8"), filename=str(path))

    def test_editor_level_generator_requires_real_provenance_and_saves_level(self) -> None:
        code = (ROOT / "Content/Python/wp_build_mobile_village.py").read_text()
        for token in (
            "asset-lock.json", "WP_ALLOW_BLOCKOUT",
            "EditorLevelLibrary.spawn_actor_from_class",
            "EditorAssetLibrary.does_asset_exist",
            "editor.new_level", "editor.save_current_level",
            "NO_COLLISION", "random.Random(SEED)",
        ):
            self.assertIn(token, code)

    def test_one_step_pipeline_preserves_stage_order(self) -> None:
        source = (ROOT / "Content/Python/wp_prepare_native_mobile.py").read_text()
        files = (
            "wp_import_external_assets.py",
            "wp_build_cc0_materials.py",
            "wp_setup_free_profiles.py",
            "wp_build_mobile_village.py",
        )
        offsets = [source.index('stage("' + file) for file in files]
        self.assertEqual(offsets, sorted(offsets))

    def test_wrapper_checks_editor_and_map_output(self) -> None:
        source = (ROOT / "tools/prepare_native_mobile.ps1").read_text()
        for term in (
            "UnrealEditor-Cmd.exe", "-ExecutePythonScript=",
            "asset-lock.json", "Test-Path $Map",
            "activate_mobile_map.py",
        ):
            self.assertIn(term, source)


if __name__ == "__main__":
    unittest.main()
