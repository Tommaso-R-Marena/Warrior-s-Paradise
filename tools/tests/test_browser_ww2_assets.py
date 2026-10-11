"""Provenance and no-external-runtime checks for actual WW2 GLB imports."""
from __future__ import annotations
import base64, hashlib, json, struct, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
FOLDER=ROOT/"browser/3d/assets/oga"
EXPECTED={
 "axis_ww2":("CC0-1.0","nisu","a8e2f5a5777d1c9d4c18f76922d9737b88be717a455a65336a8772deb53729cb"),
 "gaz67_jeep":("CC0-1.0","artie31","e2993b7cc36f663ceb5c9f9b563ddbb09293571f5bdaae3aec1bdcc7fe4a4110"),
 "panzer_study":("CC0-1.0","hubahuba","4437922d50043d53a8c4a104a240da0adc11fecf229961ab3a5ccf7eea906c5c"),
 "willys_mb":("CC-BY-SA-3.0","Aligned Games",None),
 "allied_ww2":("CC-BY-3.0","knik1985",None)
}
class HistoricalWW2Models(unittest.TestCase):
 def test_hashes_licenses_and_valid_gltf(self):
  manifest=json.loads((FOLDER/"WW2_MANIFEST.json").read_text(encoding="utf-8"))
  rows={x["id"]:x for x in manifest["assets"]}
  self.assertTrue(EXPECTED.keys()<=rows.keys())
  for name,(license,author,sha) in EXPECTED.items():
   with self.subTest(name=name):
    rec=rows[name]
    self.assertEqual(rec["license"],license)
    self.assertEqual(rec["creator"],author)
    if sha:self.assertEqual(rec["original_package_sha256"],sha)
    else:self.assertEqual(len(rec["original_package_sha256"]),64)
    raw=base64.b64decode((ROOT/rec["model_file"]).read_text("ascii").strip(),validate=True)
    self.assertEqual(hashlib.sha256(raw).hexdigest(),rec["derived_sha256"])
    self.assertEqual(len(raw),rec["derived_glb_bytes"])
    self.assertEqual(raw[:4],b"glTF")
    ver,size=struct.unpack_from("<II",raw,4)
    self.assertEqual((ver,size),(2,len(raw)))
    json_len,json_type=struct.unpack_from("<II",raw,12)
    self.assertEqual(json_type,0x4e4f534a)
    doc=json.loads(raw[20:20+json_len])
    self.assertFalse(doc.get("extensionsRequired"))
    self.assertTrue(doc.get("meshes"))
    if name in ("axis_ww2","allied_ww2"):
     self.assertEqual(len(doc.get("skins",[])),1,"Actual skin missing")
     self.assertLessEqual(len(doc["skins"][0]["joints"]),96)
     if name=="allied_ww2":
      self.assertGreaterEqual(len(doc.get("animations",[])),2,"Allied running/walking clips missing")
      self.assertGreaterEqual(rec["animations"],2)
    else:
     tris=sum((doc["accessors"][p["indices"]]["count"]
        if "indices" in p else doc["accessors"][p["attributes"]["POSITION"]]["count"])/3
        for mesh in doc["meshes"] for p in mesh["primitives"])
     self.assertLessEqual(tris,3000,"Vehicle LOD exceeds responsiveness budget")
 def test_attribution_of_any_cc_by_3_source(self):
  rows=json.loads((FOLDER/"WW2_MANIFEST.json").read_text())["assets"]
  ui=(ROOT/"browser/3d/index.html").read_text()
  for rec in rows:
   if rec["license"].startswith("CC-BY"):
    self.assertIn(rec["creator"],ui)
    self.assertIn(rec["source_page"],ui)
if __name__=="__main__":unittest.main()
