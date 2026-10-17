"""Offline browser art production invariants; runs without Blender/graphics GPU."""
from __future__ import annotations

import importlib.util
import json
import sys
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
ASSETS=ROOT/"tools"/"assets"
sys.path.insert(0,str(ASSETS))
from compile_baked_art import compile_mesh, verify_pack, fnv32
from generate_baked_facade import generate, SOURCE, DEST


class BakedArt(unittest.TestCase):
    def test_published_mesh_is_reproducible(self):
        source={"id":"normandy-stonework","source":SOURCE,
                "medium":generate(False),"high":generate(True)}
        exact=compile_mesh(source)
        published=json.loads(DEST.read_text(encoding="utf-8"))
        if exact != published:
            import base64
            for tier in ("medium","high"):
                a=base64.b64decode(exact["lods"][tier]["data"])
                b=base64.b64decode(published["lods"][tier]["data"])
                diffs=[(i,a[i],b[i]) for i in range(min(len(a),len(b))) if a[i]!=b[i]]
                print("BAKED_REPRO_DIFF",tier,len(a),len(b),"first=",diffs[:12])
        self.assertEqual(exact,published,"checked-in source and published art diverge")
        report=verify_pack(published)
        self.assertEqual(report["tiers"]["medium"]["triangles"],108)
        self.assertEqual(report["tiers"]["high"]["triangles"],1060)

    def test_checksum_rejects_damage(self):
        packet=compile_mesh({"id":"asset1","source":{"author":"Owner","license":"original"},
           "medium":generate(False),"high":generate(True)})
        packet["lods"]["high"]["fnv32"]="deadbeef"
        with self.assertRaisesRegex(ValueError,"checksum"):
            verify_pack(packet)

    def test_fail_closed_unlicensed_and_nonfinite(self):
        good={"id":"asset1","source":{"author":"Owner","license":"original"},
              "medium":generate(False),"high":generate(True)}
        for license in ("", "CC-BY-NC", "unknown"):
            good["source"]["license"]=license
            with self.assertRaises(ValueError):
                compile_mesh(good)
        good["source"]["license"]="original"
        good["high"][0]=float("nan")
        with self.assertRaisesRegex(ValueError,"non-finite"):
            compile_mesh(good)

    def test_lod_not_duplicated_and_signed_geometry(self):
        original=generate(False)
        detailed=generate(True)
        self.assertLess(len(original),len(detailed))
        self.assertEqual(len(original)%27,0)
        self.assertEqual(len(detailed)%27,0)
        self.assertTrue(all(-250 < v < 250 for v in detailed))
        self.assertEqual(fnv32(bytes([0,1,2,3])), "c3aa51b1")

    def test_manifest_existing_physical_anchors(self):
        doc=json.loads((DEST.parent/"scene.json").read_text(encoding="utf-8"))
        self.assertEqual(doc["schema"],"wp-baked-scene-v1")
        boxes=[[-24,-12,11,9],[-2,-25,9,8],[18,-12,11,11]]
        for placement in doc["groups"][0]["instances"]:
            self.assertEqual(placement["collider"],"existing-building")
            self.assertTrue(any(abs(placement["x"]-b[0]) <= b[2]/2+.8 and
                abs(placement["z"]-b[1]) <= b[3]/2+.8 for b in boxes))


if __name__=="__main__":
    unittest.main()
