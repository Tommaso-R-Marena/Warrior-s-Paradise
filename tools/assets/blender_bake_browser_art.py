#!/usr/bin/env python3
"""Offline Blender AO + LOD compiler for the free Cloudflare WebGL battlefield.

Run on a temporary GPU-equipped machine:
 blender --background --python tools/assets/blender_bake_browser_art.py -- \
  --input /absolute/model.glb --id farmhouse-hero \
  --author "Original author / CC0 source name" --license CC0-1.0 \
  --output browser/3d/assets/baked/farmhouse-hero.wpmesh.json

This bakes vertex ambient occlusion, NOT lightmaps or material textures.
Textures/UVs cannot currently be preserved by the game's vertex-color shader.
Always verify original licenses, historical suitability and performance.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import sys
from pathlib import Path

try:
    import bpy
    import bmesh
    from mathutils import Vector
    from mathutils.bvhtree import BVHTree
except ImportError as error:
    raise SystemExit("Launch using Blender's bundled Python (blender --background --python ...): " + str(error))

sys.path.insert(0, str(Path(__file__).resolve().parent))
from compile_baked_art import compile_mesh, verify_pack

def args():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--input",type=Path,required=True)
    p.add_argument("--output",type=Path,required=True)
    p.add_argument("--id",required=True)
    p.add_argument("--author",required=True)
    p.add_argument("--license",choices=["CC0-1.0","CC-BY-4.0","CC-BY-SA-4.0","original"],required=True)
    p.add_argument("--medium-ratio",type=float,default=.38)
    p.add_argument("--ao-rays",type=int,default=12)
    p.add_argument("--ao-distance",type=float,default=2.0)
    return p.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else [])

def color_for(obj,index):
    if 0<=index<len(obj.data.materials):
        m=obj.data.materials[index]
        if m:
            if m.use_nodes:
                node=next((n for n in m.node_tree.nodes if n.type=="BSDF_PRINCIPLED"),None)
                if node:
                    rgba=node.inputs["Base Color"].default_value
                    return [float(max(0,min(1,c))) for c in rgba[:3]]
            return [float(max(0,min(1,c))) for c in m.diffuse_color[:3]]
    return [.55,.54,.49]

def world_geometry(objects):
    points,polygons=[],[]
    for obj in objects:
        mesh=obj.data
        mesh.calc_loop_triangles()
        start=len(points)
        points.extend(obj.matrix_world@v.co for v in mesh.vertices)
        polygons.extend(tuple(start+i for i in tri.vertices) for tri in mesh.loop_triangles)
    return BVHTree.FromPolygons(points,polygons,all_triangles=True,epsilon=0.0)

def bake_ao(bvh,p,n,num_rays,maxdist):
    normal=n.normalized()
    tangent=normal.cross(Vector((0,0,1)))
    if tangent.length<.1:tangent=normal.cross(Vector((0,1,0)))
    tangent.normalize()
    bitangent=normal.cross(tangent).normalized()
    hits=0
    for i in range(num_rays):
        angle=2*math.pi*((i*.61803398875)%1)
        radius=math.sqrt((i+.5)/num_rays)
        direction=(normal*(1.05-radius*.45)+
                   tangent*math.cos(angle)*radius+
                   bitangent*math.sin(angle)*radius).normalized()
        point=obj_origin=p+normal*.024
        location,normal_hit,face,dist=bvh.ray_cast(point,direction,maxdist)
        if location is not None:
            hits+=max(.15,1.0-dist/maxdist)
    return max(.46,min(1.,1.-.52*hits/num_rays))

def expand(objects,bvh,ratio,rays,distance):
    values=[]
    for obj in objects:
        bm=bmesh.new()
        bm.from_mesh(obj.data)
        if ratio<1:
            # LOD geometry is discarded after export; physical collision remains
            # the game's existing building shells, not the reduced visual mesh.
            bmesh.ops.decimate_collapse(bm,edges=list(bm.edges),ratio=ratio)
        bmesh.ops.triangulate(bm,faces=list(bm.faces))
        bm.verts.ensure_lookup_table()
        bm.faces.ensure_lookup_table()
        for face in bm.faces:
            rgb=color_for(obj,face.material_index)
            if len(face.verts)!=3:continue
            for v in face.verts:
                p=obj.matrix_world@v.co
                normal=(obj.matrix_world.to_3x3().inverted().transposed()@face.normal).normalized()
                ao=bake_ao(bvh,p,normal,rays,distance)
                # Blender Z-up -> browser Y-up, preserving right-handedness.
                position=[p.x,p.z,-p.y]
                direction=[normal.x,normal.z,-normal.y]
                values.extend([*position,*direction,*[c*ao for c in rgb]])
        bm.free()
    return values

def main():
    opt=args()
    if not opt.input.is_file() or opt.input.suffix.lower() not in {".glb",".gltf",".obj",".fbx"}:
        raise SystemExit("Input must be an existing local .glb/.gltf/.obj/.fbx file")
    if not .05 <= opt.medium_ratio <= 1 or not 1<=opt.ao_rays<=48 or not .1<=opt.ao_distance<=12:
        raise SystemExit("Invalid LOD/AO settings")
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    path=str(opt.input.resolve())
    if opt.input.suffix.lower() in {".glb",".gltf"}:
        bpy.ops.import_scene.gltf(filepath=path)
    elif opt.input.suffix.lower()==".obj":
        bpy.ops.wm.obj_import(filepath=path)
    else:
        bpy.ops.import_scene.fbx(filepath=path)
    objects=[obj for obj in bpy.context.scene.objects if obj.type=="MESH" and
             len(obj.data.polygons)>0 and not obj.hide_get()]
    if not objects:raise SystemExit("Imported file contains no usable mesh")
    bvh=world_geometry(objects)
    high=expand(objects,bvh,1.0,opt.ao_rays,opt.ao_distance)
    medium=expand(objects,bvh,opt.medium_ratio,opt.ao_rays,opt.ao_distance)
    source={"author":opt.author,"license":opt.license,"uri":opt.input.name,
            "input_sha256":hashlib.sha256(opt.input.read_bytes()).hexdigest(),
            "bake":"deterministic-hemisphere-vertex-AO; Blender; no UV/material-map transfer"}
    packed=compile_mesh({"id":opt.id,"source":source,"high":high,"medium":medium})
    summary=verify_pack(packed)
    opt.output.parent.mkdir(parents=True,exist_ok=True)
    opt.output.write_text(json.dumps(packed,separators=(",",":"))+"\n",encoding="utf-8")
    print("PASS offline baked Cloudflare art:",json.dumps(summary,sort_keys=True))
    print("Next: add an anchored placement to browser/3d/assets/baked/scene.json and test. "
          "Keep the source license record and performance budget.")

if __name__=="__main__":
    main()
