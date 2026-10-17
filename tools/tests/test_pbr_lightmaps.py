"""PBR UV0/UV1 release checks without graphics or Blender."""
import ast
import base64
import json
import sys
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/assets"))
from compile_pbr_lightmaps import compile_mesh,verify,fnv32
ASSETS=ROOT/"browser/3d/assets/pbr"


class PackedPBRTests(unittest.TestCase):
    def test_real_published_mesh_and_irradiance_bytes(self):
        pack=json.loads((ASSETS/"normandy-brick-face.wppbr.json").read_text())
        self.assertEqual(pack["source"]["license"],"original")
        self.assertEqual(verify(pack),{"medium":2,"high":2350})
        light=json.loads((ASSETS/"normandy-brick-irradiance.wptex.json").read_text())
        tex=base64.b64decode(light["rgba"],validate=True)
        self.assertEqual(len(tex),64*64*4)
        self.assertEqual(fnv32(tex),light["fnv32"])
        self.assertGreater(len(set(tex[::4])),7,"irradiance map must not be uniform")

    def test_pack_writer_preserves_independent_uvs_and_provenance(self):
        face=[-1,0,0,0,0,1,0,.1,.18,.2,
               1,0,0,0,0,1,.8,.1,.77,.2,
               0,1,0,0,0,1,.4,.8,.50,.81]
        doc={"id":"sample1","source":{"license":"CC0-1.0","author":"Artist"},
             "medium":face,"high":face+face}
        result=compile_mesh(doc)
        self.assertEqual(verify(result),{"medium":1,"high":2})
        buf=base64.b64decode(result["lods"]["medium"]["data"])
        import struct
        item=struct.unpack_from("<3H3bB4H",buf)
        self.assertEqual(item[6],0)
        self.assertNotEqual(item[7],item[9],"surface UV must be independent of light UV")
        self.assertEqual(result["source"]["author"],"Artist")

    def test_reject_corrupt_invalid_source_and_nan(self):
        face=[0,0,0,0,0,1,0,0,0,0,
              1,0,0,0,0,1,1,0,1,0,
              0,1,0,0,0,1,0,1,0,1]
        doc={"id":"source","source":{"license":"original","author":"Me"},
             "medium":face.copy(),"high":face.copy()}
        pack=compile_mesh(doc)
        pack["lods"]["high"]["fnv32"]="0"*8
        with self.assertRaisesRegex(ValueError,"checksum"):verify(pack)
        doc["source"]["license"]="unlicensed"
        with self.assertRaises(ValueError):compile_mesh(doc)
        doc["source"]["license"]="original"
        doc["medium"][0]=float("inf")
        with self.assertRaises(ValueError):compile_mesh(doc)

    def test_blender_export_source_is_parseable(self):
        for script in ("compile_pbr_lightmaps.py","blender_export_pbr_lightmaps.py"):
            text=(ROOT/"tools/assets"/script).read_text(encoding="utf-8")
            ast.parse(text)
        blender=(ROOT/"tools/assets/blender_export_pbr_lightmaps.py").read_text()
        for token in ("WP_PBR_UV","WP_STATIC_LIGHT_UV","DIFFUSE","NORMAL",
                      "ROUGHNESS","DIRECT","INDIRECT","METALLIC" if False else "metallic_bake"):
            self.assertIn(token,blender)


if __name__=="__main__":
    unittest.main()
