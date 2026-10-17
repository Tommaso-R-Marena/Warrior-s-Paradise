#!/usr/bin/env python3
"""Compile standalone UV/PBR meshes to an 18-byte/vertex Cloudflare-safe pack.

Input: JSON {id,source,bounds?,medium:[position3,normal3,uv2,lightmapUV2],
high:[...]} in expanded non-indexed triangle order. Coordinates are Y up,
UVs in [0,1], UV2 non-overlapping from Blender. Output is static JSON.
This compiler is CPU-only and uses Python's standard library.
"""
from __future__ import annotations

import argparse
import base64
import json
import math
import struct
from pathlib import Path

LICENSES={"CC0-1.0","CC-BY-3.0","CC-BY-4.0","CC-BY-SA-3.0","CC-BY-SA-4.0","original"}


def check(ok,msg):
    if not ok:raise ValueError("PBR pack: "+msg)


def fnv32(data):
    h=0x811c9dc5
    for v in data:h=((h^v)*0x01000193)&0xffffffff
    return f"{h:08x}"


def round_js(x):
    return math.floor(x+.5)


def compile_mesh(src):
    import re
    check(isinstance(src.get("id"),str) and re.fullmatch(r"[a-z0-9][a-z0-9_-]{0,63}",src["id"]),"id")
    source=src.get("source",{})
    check(source.get("license") in LICENSES and isinstance(source.get("author"),str) and
          0<len(source["author"])<160,"license/author")
    all_vertices=[]
    for tier in ("medium","high"):
        values=src.get(tier)
        check(isinstance(values,list) and 30<=len(values)<=1800000 and len(values)%30==0,
              tier+" triangle budget")
        for i in range(0,len(values),10):
            item=values[i:i+10]
            check(len(item)==10 and all(isinstance(x,(float,int)) and math.isfinite(x) for x in item),
                  "nonfinite vertex")
            check(all(abs(x)<250 for x in item[:3]),"position outside bounds")
            check(.85<math.hypot(*item[3:6])<1.15,"normal")
            check(all(0<=uv<=1 for uv in item[6:]),"UVs must fit atlas [0,1]")
            all_vertices.append(item)
    low=[min(v[k] for v in all_vertices) for k in range(3)]
    high=[max(v[k] for v in all_vertices) for k in range(3)]
    lods={}
    for tier in ("medium","high"):
        data=bytearray()
        values=src[tier]
        for i in range(0,len(values),10):
            vertex=values[i:i+10]
            quant=[round_js((vertex[k]-low[k])/(high[k]-low[k])*65535) if high[k]!=low[k] else 0
                   for k in range(3)]
            signed=[max(-127,min(127,round_js(vertex[k]*127))) for k in range(3,6)]
            uv=[max(0,min(65535,round_js(vertex[k]*65535))) for k in range(6,10)]
            data.extend(struct.pack("<3H3bB4H",*quant,*signed,0,*uv))
        lods[tier]={"vertices":len(values)//10,"data":base64.b64encode(data).decode("ascii"),
                    "fnv32":fnv32(data)}
    return {"schema":"wp-pbr-mesh-v2","id":src["id"],"source":source,
            "bounds":{"min":low,"max":high},"lods":lods}


def verify(pack):
    check(pack["schema"]=="wp-pbr-mesh-v2","schema")
    bounds=pack["bounds"]
    for tier in ("medium","high"):
        lod=pack["lods"][tier]
        data=base64.b64decode(lod["data"],validate=True)
        check(len(data)==lod["vertices"]*18 and lod["vertices"]%3==0,"stride/vertices")
        check(fnv32(data)==lod["fnv32"],"checksum")
        for i in range(0,len(data),18):
            values=struct.unpack_from("<3H3bB4H",data,i)
            check(values[6]==0 and math.hypot(*values[3:6])>70,"vertex normal/reserved")
    return {tier:pack["lods"][tier]["vertices"]//3 for tier in ("medium","high")}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--input",type=Path)
    p.add_argument("--output",type=Path)
    p.add_argument("--verify",type=Path)
    args=p.parse_args()
    if args.verify:
        print("PASS PBR PACK",verify(json.loads(args.verify.read_text(encoding="utf-8"))))
        return
    if not args.input or not args.output:p.error("specify input and output")
    pack=compile_mesh(json.loads(args.input.read_text(encoding="utf-8")))
    verify(pack)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(pack,separators=(",",":"),sort_keys=True)+"\n",encoding="utf-8")
    print("PBR UV/LightUV compiled",verify(pack))


if __name__=="__main__":
    main()
