"""Inspect the actual converted Blender mesh against its captured DMX snapshot."""
import json
import sys
from pathlib import Path
import bpy
import numpy as np
import argparse
root=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(root/'work'))
parser=argparse.ArgumentParser()
parser.add_argument('--prepared',default=str(root/'outputs/cycles-conversion/embedded-prepared-job.json'))
parser.add_argument('--conversion-exit-code',required=True,type=int,choices=[0])
args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
prepared=json.loads(Path(args.prepared).read_text())
sys.path.insert(0,prepared['job']['converter_snapshot'])
from session import Session
from cycles_obj_snapshot_candidate import decode_session_snapshot, snapshot_corner_normals
from cycles_obj_triangulation_candidate import triangulate_snapshot
from sfm_cycles_conversion_candidate import accept_conversion
snapshot=decode_session_snapshot(Session(prepared['job']['session']))
if snapshot is None:raise ValueError('Missing expected session OBJ')
expected=dict(scale=snapshot['scale'],polygons=len(snapshot['polygons']),materials=list(snapshot['images']))
# Launch this verifier only after observing the conversion process exit zero.
accepted=accept_conversion(prepared,0,prepared['provenance']['source_session'],expected)
bpy.ops.wm.open_mainfile(filepath=accepted['input_blend'],load_ui=False,use_scripts=False)
info=accepted['report']['bridge']['imported_obj']
obj=bpy.data.objects.get(info['object'])
if obj is None or obj.type!='MESH':raise ValueError('Converted OBJ mesh is missing')
if any(bpy.data.scenes[name].objects.get(obj.name)!=obj for name in info['shots']):
    raise ValueError('Imported OBJ is missing from a converted shot')
triangles=triangulate_snapshot(snapshot)
mesh=obj.data
assert len(mesh.vertices)==len(triangles)*3 and len(mesh.polygons)==len(triangles)
used={corner[0] for _,corners in triangles for corner in corners}
center=[(min(snapshot['positions'][i][k] for i in used)+max(snapshot['positions'][i][k] for i in used))*.5 for k in range(3)]
normals=snapshot_corner_normals(snapshot)
offsets=[];offset=0
for polygon in snapshot['polygons']:
    offsets.append(offset);offset+=len(polygon['corners'])
vertex_error=normal_error=uv_error=0.
checked_images=set()
saved_normals=mesh.corner_normals
for index,(source_index,corners) in enumerate(triangles):
    original=snapshot['polygons'][source_index]
    polygon=mesh.polygons[index]
    assert len(polygon.vertices)==3
    slot=mesh.materials[polygon.material_index]
    texture=next(n for n in slot.node_tree.nodes if n.type=='TEX_IMAGE')
    source_image=snapshot['images'][original['material']]
    assert texture.image.packed_file and list(texture.image.size)==[source_image['width'],source_image['height']]
    image_key=(texture.image.as_pointer(),original['material'])
    if image_key not in checked_images:
        pixels=np.empty(len(source_image['rgba']),dtype=np.float32)
        texture.image.pixels.foreach_get(pixels)
        assert np.array_equal(np.rint(pixels*255).astype(np.uint8),np.frombuffer(source_image['rgba'],dtype=np.uint8))
        checked_images.add(image_key)
    for corner_index,corner in enumerate(corners):
        position,uv,normal=corner
        x,y,z=[center[k]+(snapshot['positions'][position][k]-center[k])*snapshot['scale'] for k in range(3)]
        target=(-z*.0254,-x*.0254,y*.0254)
        vertex=mesh.vertices[polygon.vertices[corner_index]]
        vertex_error=max(vertex_error,max(abs(vertex.co[k]-target[k]) for k in range(3)))
        loop=polygon.loop_start+corner_index
        expected_uv=snapshot['uvs'][uv][:2] if uv!=0xffffffff else (0.,0.)
        uv_error=max(uv_error,max(abs(mesh.uv_layers.active.data[loop].uv[k]-expected_uv[k]) for k in range(2)))
        expected_normal=normals[offsets[source_index]+original['corners'].index(corner)]
        normal_error=max(normal_error,max(abs(saved_normals[loop].vector[k]-expected_normal[k]) for k in range(3)))
assert vertex_error<2e-6 and uv_error<1e-6 and normal_error<.0005
assert obj['sfm64_obj_scale']==snapshot['scale']
folder=Path(prepared['folder'])
(folder/'embedded-obj-scene-validation.json').write_text(json.dumps(dict(
    shots_verified=len(info['shots']),triangles=len(triangles),packed_image_bytes_exact=True,
    unique_image_comparisons=len(checked_images),
    vertex_max_error=vertex_error,uv_max_error=uv_error,normal_max_error=normal_error,
    limitation='Actual saved geometry/state check; native Source lighting and full renderer parity unverified.'),indent=2))
print('converted_embedded_obj_scene_verified',flush=True)
