"""Static browser edition contract tests (no browsers or Node dependencies needed).

Optional Node syntax test runs when Node.js is installed. Real mobile browser
testing remains a separate acceptance step.
"""
from __future__ import annotations

import json
import re
import shutil
import subprocess
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BROWSER = ROOT / "browser"


class BrowserEditionTests(unittest.TestCase):
    def test_static_site_is_self_contained(self) -> None:
        for name in ("index.html", "style.css", "game.js", "visuals.js", "sw.js", "manifest.webmanifest", "icon.svg"):
            self.assertTrue((BROWSER / name).is_file(), name)
        html = (BROWSER / "index.html").read_text(encoding="utf-8")
        self.assertNotIn("https://cdn", html)
        self.assertIn('src="./game.js"', html)
        self.assertIn('href="./style.css"', html)

    def test_declared_controls_exist(self) -> None:
        html = (BROWSER / "index.html").read_text(encoding="utf-8")
        js = (BROWSER / "game.js").read_text(encoding="utf-8")
        ids = set(re.findall(r'id="([\w-]+)"', html))
        used = set(re.findall(r'\$\("([\w-]+)"\)', js))
        missing = used - ids
        self.assertFalse(missing, f"Missing HTML controls: {sorted(missing)}")
        for control in ("world", "radar", "move-pad", "look-zone", "fire-btn", "reload-btn", "heal-btn", "switch-btn", "quality", "start-btn"):
            self.assertIn(control, ids)

    def test_pwa_assets_cover_all_runtime_files(self) -> None:
        manifest = json.loads((BROWSER / "manifest.webmanifest").read_text(encoding="utf-8"))
        self.assertEqual(manifest["display"], "standalone")
        self.assertEqual(manifest["start_url"], "./")
        service = (BROWSER / "sw.js").read_text(encoding="utf-8")
        for asset in ("index.html", "style.css", "game.js", "visuals.js", "manifest.webmanifest", "icon.svg"):
            self.assertIn("./" + asset, service)

    def test_software_renderer_needs_no_webgl(self) -> None:
        js = (BROWSER / "game.js").read_text(encoding="utf-8").lower()
        self.assertIn('getcontext("2d"', js)
        self.assertNotIn('getcontext("webgl"', js)
        self.assertNotIn("three.js", js)
        self.assertIn("function trace(", js)

    def test_browser_game_has_local_gameplay_loop(self) -> None:
        js = (BROWSER / "game.js").read_text(encoding="utf-8")
        for term in ("function tick(", "function fire(", "function reload(", "function heal(", "function switchSoldier(", "function update(", "function renderWorld(", "function spawnDefenders(", "function renderRadar("):
            self.assertIn(term, js)

    def test_node_can_parse_javascript_if_available(self) -> None:
        node = shutil.which("node")
        if not node:
            self.skipTest("Node.js unavailable; JS is additionally parsed at development time")
        for name in ("game.js", "visuals.js", "sw.js"):
            proc = subprocess.run(
                [node, "--check", str(BROWSER / name)],
                capture_output=True, text=True, check=False, timeout=10,
            )
            self.assertEqual(proc.returncode, 0, proc.stderr)


    def test_enhanced_mobile_controls_and_renderer(self) -> None:
        html = (BROWSER / "index.html").read_text(encoding="utf-8")
        js = (BROWSER / "game.js").read_text(encoding="utf-8")
        for control in ("ads-btn", "frag-btn", "smoke-btn", "order-btn", "aim-ring", "loadout", "grenades"):
            self.assertIn(f'id="{control}"', html)
        for feature in ("function smokeBlocks(", "function throwGrenade(", "function toggleOrders(", "function moveSmart(", "function setADS("):
            self.assertIn(feature, js)
        self.assertIn('src="./visuals.js"', html)
        self.assertTrue((BROWSER / "visuals.js").is_file())
        self.assertIn("visuals.setup()", js)

    def test_multiple_maps_modes_and_pitch(self) -> None:
        html = (BROWSER / "index.html").read_text(encoding="utf-8")
        js = (BROWSER / "game.js").read_text(encoding="utf-8")
        visuals = (BROWSER / "visuals.js").read_text(encoding="utf-8")
        for selector in ("battlefield", "difficulty", "mode", "sensitivity"):
            self.assertIn(f'id="{selector}"', html)
        for mode in ("assault", "defense"):
            self.assertIn(f'value="{mode}"', html)
        for battlefield in ("farm", "town", "forest"):
            self.assertIn(f'value="{battlefield}"', html)
        self.assertIn('value="ultra"', html)
        for feature in ("function setBattlefield(", "function spawnDefenseWave(",
                        "function resupplyBetweenWaves(", "function adjustRendering(",
                        "function playGunshot("):
            self.assertIn(feature, js)
        self.assertIn("player.pitch", js)
        self.assertIn("touches.lastLookY", js)
        self.assertIn("function floor(", visuals)
        self.assertIn("pitch=0", visuals)

    def test_mobile_device_profiles_are_render_only(self) -> None:
        profiles = (ROOT / "Config/DefaultDeviceProfiles.ini").read_text(encoding="utf-8")
        android = (ROOT / "Config/Android/AndroidEngine.ini").read_text(encoding="utf-8")
        for level in ("Android_Low", "Android_Mid", "Android_High"):
            self.assertIn(f"[{level} DeviceProfile]", profiles)
        self.assertIn("r.Mobile.ShadingPath=0", android)
        self.assertIn("r.Shadow.Virtual.Enable=0", android)

    def test_node_browser_runtime_smoke_if_available(self) -> None:
        node = shutil.which("node")
        if not node:
            self.skipTest("Node.js unavailable; runtime test requires Node")
        smoke = ROOT / "tools/tests/browser_runtime_smoke.js"
        self.assertTrue(smoke.is_file())
        proc = subprocess.run(
            [node, str(smoke)], capture_output=True, text=True,
            check=False, timeout=40,
        )
        self.assertEqual(proc.returncode, 0, proc.stderr + proc.stdout)
        self.assertIn("PASS 390x844", proc.stdout)
        self.assertIn("PASS 844x390", proc.stdout)
        self.assertIn("farm defense ultra", proc.stdout)
        self.assertIn("forest defense ultra", proc.stdout)
        self.assertIn("town assault sharp", proc.stdout)



if __name__ == "__main__":
    unittest.main()
