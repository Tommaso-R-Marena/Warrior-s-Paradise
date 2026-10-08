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
        for name in ("index.html", "style.css", "game.js", "asset-loader.js"):
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
        self.assertIn('Math.min(.045', s)

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
                     ROOT / "tools/tests/browser_cc0_scene_smoke.js"):
            p = subprocess.run([node, "--check", str(file)], capture_output=True,
                               text=True, timeout=15, check=False)
            self.assertEqual(p.returncode, 0, p.stdout+p.stderr)
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


if __name__ == "__main__":
    unittest.main()
