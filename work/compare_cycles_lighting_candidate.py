"""Inspect saved lighting without modifying either converted project."""
import json
import argparse
import sys
from pathlib import Path
import bpy

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser()
parser.add_argument('--candidate', type=Path)
parser.add_argument('--output', type=Path)
arguments = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
inputs = [root / 'outputs/cycles-conversion/short-path-session/session.blend',
          root / 'outputs/cycles-conversion/prepared/5c56456d-c129-47e4-88a9-20bb810892ae/session.blend']
if arguments.candidate is not None:
    inputs[1] = arguments.candidate.resolve()
def scalar(value):
    if isinstance(value, (str, bool, int, float)):
        return value
    try:
        return list(value)
    except TypeError:
        return str(value)
def nodes(tree):
    if tree is None:
        return None
    return dict(nodes={n.name: dict(type=n.type, operation=getattr(n, 'operation', None),
        image=n.image.name if getattr(n, 'image', None) else None,
        inputs=[dict(index=i, name=s.name, value=scalar(s.default_value))
                for i,s in enumerate(n.inputs) if hasattr(s, 'default_value')],
        outputs=[dict(index=i, name=s.name, value=scalar(s.default_value))
                 for i,s in enumerate(n.outputs) if hasattr(s, 'default_value')])
        for n in tree.nodes}, links=sorted((l.from_node.name, l.from_socket.name,
        l.to_node.name, l.to_socket.name) for l in tree.links))
results = []
for path in inputs:
    bpy.ops.wm.open_mainfile(filepath=str(path), load_ui=False, use_scripts=False)
    scene = bpy.data.scenes['shot47 kogenta']
    bpy.context.window.scene = scene
    scene.frame_set(1)
    bpy.context.view_layer.update()
    lights = {}
    for obj in scene.objects:
        if obj.type == 'LIGHT':
            light = obj.data
            lights[obj.name] = dict(type=light.type, energy=light.energy,
                color=list(light.color), use_nodes=light.use_nodes,
                hide_render=obj.hide_render, matrix=[list(row) for row in obj.matrix_world],
                nodes=nodes(light.node_tree))
    results.append(dict(path=str(path), lights=lights,
        world=nodes(scene.world.node_tree) if scene.world else None,
        view=dict(exposure=scene.view_settings.exposure, gamma=scene.view_settings.gamma,
                  transform=scene.view_settings.view_transform),
        camera=scene.camera.name, objects=len(scene.objects)))
    print('lighting_inspected', path.name, flush=True)
folder = root / 'outputs/cycles-conversion/lighting-comparison'
folder.mkdir(parents=True, exist_ok=True)
output = arguments.output or folder / 'detailed-report.json'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(results, indent=2))
baseline, candidate = results
summary = dict(light_counts=[len(baseline['lights']),len(candidate['lights'])],
    missing_lights=sorted(set(baseline['lights'])-set(candidate['lights'])),
    added_lights=sorted(set(candidate['lights'])-set(baseline['lights'])),
    changed_lights=sorted(name for name in baseline['lights'].keys() & candidate['lights'].keys()
                         if baseline['lights'][name] != candidate['lights'][name]),
    world_equal=baseline['world']==candidate['world'], view_equal=baseline['view']==candidate['view'])
output.with_suffix('.summary.json').write_text(json.dumps(summary,indent=2))
print('lighting_comparison_complete', flush=True)
