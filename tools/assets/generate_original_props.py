#!/usr/bin/env python3
"""Generate project-original lightweight WWII environment props as real OBJ meshes.

No Blender, internet, external libraries, or downloaded game artwork needed.
These are production-pipeline STARTER meshes, not commercial-quality assets.

Outputs eight reproducible meshes plus basic .mtl materials under
ExternalAssets/wp_original/ and adds checked hashes to asset-lock.json.
Existing unrelated lock entries are preserved. An edited existing generated
file is NEVER overwritten silently.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import random

ROOT = Path(__file__).resolve().parents[2]
SLOTS = ("tree", "bush", "rubble", "crate", "fence", "barrel", "stump", "sandbag")
PALETTE = {
    "wood": (0.27, 0.19, 0.12),
    "wood_light": (0.48, 0.33, 0.18),
    "leaf": (0.19, 0.28, 0.16),
    "leaf_light": (0.29, 0.39, 0.22),
    "stone": (0.48, 0.46, 0.38),
    "stone_light": (0.59, 0.54, 0.45),
    "soil": (0.42, 0.35, 0.25),
    "canvas": (0.61, 0.55, 0.38),
    "metal": (0.26, 0.29, 0.25),
}


class Mesh:
    def __init__(self):
        self.vertices: list[tuple[float, float, float]] = []
        self.faces: list[tuple[int, int, int, str]] = []

    def point(self, x: float, y: float, z: float) -> int:
        self.vertices.append((x, y, z))
        return len(self.vertices)

    def quad(self, a: int, b: int, c: int, d: int, material: str) -> None:
        self.faces.append((a, b, c, material))
        self.faces.append((a, c, d, material))

    def box(self, x: float, y: float, z: float,
            sx: float, sy: float, sz: float, material: str) -> None:
        a, b, c, d = (
            self.point(x+dx*sx/2, y+dy*sy/2, z-sz/2)
            for dx, dy in ((-1, -1), (1, -1), (1, 1), (-1, 1))
        )
        e, f, g, h = (
            self.point(x+dx*sx/2, y+dy*sy/2, z+sz/2)
            for dx, dy in ((-1, -1), (1, -1), (1, 1), (-1, 1))
        )
        for q in ((a, d, c, b), (e, f, g, h), (a, b, f, e),
                  (b, c, g, f), (c, d, h, g), (d, a, e, h)):
            self.quad(*q, material)

    def cylinder(self, x: float, y: float, bottom: float, top: float,
                 rb: float, rt: float, material: str, segments: int = 10) -> None:
        low, high = [], []
        for i in range(segments):
            t=math.tau*i/segments
            co, si=math.cos(t),math.sin(t)
            low.append(self.point(x+rb*co, y+rb*si, bottom))
            high.append(self.point(x+rt*co, y+rt*si, top))
        for i in range(segments):
            j=(i+1)%segments
            self.quad(low[i], low[j], high[j], high[i], material)
        self.faces.extend(
            (low[0], low[i+1], low[i], material) for i in range(1, segments-1)
        )
        self.faces.extend(
            (high[0], high[i], high[i+1], material) for i in range(1, segments-1)
        )

    def ellipsoid(self, x: float, y: float, z: float, rx: float, ry: float,
                  rz: float, material: str, segments: int = 10,
                  rings: int = 5) -> None:
        grid: list[list[int]] = []
        for j in range(rings+1):
            latitude=-math.pi/2+j*math.pi/rings
            row=[]
            for i in range(segments):
                angle=math.tau*i/segments
                row.append(self.point(
                    x+rx*math.cos(latitude)*math.cos(angle),
                    y+ry*math.cos(latitude)*math.sin(angle),
                    z+rz*math.sin(latitude)))
            grid.append(row)
        for j in range(rings):
            for i in range(segments):
                k=(i+1)%segments
                self.quad(grid[j][i],grid[j][k],grid[j+1][k],grid[j+1][i],material)

    def obj(self, slot: str) -> str:
        if not self.faces:
            raise ValueError(f"Mesh {slot} has no faces")
        lines=[f"# Warrior's Paradise original procedural {slot} mesh",
               f"mtllib wp_{slot}.mtl",f"o WP_{slot.title()}"]
        lines.extend(f"v {x:.5f} {y:.5f} {z:.5f}" for x,y,z in self.vertices)
        last=None
        for a,b,c,material in self.faces:
            if material!=last:
                lines.append(f"usemtl {material}")
                last=material
            lines.append(f"f {a} {b} {c}")
        return "\n".join(lines)+"\n"


def build(slot: str) -> Mesh:
    mesh=Mesh()
    rand=random.Random(19440606+SLOTS.index(slot)*931)
    if slot=="tree":
        mesh.cylinder(0,0,0,420,29,15,"wood")
        for x,y,z,r in ((0,0,395,158),(90,12,320,106),
                        (-90,-30,355,112),(15,55,480,95)):
            mesh.ellipsoid(x,y,z,r,r*.85,r*.76,
                           "leaf" if x<0 else "leaf_light")
    elif slot=="bush":
        for i in range(11):
            x=rand.uniform(-100,100);y=rand.uniform(-65,65)
            mesh.ellipsoid(x,y,55+rand.uniform(3,45),
                           rand.uniform(53,80),rand.uniform(40,68),
                           rand.uniform(36,62),
                           "leaf" if i%3 else "leaf_light",8,4)
    elif slot=="rubble":
        for i in range(19):
            x=rand.uniform(-170,170);y=rand.uniform(-115,115)
            w=rand.uniform(20,68);d=rand.uniform(22,69);h=rand.uniform(13,47)
            mesh.box(x,y,h/2,w,d,h,"stone" if i%3 else "stone_light")
            if i%4==0:
                mesh.ellipsoid(x+12,y,h,w*.67,d*.44,h*.7,"soil",8,3)
    elif slot=="crate":
        mesh.box(0,0,49,94,94,98,"wood")
        for h in (20,70):
            mesh.box(0,-49,h,100,9,12,"wood_light")
            mesh.box(0,49,h,100,9,12,"wood_light")
        mesh.box(0,0,88,104,11,9,"metal")
        mesh.box(0,0,50,10,104,8,"metal")
    elif slot=="fence":
        for x in (-185,-60,60,185):
            mesh.box(x,0,88,16,20,177,"wood")
            mesh.box(x,0,178,23,24,12,"wood_light")
        for h in (58,127):
            mesh.box(0,3,h,390,13,14,"wood_light")
        for x in (-125,0,125):
            mesh.box(x,3,88,10,13,160,"wood")
    elif slot=="barrel":
        mesh.cylinder(0,0,0,104,48,46,"wood",12)
        for height in (12,51,91):
            mesh.cylinder(0,0,height,height+7,50,50,"metal",12)
        mesh.cylinder(0,0,104,109,44,43,"wood_light",12)
    elif slot=="stump":
        mesh.cylinder(0,0,0,96,42,40,"wood",12)
        mesh.cylinder(0,0,96,101,39,35,"wood_light",12)
        for i in range(6):
            a=math.tau*i/6
            mesh.box(52*math.cos(a),52*math.sin(a),15,42,19,30,"wood")
    elif slot=="sandbag":
        for layer in range(4):
            for col in range(6-layer%2):
                x=(col-(5-layer%2)/2)*62
                y=0 if layer%2==0 else 8
                z=22+layer*35
                mesh.ellipsoid(x,y,z,38,26,21,
                               "canvas" if (col+layer)%3 else "soil",9,4)
    return mesh


def write_if_equal_or_new(path: Path, content: bytes) -> None:
    if path.exists() and path.read_bytes()!=content:
        raise RuntimeError(f"Refusing to overwrite edited original mesh: {path}")
    path.parent.mkdir(parents=True,exist_ok=True)
    if not path.exists():
        path.write_bytes(content)


def generate(root: Path) -> dict:
    root=root.resolve()
    dest=root/"ExternalAssets"/"wp_original"
    lock_path=root/"ExternalAssets"/"asset-lock.json"
    if lock_path.exists():
        lock=json.loads(lock_path.read_text(encoding="utf-8"))
        if not isinstance(lock,dict) or lock.get("schema_version")!=1 or not isinstance(lock.get("assets"),dict):
            raise ValueError("Cannot merge with incompatible asset provenance lock")
    else:
        lock={"schema_version":1,"generated_by":"WarriorsParadise original prop generator","assets":{}}
    generated={}
    for slot in SLOTS:
        directory=dest/slot
        mesh=build(slot)
        mtl=["# Original project material palette; tweak in Unreal Editor."]
        for name, rgb in PALETTE.items():
            mtl.extend([f"newmtl {name}","Kd "+" ".join(f"{v:.3f}" for v in rgb),
                        "Ka 0.04 0.04 0.04","Ks 0.02 0.02 0.02","Ns 8"])
        records=[]
        for filename,data in (
            (f"wp_{slot}.obj",mesh.obj(slot).encode("utf-8")),
            (f"wp_{slot}.mtl",("\n".join(mtl)+"\n").encode("utf-8")),
        ):
            path=directory/filename
            write_if_equal_or_new(path,data)
            records.append({
                "path":path.relative_to(root).as_posix(),
                "bytes":len(data),
                "sha256":hashlib.sha256(data).hexdigest(),
            })
        key=f"original:{slot}"
        entry={
            "id":f"wp_original_{slot}",
            "name":f"Warrior's Paradise original {slot} mesh",
            "source":"Warrior's Paradise in-repo procedural generator",
            "source_url":"https://github.com/Tommaso-R-Marena/Warrior-s-Paradise",
            "license":"Project-owned-original",
            "purpose":"Editable, original mobile environment starter art; not commercial fidelity",
            "files":records,
        }
        prior=lock["assets"].get(key)
        if prior is not None and prior!=entry:
            raise RuntimeError(f"Refusing to overwrite previously changed provenance: {key}")
        lock["assets"][key]=entry
        generated[slot]={"triangles":len(mesh.faces),"vertices":len(mesh.vertices)}
    new_lock=(json.dumps(lock,indent=2,sort_keys=True)+"\n").encode("utf-8")
    if lock_path.is_symlink():
        raise RuntimeError("Refusing symlinked asset provenance lock")
    lock_path.write_bytes(new_lock)
    return generated


def main() -> int:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root",type=Path,default=ROOT)
    args=parser.parse_args()
    result=generate(args.root)
    print(json.dumps({"status":"PASS","models":result},indent=2))
    return 0


if __name__=="__main__":
    raise SystemExit(main())
