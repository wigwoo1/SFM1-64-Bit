"""Explain visibility in the actual converted shot without modifying it."""
import json
from pathlib import Path
import bpy
from bpy_extras.object_utils import world_to_camera_view
root=Path(__file__).resolve().parent.parent
prepared=json.loads((root/'outputs/cycles-conversion/embedded-prepared-job.json').read_text())
folder=Path(prepared['folder'])
bpy.ops.wm.open_mainfile(filepath=prepared['job']['output'],load_ui=False,use_scripts=False)
scene=bpy.data.scenes['shot47 kogenta']
bpy.context.window.scene=scene
scene.frame_set(1);bpy.context.view_layer.update()
obj=bpy.data.objects['Imported OBJ']
projected=[list(world_to_camera_view(scene,scene.camera,obj.matrix_world@v.co)) for v in obj.data.vertices]
report=dict(scene=scene.name,frame=1,projected_vertices=projected,
    clip_start=scene.camera.data.clip_start,clip_end=scene.camera.data.clip_end,
    object_hide_render=obj.hide_render,
    scope='Projection/depth only; occlusion and native Source framing not yet compared.')
(folder/'import-camera-projection.json').write_text(json.dumps(report,indent=2))
print('embedded_obj_camera_projection_complete',flush=True)
