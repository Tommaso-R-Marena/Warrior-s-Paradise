#!/usr/bin/env python3
"""Download curated OGA 3D sources; inspect safely before Blender conversion.

Only models with reviewed source-page author/license metadata are admitted.
Hash original packages and export source audit manifest alongside derived GLBs.
"""
import base64, hashlib, io, json, os, pathlib, subprocess, sys, tempfile, urllib.request, zipfile
ROOT=pathlib.Path(__file__).resolve().parents[2]
SPECS=json.loads((ROOT/"tools/assets/ww2_open_art_sources.json").read_text())
OUT=ROOT/"browser/3d/assets/oga"
OUT.mkdir(parents=True,exist_ok=True)
WORK=pathlib.Path(tempfile.mkdtemp(prefix="wp-ww2-"))
records=[]
for spec in SPECS:
 try:
  name=spec["id"];file=spec["file"];target=WORK/name;target.mkdir()
  print("FETCH_CC_LICENSED",name,spec["source_page"],flush=True)
  req=urllib.request.Request(spec["url"],headers={"User-Agent":"Mozilla/5.0 (compatible; WarriorParadiseCC0Importer/1.0)"})
  with urllib.request.urlopen(req,timeout=50) as res: raw=res.read(spec["max_source_bytes"]+1)
  assert 100<len(raw)<=spec["max_source_bytes"],"source size outside reviewed bound"
  digest=hashlib.sha256(raw).hexdigest()
  (target/file).write_bytes(raw)
  candidates=[]
  if raw[:2]==b"PK":
   with zipfile.ZipFile(io.BytesIO(raw)) as z:
    for item in z.infolist():
     p=pathlib.PurePosixPath(item.filename.replace("\\","/"))
     assert not p.is_absolute() and ".." not in p.parts,"zip path traversal"
     assert item.file_size<=40_000_000,"oversized source entry"
     if item.is_dir():continue
     if p.suffix.lower() not in (".blend",".fbx",".obj",".glb"):continue
     outfile=target/("file_"+str(len(candidates))+p.suffix.lower())
     with z.open(item) as src, outfile.open("wb") as dest:
      data=src.read(40_000_001)
      assert len(data)<=40_000_000
      dest.write(data)
     candidates.append((p.name,outfile))
  else:
   assert file.lower().endswith(".blend") or file.lower().endswith(".glb")
   candidates=[(file,target/file)]
  assert candidates,"no supported 3D source found"
  def rank(item):
   stem=item[0].lower()
   return (0 if "soldier" in stem or "paratrooper" in stem or "gaz" in stem or "tank" in stem or "jeep" in stem else 1,
    {".blend":0,".fbx":1,".glb":2,".obj":3}[item[1].suffix],len(stem))
  candidates.sort(key=rank)
  print("SOURCE_CANDIDATES",name,[(a,b.stat().st_size) for a,b in candidates][:9],flush=True)
  good=None
  for original,path in candidates[:4]:
   dest=target/(name+".glb")
   try:
    if path.suffix==".glb":dest.write_bytes(path.read_bytes())
    else:
     cmd=["blender","-b","-t","1","--python",str(ROOT/"tools/assets/blender_export_ww2.py"),
          "--",str(path),str(dest),spec["category"],name]
     result=subprocess.run(cmd,timeout=105,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
     print("BLENDER_OUTPUT",name,original,result.stdout[-2200:],flush=True)
     if result.returncode!=0:raise RuntimeError("Blender exit "+str(result.returncode))
    assert dest.exists(),"Blender produced no GLB"
    content=dest.read_bytes()
    import struct
    assert 28<len(content)<3_500_000 and content[:4]==b"glTF"
    assert struct.unpack_from("<II",content,4)==(2,len(content)),"glb malformed"
    # Verify JSON chunk extensions are supported by in-house decoders.
    l,ctype=struct.unpack_from("<II",content,12)
    assert ctype==0x4e4f534a
    scene=json.loads(content[20:20+l].decode("utf-8"))
    required=scene.get("extensionsRequired") or []
    assert not required, "unsupported glTF extension "+str(required)
    if spec["category"]=="character":
     assert scene.get("skins"),"character export lacks skeletal skin"
     assert len(scene["skins"])==1,"multi-skin unsupported"
    else:assert scene.get("meshes"),"vehicle mesh missing"
    good=(original,content,scene)
    break
   except Exception as e:
    print("EXPORT_REJECT",name,original,repr(e),flush=True)
  assert good,"no candidate exported within runtime compatibility budgets"
  original,content,scene=good
  transport=OUT/(name+".glb.b64")
  transport.write_text(base64.b64encode(content).decode("ascii")+"\n",encoding="ascii")
  record={key:spec[key] for key in ("id","url","license","creator","source_page","category")}
  record.update(source_file=file,chosen_file=original,source_bytes=len(raw),
    original_package_sha256=digest,derived_glb_bytes=len(content),
    derived_sha256=hashlib.sha256(content).hexdigest(),
    animations=len(scene.get("animations",[])),skins=len(scene.get("skins",[])),
    model_file=transport.relative_to(ROOT).as_posix())
  records.append(record)
  print("VENDORED_WW2",name,record["derived_glb_bytes"],record["animations"],flush=True)
 except Exception as e:
  print("ASSET_IMPORT_FAILED",spec["id"],repr(e),flush=True)
manifest={"schema_version":1,"summary":"Original sources retain their own license; conversions by Warrior's Paradise. CC-BY must be attributed.","assets":records}
(OUT/"WW2_MANIFEST.json").write_text(json.dumps(manifest,indent=2)+"\n")
print("VENDORED_WW2_ASSETS",len(records),flush=True)
# Fail if we don't get at least a period-correct soldier and military vehicle model.
assert any(x["category"]=="character" for x in records),"No actual WW2 soldier mesh imported"
assert any(x["category"]=="vehicle" for x in records),"No actual military vehicle mesh imported"
assert any(x["id"]=="allied_ww2" and x["animations"]>=2 for x in records),"No working Allied animation GLB converted"
assert any(x["id"]=="willys_mb" for x in records),"Historically authentic Willys MB jeep absent"
