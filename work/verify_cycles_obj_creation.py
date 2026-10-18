"""Actual Blender creation/save/reopen check for an owned native OBJ fixture."""
import json
import math
import sys
from pathlib import Path
import bpy
root = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(root/'work'))
from cycles_obj_snapshot_candidate import decode_snapshot, insert_snapshot_in_scenes, snapshot_corner_normals
snapshot = decode_snapshot((root/'outputs/obj-gpu-import/snapshot-editor/model.sfmobj').read_bytes())
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
primary_scene = bpy.context.scene
second_scene = bpy.data.scenes.new('Second shot')
obj = insert_snapshot_in_scenes(snapshot, [primary_scene, second_scene])
assert primary_scene.objects[obj.name] == second_scene.objects[obj.name]
def state(obj):
    return dict(vertices=[list(v.co) for v in obj.data.vertices],
        uvs=[list(v.uv) for v in obj.data.uv_layers.active.data],
        slots=[p.material_index for p in obj.data.polygons],
        images={image.name:dict(size=list(image.size), pixels=[round(v*255) for v in image.pixels[:]],
                               packed=bool(image.packed_file)) for image in bpy.data.images
                if image.name.startswith('Imported OBJ ')}, scale=obj['sfm64_obj_scale'])
before = state(obj)
expected_normals = snapshot_corner_normals(snapshot)
def normal_error(obj, expected):
    normals = obj.data.corner_normals
    return max(abs(float(normals[loop.index].vector[k])-expected[loop.vertex_index][k])
               for loop in obj.data.loops for k in range(3))
before_normal_error = normal_error(obj, expected_normals)
assert before_normal_error < .0005
assert before['scale']==.5 and len(before['vertices'])==6 and len(before['slots'])==2
assert len(before['images'])==2 and all(v['packed'] for v in before['images'].values())
for material, image in snapshot['images'].items():
    assert before['images']['Imported OBJ '+material]['pixels']==list(image['rgba'])
folder=root/'outputs/cycles-conversion/obj-creation'
folder.mkdir(parents=True,exist_ok=True)
blend=folder/'import.blend'
bpy.ops.wm.save_as_mainfile(filepath=str(blend),check_existing=False)
bpy.ops.wm.open_mainfile(filepath=str(blend),load_ui=False,use_scripts=False)
assert state(bpy.data.objects['Imported OBJ'])==before
after_normal_error = normal_error(bpy.data.objects['Imported OBJ'], expected_normals)
assert after_normal_error < .0005
assert bpy.data.scenes['Second shot'].objects['Imported OBJ'] == bpy.data.objects['Imported OBJ']
(folder/'report.json').write_text(json.dumps(dict(created=True,reopen_state_exact=True,
    vertices=6,polygons=2,packed_images=2,scale=.5, shared_across_shots=True,
    normal_max_component_error_before=before_normal_error,
    normal_max_component_error_after=after_normal_error,
    limitation='Mesh/image persistence only; render, smoothing/custom normals and triangulation parity pending.'),indent=2))
print('cycles_obj_creation_reopen_verified',flush=True)
