#!/usr/bin/env python3
"""Deterministic *starter* stonework authored for existing Normandy building shells.

Not photogrammetry, not a replacement for licensed soldier/tank art. Regenerate
with: python tools/assets/generate_baked_facade.py
Assert reproducibility with: python tools/assets/generate_baked_facade.py --check
"""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
from compile_baked_art import compile_mesh, verify_pack

ROOT = Path(__file__).resolve().parents[2]
DEST = ROOT / "browser/3d/assets/baked/normandy-stonework.wpmesh.json"
SOURCE = {
    "author": "Warrior's Paradise original parametric art",
    "license": "original",
    "uri": "tools/assets/generate_baked_facade.py",
    "note": "Illustrative low-poly baked-color stonework. Not production photogrammetry or full lightmaps.",
}


def generate(high: bool) -> list[float]:
    values: list[float] = []
    def tri(a, b, c, color):
        ab = [b[j]-a[j] for j in range(3)]
        ac = [c[j]-a[j] for j in range(3)]
        n = [ab[1]*ac[2]-ab[2]*ac[1],
             ab[2]*ac[0]-ab[0]*ac[2],
             ab[0]*ac[1]-ab[1]*ac[0]]
        length = math.sqrt(sum(x*x for x in n)) or 1
        normal = [x/length for x in n]
        for p in (a,b,c):
            values.extend([*p,*normal,*color])
    def quad(a,b,c,d,col):
        tri(a,b,c,col)
        tri(a,c,d,col)
    index = 0
    def stone(x,y,w,h,d):
        nonlocal index
        light=.86+.10*math.sin(index*7.19)
        col=[.64*light,.59*light,.48*light]
        side=[v*.78 for v in col]
        deep=[col[0]*.62,col[1]*.61,col[2]*.64]
        x0,x1=x-w/2,x+w/2
        y0,y1=y-h/2,y+h/2
        z0,z1=.052,d
        index+=1
        if not high:
            quad([x0,y0,z1],[x1,y0,z1],[x1,y1,z1],[x0,y1,z1],col)
            return
        eps=min(.10,w*.17,h*.18)
        p=[[x0,y0,z0],[x1,y0,z0],[x1,y1,z0],[x0,y1,z0]]
        q=[[x0+eps,y0+eps,z1],[x1-eps,y0+eps,z1],[x1-eps,y1-eps,z1],[x0+eps,y1-eps,z1]]
        quad(q[0],q[1],q[2],q[3],col)
        for k in range(4):
            quad(p[k],p[(k+1)%4],q[(k+1)%4],q[k],deep if k in (0,3) else side)
    for x in (-4.03,4.03):
        for i in range(12):
            stone(x+(-.065 if i%2 else .07),.27+i*.313,.72,.276,.24+(i%3)*.016)
    for i in range(19):
        stone(-4.35+i*.486,.23,.468,.27,.23)
    for x in (-2.35,2.35):
        stone(x,3.62,1.30,.21,.20)
        stone(x,1.75,1.25,.15,.21)
        if high:
            for i in range(5):
                stone(x-.54+i*.27,3.78,.245,.14,.13)
                stone(x-.54+i*.27,1.62,.245,.12,.18)
    for i in range(7):
        stone(-1.04+i*.34,2.49,.32,.23,.22)
    if high:
        for x in (-3.45,-1.23,1.2,3.42):
            for i in range(8):
                stone(x+math.sin(i*2.2)*.055,.44+i*.44,.20,.25,.12)
    return values


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--check",action="store_true")
    args=p.parse_args()
    compiled=compile_mesh({"id":"normandy-stonework","source":SOURCE,
                           "medium":generate(False),"high":generate(True)})
    report=verify_pack(compiled)
    if args.check:
        current=json.loads(DEST.read_text(encoding="utf-8"))
        if current != compiled:
            raise SystemExit("ERROR: published baked art does not match deterministic source")
        print("PASS baked source reproducibility",json.dumps(report,sort_keys=True))
    else:
        DEST.parent.mkdir(parents=True,exist_ok=True)
        DEST.write_text(json.dumps(compiled,separators=(",",":"))+"\n",encoding="utf-8")
        print("Generated baked sample",json.dumps(report,sort_keys=True))


if __name__=="__main__":
    main()
