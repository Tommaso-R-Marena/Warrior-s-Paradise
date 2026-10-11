"""Contracts and dependency-free Node runtime tests for the real 3D browser build."""
from __future__ import annotations
import re
import shutil
import subprocess
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BROWSER = ROOT / "browser"
GAME3D = BROWSER / "3d"


class WebGL3DTests(unittest.TestCase):
    def test_cloudflare_fully_static_with_low_end_fallback(self):
        for name in ("index.html", "style.css", "game.js", "asset-loader.js", "battle-art.js", "battle-audio.js", "skinning.js", "material-atlas.js", "telemetry.js", "tactical-navigation.js", "battle-atmosphere.js", "normandy-dressing.js", "ww2-vehicles.js"):
            self.assertTrue((GAME3D / name).is_file(), name)
        html = (GAME3D / "index.html").read_text(encoding="utf-8")
        script = (GAME3D / "game.js").read_text(encoding="utf-8")
        root = (BROWSER / "index.html").read_text(encoding="utf-8")
        self.assertIn('href="./3d/"', root)
        self.assertIn('href="../index.html"', html)
        self.assertIn('getContext("webgl2"', script)
        self.assertIn('navigator.serviceWorker.register("../sw.js")', script)
        self.assertNotIn("https://cdn", html)
        self.assertNotIn("https://unpkg.com", html)
        self.assertNotIn("https://esm.sh", html)
        self.assertNotIn("PixelStreaming", script)

    def test_live_gameplay_uses_real_geometry_and_controls(self):
        s = (GAME3D / "game.js").read_text(encoding="utf-8")
        for feature in (
            "class Geometry", "gl.DEPTH_TEST", "gl.drawArrays(gl.TRIANGLES",
            "gl.uniformMatrix4fv", "function lookAt(", "function perspective(",
            "function buildWorld(", "function spawnEnemies(", "function shoot(",
            "function reload(", "function grenade(", "function heal(",
            "function simulate(", "function resize(", "function openLOS(",
            "requestAnimationFrame(frame)", "WP3DDiagnostics",
        ):
            self.assertIn(feature, s)
        self.assertIn('getContext("webgl2"', s)
        self.assertIn('Math.min(.20,rawMs/1000)', s)
        self.assertIn('Math.ceil(dt/.034)', s)

    def test_html_and_script_controls_match(self):
        html = (GAME3D / "index.html").read_text(encoding="utf-8")
        code = (GAME3D / "game.js").read_text(encoding="utf-8")
        ids = set(re.findall(r'id="([\w-]+)"', html))
        used = set(re.findall(r'\$\("([\w-]+)"\)', code))
        self.assertFalse(used-ids, f"Unknown game controls: {used-ids}")
        for essential in ("viewport", "menu", "deploy", "quality", "weapon", "left-stick",
                          "right-look", "fire", "aim", "reload", "grenade", "heal", "swap", "fallback"):
            self.assertIn(essential, ids)

    def test_node_gameplay_smoke(self):
        node = shutil.which("node")
        if not node:
            self.skipTest("Node is required to execute the WebGL2 runtime mock")
        for file in (GAME3D / "game.js", GAME3D / "asset-loader.js",
                     ROOT / "tools/tests/browser_webgl3d_smoke.js",
                     ROOT / "tools/tests/browser_cc0_glb_smoke.js",
                     ROOT / "tools/tests/browser_cc0_scene_smoke.js",
                     ROOT / "tools/tests/browser_quaternius_glb_smoke.js",
                     ROOT / "tools/tests/browser_visual_audio_smoke.js",
                     GAME3D / "battle-art.js", GAME3D / "battle-audio.js",
                     GAME3D / "skinning.js", GAME3D / "material-atlas.js",
                     ROOT / "tools/tests/browser_skinning_smoke.js",
                     ROOT / "tools/tests/browser_material_atlas_smoke.js",
                     ROOT / "tools/tests/browser_recorded_audio_smoke.js",
                     ROOT / "tools/tests/browser_telemetry_smoke.js",
                     ROOT / "tools/tests/browser_photo_material_smoke.js",
                     GAME3D / "telemetry.js", GAME3D / "tactical-navigation.js",
                     GAME3D / "battle-atmosphere.js",
                     GAME3D / "normandy-dressing.js",
                     GAME3D / "ww2-vehicles.js",
                     ROOT / "tools/tests/browser_ww2_vehicles_smoke.js",
                     ROOT / "tools/tests/browser_battle_atmosphere_smoke.js",
                     ROOT / "tools/tests/browser_tactical_navigation_smoke.js"):
            p = subprocess.run([node, "--check", str(file)], capture_output=True,
                               text=True, timeout=15, check=False)
            self.assertEqual(p.returncode, 0, p.stdout+p.stderr)
        vehicles = subprocess.run([node, str(ROOT / "tools/tests/browser_ww2_vehicles_smoke.js")],
                                  capture_output=True, text=True, timeout=20, check=False)
        self.assertEqual(vehicles.returncode, 0, vehicles.stdout + vehicles.stderr)
        atmosphere = subprocess.run([node, str(ROOT / "tools/tests/browser_battle_atmosphere_smoke.js")],
                                    capture_output=True, text=True, timeout=20, check=False)
        self.assertEqual(atmosphere.returncode, 0, atmosphere.stdout + atmosphere.stderr)
        self.assertIn("PASS battlefield atmosphere:", atmosphere.stdout)
        navigation = subprocess.run([node, str(ROOT / "tools/tests/browser_tactical_navigation_smoke.js")],
                                    capture_output=True, text=True, timeout=20, check=False)
        self.assertEqual(navigation.returncode, 0, navigation.stdout + navigation.stderr)
        self.assertIn("PASS tactical navigation:", navigation.stdout)
        p = subprocess.run([node, str(ROOT / "tools/tests/browser_webgl3d_smoke.js")],
                           capture_output=True, text=True, timeout=50, check=False)
        self.assertEqual(p.returncode, 0, p.stdout+p.stderr)
        self.assertIn("PASS WebGL2 mobile combat smoke:", p.stdout)
        art = subprocess.run([node, str(ROOT / "tools/tests/browser_cc0_glb_smoke.js")],
                             capture_output=True, text=True, timeout=30, check=False)
        self.assertEqual(art.returncode, 0, art.stdout + art.stderr)
        self.assertIn("PASS CC0 GLB source and geometry:", art.stdout)
        scene = subprocess.run([node, str(ROOT / "tools/tests/browser_cc0_scene_smoke.js")],
                               capture_output=True, text=True, timeout=50, check=False)
        self.assertEqual(scene.returncode, 0, scene.stdout + scene.stderr)
        self.assertIn("PASS CC0 integrated WebGL2 scenes:", scene.stdout)
        for path, marker in [
            ("browser_quaternius_glb_smoke.js", "PASS Quaternius farm artwork:"),
            ("browser_visual_audio_smoke.js", "PASS visual and spatial audio systems:"),
            ("browser_skinning_smoke.js", "PASS genuine CC0 58-bone glTF skinning:"),
            ("browser_material_atlas_smoke.js", "PASS tiled PBR material atlas:"),
            ("browser_recorded_audio_smoke.js", "PASS CC0 recorded battlefield audio:"),
            ("browser_telemetry_smoke.js", "PASS local frame and gameplay telemetry:"),
            ("browser_photo_material_smoke.js", "PASS real CC0 terrain photo bundle:"),
        ]:
            p = subprocess.run([node, str(ROOT / "tools/tests" / path)],
                               capture_output=True, text=True, timeout=50, check=False)
            self.assertEqual(p.returncode, 0, p.stdout + p.stderr)
            self.assertIn(marker, p.stdout)


if __name__ == "__main__":
    unittest.main()
