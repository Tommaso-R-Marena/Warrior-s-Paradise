"""Blender background conversion of reviewed archival WWII 3D model files."""
import bpy,sys,os
args=sys.argv[sys.argv.index("--")+1:]
source,out,category=args[:3]
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
extension=os.path.splitext(source)[1].lower()
if extension==".blend":
 bpy.ops.wm.open_mainfile(filepath=source)
elif extension==".fbx":
 bpy.ops.import_scene.fbx(filepath=source)
elif extension==".obj":
 if hasattr(bpy.ops.wm,"obj_import"):bpy.ops.wm.obj_import(filepath=source)
 else:bpy.ops.import_scene.obj(filepath=source)
else:raise RuntimeError("Source type unsupported "+extension)
for ob in list(bpy.data.objects):
 if ob.type not in ("MESH","ARMATURE"):
  bpy.data.objects.remove(ob,do_unlink=True)
mesh=[ob for ob in bpy.data.objects if ob.type=="MESH"]
assert mesh,"Empty model scene"
# Exclude oversized/obvious environment floor panels rather than exporting game scenes.
for ob in list(mesh):
 if ob.name.lower() in ("plane","ground","floor","grid"):
  bpy.data.objects.remove(ob,do_unlink=True)
mesh=[ob for ob in bpy.data.objects if ob.type=="MESH"]
assert mesh
if category=="character":
 armatures=[o for o in bpy.data.objects if o.type=="ARMATURE"]
 assert armatures,"Cannot claim genuine rigging without an armature"
 # Keep only the main armature: exporter should encode one skin.
 primary=max(armatures,key=lambda o:len(o.data.bones))
 for a in armatures:
  if a!=primary:bpy.data.objects.remove(a,do_unlink=True)
 for ob in mesh:
  if not any(m.type=="ARMATURE" for m in ob.modifiers) and len(ob.vertex_groups):
   mod=ob.modifiers.new(name="Armature",type="ARMATURE")
   mod.object=primary
  for mod in ob.modifiers:
   if mod.type=="ARMATURE":mod.object=primary
  if len(ob.vertex_groups):
   # glTF requires the armature to own the skinned mesh hierarchy.
   world=ob.matrix_world.copy()
   ob.parent=primary
   ob.matrix_world=world
if category=="vehicle":
 # Offline topology simplification — keeps authored silhouette while avoiding
 # thousands of CPU-transformed triangles per dynamic vehicle each frame.
 total=sum(max(0,len(poly.vertices)-2) for ob in mesh for poly in ob.data.polygons)
 target=1700
 if total>target:
  ratio=max(.05,min(1.0,target/total))
  for ob in mesh:
   if not ob.data.polygons:continue
   bpy.ops.object.select_all(action="DESELECT")
   ob.select_set(True);bpy.context.view_layer.objects.active=ob
   dec=ob.modifiers.new("BrowserVehicleLOD",type="DECIMATE")
   dec.ratio=ratio
   try:bpy.ops.object.modifier_apply(modifier=dec.name)
   except Exception as exc:print("LOD_REDUCTION_SKIP",ob.name,str(exc))
  print("VEHICLE_TRIANGLE_LOD",total,target)
for ob in bpy.data.objects: ob.select_set(ob.type in ("MESH","ARMATURE"))
if bpy.context.view_layer.objects.active not in bpy.context.selected_objects:
 bpy.context.view_layer.objects.active=next(o for o in bpy.data.objects if o.type=="MESH")
bpy.ops.export_scene.gltf(filepath=out,export_format="GLB",use_selection=True,
 export_skins=True,export_animations=(category=="character"),export_morph=False,
 export_image_format="AUTO")
print("BLENDER_WW2_EXPORT_OK",out,os.stat(out).st_size)
