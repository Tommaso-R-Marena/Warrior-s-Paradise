#!/usr/bin/env python3
"""Bake a properly UV-textured static WWII model into the free Cloudflare renderer.

Run with Blender 4.2+ installed:
 blender --background --python tools/assets/blender_export_pbr_lightmaps.py -- \
   --input /path/approved-model.glb --id farmhouse-hero \
   --license CC0-1.0 --author "Original creator" \
   --output browser/3d/assets/pbr

Features: preserve UV0, generate nonoverlapping UV1; Cycles diffuse albedo,
tangent normals, roughness, metallic-as-emission and scene irradiance;
export optimized High/Medium triangle LODs with both UV channels.
This is a *static opaque object* exporter, not skinned animation export.
All source licensing and world placement must be reviewed manually.
"""
from __future__ import annotations

import argparse
import array
import base64
import hashlib
import json
import math
from pathlib import Path
import sys

try:
    import bpy
except ImportError as exc:
    raise SystemExit("Use Blender's bundled Python: blender --background --python ...") from exc

sys.path.insert(0,str(Path(__file__).resolve().parent))
from compile_pbr_lightmaps import compile_mesh,verify,fnv32


def cli():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--input",type=Path,required=True)
    p.add_argument("--id",required=True)
    p.add_argument("--output",type=Path,required=True)
    p.add_argument("--author",required=True)
    p.add_argument("--license",choices=["CC0-1.0","CC-BY-3.0","CC-BY-4.0",
                                          "CC-BY-SA-3.0","CC-BY-SA-4.0","original"],required=True)
    p.add_argument("--medium-ratio",type=float,default=.42)
    p.add_argument("--texture-size",type=int,choices=[128,256,512,1024],default=512)
    p.add_argument("--lightmap-size",type=int,choices=[64,128,256,512,1024],default=256)
    p.add_argument("--cycles-samples",type=int,default=32)
    return p.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else [])


def select(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True);bpy.context.view_layer.objects.active=obj


def prepare_mesh(filename):
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    path=str(filename.resolve())
    suffix=filename.suffix.lower()
    if suffix in (".glb",".gltf"):bpy.ops.import_scene.gltf(filepath=path)
    elif suffix==".obj":bpy.ops.wm.obj_import(filepath=path)
    elif suffix==".fbx":bpy.ops.import_scene.fbx(filepath=path)
    else:raise ValueError("only GLB/glTF, OBJ and FBX")
    meshes=[o for o in bpy.context.scene.objects if o.type=="MESH" and len(o.data.polygons)]
    if not meshes:raise ValueError("source has no mesh")
    for obj in meshes:
        if obj.get("wp_no_export"):continue
        select(obj)
        bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    bpy.ops.object.select_all(action="DESELECT")
    for obj in meshes:obj.select_set(True)
    bpy.context.view_layer.objects.active=meshes[0]
    bpy.ops.object.join()
    obj=meshes[0];select(obj)
    if len(obj.data.materials)!=1 or not obj.data.materials[0]:
        raise ValueError("Export one authored material per pack; split multi-material sources into separate models first")
    mat=obj.data.materials[0]
    if mat.blend_method!="OPAQUE":
        raise ValueError("Alpha blending/transparency is unsupported by this opaque game renderer")
    mat.use_nodes=True
    if not obj.data.uv_layers:
        raise ValueError("Model must have genuine UV0; unwrap UVs before export")
    uv0=obj.data.uv_layers[0];uv0.name="WP_PBR_UV"
    if len(obj.data.uv_layers)>1:
        for uv in list(obj.data.uv_layers)[1:]:obj.data.uv_layers.remove(uv)
    # Keep the first authored material UV set intact and construct independent
    # nonoverlapping atlas charts for the static lightmap.
    lightuv=obj.data.uv_layers.new(name="WP_STATIC_LIGHT_UV")
    obj.data.uv_layers.active=lightuv
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(island_margin=.025)
    bpy.ops.object.mode_set(mode="OBJECT")
    select(obj)
    return obj,mat,uv0,lightuv


def new_image(label,size):
    im=bpy.data.images.new(label,width=size,height=size,alpha=True,float_buffer=False)
    im.generated_color=(.5,.5,.5,1)
    return im


def attach_target(mat,image):
    for node in mat.node_tree.nodes:
        node.select=False
    target=mat.node_tree.nodes.new("ShaderNodeTexImage")
    target.image=image
    mat.node_tree.nodes.active=target
    return target


def bake(obj,mat,uv,image,bake_type,*,pass_filter=None):
    obj.data.uv_layers.active=uv
    target=attach_target(mat,image)
    select(obj)
    params={"type":bake_type}
    if pass_filter is not None:params["pass_filter"]=pass_filter
    bpy.ops.object.bake(**params)
    mat.node_tree.nodes.remove(target)
    return image


def metallic_bake(obj,mat,uv,size):
    """Bake the actual metallic BSDF input, including linked packed ORM maps."""
    nodes=mat.node_tree.nodes
    outputs=[n for n in nodes if n.type=="OUTPUT_MATERIAL" and n.is_active_output]
    bsdfs=[n for n in nodes if n.type=="BSDF_PRINCIPLED"]
    if len(outputs)!=1 or len(bsdfs)!=1:
        raise ValueError("Metallic baking requires one active Material Output and Principled BSDF")
    output,bsdf=outputs[0],bsdfs[0]
    socket=bsdf.inputs.get("Metallic")
    if socket is None:raise ValueError("No Principled Metallic socket")
    original=output.inputs["Surface"].links[0] if output.inputs["Surface"].is_linked else None
    previous=(original.from_socket if original else None)
    emit=nodes.new("ShaderNodeEmission")
    links=mat.node_tree.links
    try:
        if socket.is_linked:
            links.new(socket.links[0].from_socket,emit.inputs["Color"])
        else:
            emit.inputs["Color"].default_value=(socket.default_value,)*3+(1.,)
        links.new(emit.outputs["Emission"],output.inputs["Surface"])
        im=new_image("WP_Baked_Metallic",size)
        bake(obj,mat,uv,im,"EMIT")
    finally:
        if previous is not None:links.new(previous,output.inputs["Surface"])
        else:
            for link in list(output.inputs["Surface"].links):links.remove(link)
        nodes.remove(emit)
    return im


def save_b64_png(image,target):
    # PNG lossless; avoids normal-map JPEG artifacts and preserves authored PBR.
    image.filepath_raw=str(target.with_suffix(".png").resolve())
    image.file_format="PNG"
    image.save()
    raw=target.with_suffix(".png").read_bytes()
    if len(raw)>5_000_000:raise ValueError("texture too large; lower texture-size")
    target.write_text(base64.b64encode(raw).decode("ascii")+"\n",encoding="ascii")
    target.with_suffix(".png").unlink()
    return target.name


def save_linear_lightmap(image,path):
    # Irradiance is LINEAR data and must bypass browser SRGB decoding.
    data=array.array("f",[0.0])*(image.size[0]*image.size[1]*4)
    image.pixels.foreach_get(data)
    out=bytearray(len(data))
    for i,value in enumerate(data):
        if not math.isfinite(value):raise ValueError("nonfinite Cycles lightmap pixel")
        # Clamp physically baked HDR irradiance to LDR; use larger HDR workflow
        # separately when dynamic range is too great for a static 8-bit atlas.
        out[i]=round(max(0.,min(1.,value))*255)
    result={"schema":"wp-linear-texture-v1","width":image.size[0],"height":image.size[1],
       "encoding":"linear-irradiance-rgba8","rgba":base64.b64encode(out).decode("ascii"),"fnv32":fnv32(out)}
    path.write_text(json.dumps(result,separators=(",",":"))+"\n",encoding="utf-8")


def gather(obj):
    result=[]
    mesh=obj.data
    mesh.calc_loop_triangles()
    uv0=mesh.uv_layers["WP_PBR_UV"].data
    uv1=mesh.uv_layers["WP_STATIC_LIGHT_UV"].data
    inverse=obj.matrix_world.to_3x3().inverted().transposed()
    for t in mesh.loop_triangles:
        for loop_index in t.loops:
            loop=mesh.loops[loop_index]
            p=obj.matrix_world@mesh.vertices[loop.vertex_index].co
            n=(inverse@loop.normal).normalized()
            texture=uv0[loop_index].uv
            light=uv1[loop_index].uv
            result.extend((p.x,p.z,-p.y,n.x,n.z,-n.y,
             float(texture.x),float(texture.y),float(light.x),float(light.y)))
    return result


def add_lod(obj,ratio):
    # Non-destructively duplicate for Medium while retaining both UV channels.
    duplicate=obj.copy()
    duplicate.data=obj.data.copy()
    bpy.context.collection.objects.link(duplicate)
    if ratio<.999:
        modifier=duplicate.modifiers.new("WP_Medium_Optimized","DECIMATE")
        modifier.ratio=ratio
        select(duplicate)
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    return duplicate


def main():
    args=cli()
    if not args.input.is_file():raise ValueError("source file missing")
    if not .10<=args.medium_ratio<=1 or not 1<=args.cycles_samples<=512:
        raise ValueError("invalid ratio or bake samples")
    args.output.mkdir(parents=True,exist_ok=True)
    obj,mat,uv,lightuv=prepare_mesh(args.input)
    scene=bpy.context.scene
    scene.render.engine="CYCLES"
    scene.cycles.samples=args.cycles_samples
    scene.render.bake.margin=8
    scene.render.bake.use_clear=True
    # Set consistent neutral lighting for static lightmap generation.
    world=bpy.data.worlds.new("WP_Static_Outdoor_World")
    world.use_nodes=True
    world.node_tree.nodes.get("Background").inputs["Color"].default_value=(.49,.59,.71,1)
    world.node_tree.nodes.get("Background").inputs["Strength"].default_value=.55
    scene.world=world
    bpy.ops.object.light_add(type="SUN",location=(7,-8,16))
    sun=bpy.context.object
    sun.data.energy=1.7
    sun.rotation_euler=(.35,-.48,-.3)
    select(obj)
    # Render authored color, tangent normal, roughness, metallic and irradiance
    # into explicit image maps, with no requirement for runtime materials/FBX.
    albedo=bake(obj,mat,uv,new_image("WP_Baked_Albedo",args.texture_size),
                "DIFFUSE",pass_filter={"COLOR"})
    normal=bake(obj,mat,uv,new_image("WP_Baked_Tangent_Normal",args.texture_size),"NORMAL")
    roughness=bake(obj,mat,uv,new_image("WP_Baked_Roughness",args.texture_size),"ROUGHNESS")
    metallic=metallic_bake(obj,mat,uv,args.texture_size)
    irradiance=bake(obj,mat,lightuv,new_image("WP_Static_Irradiance",args.lightmap_size),
                    "DIFFUSE",pass_filter={"DIRECT","INDIRECT"})
    prefix=args.id
    names={}
    for channel,img in (("albedo",albedo),("normal",normal),("roughness",roughness),("metallic",metallic)):
        names[channel]="./assets/pbr/"+save_b64_png(img,args.output/(prefix+"-"+channel+".png.b64"))
    lightname=prefix+"-lightmap.wptex.json"
    save_linear_lightmap(irradiance,args.output/lightname)
    names["lightmap"]="./assets/pbr/"+lightname
    high=gather(obj)
    medium=gather(add_lod(obj,args.medium_ratio))
    source={"license":args.license,"author":args.author,"source_filename":args.input.name,
            "sha256":hashlib.sha256(args.input.read_bytes()).hexdigest(),
            "baking":"Blender Cycles normal/roughness/metallic + UV1 direct-indirect diffuse irradiance"}
    packed=compile_mesh({"id":args.id,"source":source,"high":high,"medium":medium})
    verify(packed)
    (args.output/(prefix+".wppbr.json")).write_text(
        json.dumps(packed,separators=(",",":"),sort_keys=True)+"\n",encoding="utf-8")
    print("EXPORT_SUCCESS",json.dumps({"id":prefix,"triangles":verify(packed),
     "maps":names,"license":source["license"]},sort_keys=True))
    print("Add a reviewed placement to scene.json, then test in Chromium. No GPU server is required after upload.")


if __name__=="__main__":
    main()
