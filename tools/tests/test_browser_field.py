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
        for name in ("index.html", "style.css", "game.js", "sw.js", "manifest.webmanifest", "icon.svg"):
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
        for asset in ("index.html", "style.css", "game.js", "manifest.webmanifest", "icon.svg"):
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
        for name in ("game.js", "sw.js"):
            proc = subprocess.run(
                [node, "--check", str(BROWSER / name)],
                capture_output=True, text=True, check=False, timeout=10,
            )
            self.assertEqual(proc.returncode, 0, proc.stderr)


if __name__ == "__main__":
    unittest.main()
