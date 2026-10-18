"""Render the persisted actual OBJ fixture with a fitted diagnostic camera."""
from pathlib import Path
import sys,json
import bpy
from mathutils import Vector
root = Path(__file__).resolve().parent.parent
folder = root/'outputs/cycles-conversion/obj-creation'
integrated='--integrated' in sys.argv
if integrated:
    prepared=json.loads((root/'outputs/cycles-conversion/embedded-prepared-job.json').read_text())
    folder=Path(prepared['folder'])
bpy.ops.wm.open_mainfile(filepath=str(folder/('session.blend' if integrated else 'import.blend')),load_ui=False,use_scripts=False)
if integrated:bpy.context.window.scene=bpy.data.scenes['shot47 kogenta']
scene=bpy.context.scene
scene.frame_set(1)
obj=bpy.data.objects['Imported OBJ']
uv_test='--uv-test' in sys.argv
if uv_test:
    coords=[(0,0),(1,0),(1,1),(0,0),(1,1),(0,1)]
    for loop in obj.data.loops:
        obj.data.uv_layers.active.data[loop.index].uv=coords[loop.vertex_index]
    image=bpy.data.images.new('UV quadrant diagnostic',width=2,height=2,alpha=True)
    image.pixels.foreach_set([1,0,0,1, 0,1,0,1, 0,0,1,1, 1,1,0,1])
    image.pack()
    for material in obj.data.materials:
        for node in material.node_tree.nodes:
            if node.type=='TEX_IMAGE':node.image=image
points=[obj.matrix_world@v.co for v in obj.data.vertices]
center=sum(points,Vector())/len(points)
diameter=max((p-center).length for p in points)*2
normal=(points[1]-points[0]).cross(points[2]-points[0]).normalized()
camera=bpy.data.objects.new('Diagnostic camera',bpy.data.cameras.new('Diagnostic camera'))
scene.collection.objects.link(camera)
camera.location=center+normal*diameter*3
camera.rotation_euler=(center-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.type='ORTHO';camera.data.ortho_scale=diameter*1.3
camera.data.clip_start=.00001;camera.data.clip_end=100
scene.camera=camera
scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=4
scene.cycles.use_denoising=False
scene.render.resolution_x=scene.render.resolution_y=256
scene.render.resolution_percentage=100
scene.view_settings.view_transform='Standard'
scene.render.image_settings.file_format='PNG'
scene.render.filepath=str(folder/('uv-render.png' if uv_test else 'import-diagnostic-render.png' if integrated else 'render.png'))
if uv_test:
    from bpy_extras.object_utils import world_to_camera_view
    bpy.context.view_layer.update()
    samples=[]
    for u,v,color in [(.2,.3,[255,0,0]),(.7,.2,[0,255,0]),(.3,.8,[0,0,255]),(.8,.7,[255,255,0])]:
        point=(points[0]*(1-u)+points[1]*(u-v)+points[2]*v if u>=v else
               points[0]*(1-v)+points[2]*u+points[5]*(v-u))
        screen=world_to_camera_view(scene,camera,point)
        samples.append(dict(uv=[u,v],pixel=[int(screen.x*256),255-int(screen.y*256)],expected=color))
    (folder/'uv-samples.json').write_text(json.dumps(samples,indent=2))
bpy.ops.render.render(write_still=True)
print('persisted_obj_cycles_render_complete',flush=True)
