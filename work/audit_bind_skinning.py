"""Check decoded default skeleton against raw VVD bind positions."""
import argparse
import json
from pathlib import Path
from decode_model_skeleton import decode_skeleton
from decode_vertex_attributes import decode_vertices

parser = argparse.ArgumentParser()
parser.add_argument('base')
parser.add_argument('output')
args = parser.parse_args()
bones = decode_skeleton(Path(args.base + '.mdl').read_bytes())
vertices = decode_vertices(Path(args.base + '.vvd').read_bytes(), len(bones))
maximum = 0
for vertex in vertices:
    p = vertex['position']
    if not vertex['influences']:
        raise ValueError('zero-influence skinning semantics unverified')
    result = [0.0, 0.0, 0.0]
    for n in range(vertex['influences']):
        matrix = bones[vertex['bones'][n]]['bind_skin_matrix']
        weight = vertex['weights'][n]
        for r in range(3):
            result[r] += weight * (sum(matrix[r*4+k] * p[k] for k in range(3)) + matrix[r*4+3])
    maximum = max(maximum, *(abs(result[r] - p[r]) for r in range(3)))
report = dict(vertices=len(vertices), bones=len(bones), maximum_position_component_error=maximum,
              arithmetic='Python double, unnormalized source weights', native_parity_verified=False)
Path(args.output).write_text(json.dumps(report, indent=2), encoding='utf-8')
print(f'Bind skinning check: vertices={len(vertices)} maximum_position_component_error={maximum:.9g}')
