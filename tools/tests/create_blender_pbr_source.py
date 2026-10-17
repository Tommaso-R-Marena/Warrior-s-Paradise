"""Executed inside Blender to create a real textured GLB and smoke its Cycles PBR baking."""
import bpy
from pathlib import Path
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
bpy.ops.mesh.primitive_cube_add(size=2)
o=bpy.context.object
o.name="WW2_Cycles_Export_Test_Wall"
o.scale=(2,.12,1.4)
bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
m=bpy.data.materials.new("PBR Brick Demonstration")
m.use_nodes=True
bsdf=next(n for n in m.node_tree.nodes if n.type=="BSDF_PRINCIPLED")
bsdf.inputs["Base Color"].default_value=(.35,.15,.095,1)
bsdf.inputs["Roughness"].default_value=.83
bsdf.inputs["Metallic"].default_value=.12
o.data.materials.append(m)
assert o.data.uv_layers,"test cube must preserve UV0"
dest=Path("artifacts/blender_pbr_source.glb").resolve()
dest.parent.mkdir(parents=True,exist_ok=True)
bpy.ops.export_scene.gltf(filepath=str(dest),export_format="GLB",use_selection=False)
assert dest.is_file() and dest.stat().st_size>100
print("BLENDER_TEST_SOURCE_CREATED",dest,dest.stat().st_size)
