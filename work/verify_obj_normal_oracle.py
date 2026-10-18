import json
import struct
from pathlib import Path
from cycles_obj_snapshot_candidate import snapshot_corner_normals, MISSING
root=Path(__file__).resolve().parent.parent
cases=[json.loads(line) for line in (root/'outputs/cycles-conversion/normal-oracle.jsonl').read_text().splitlines()]
maximum=0.
for case in cases:
    mode=case['mode']
    corner=lambda i:(i,MISSING,MISSING)
    snapshot=dict(positions=[(0,0,0),(1,0,0),(0,1,0),(0,0,2)],normals=[(0,2,0)],
        polygons=[dict(smoothing='1' if mode else 'off',corners=[corner(0),corner(1),corner(2)]),
                  dict(smoothing='1' if mode else 'off',corners=[corner(0),corner(2),corner(3)])])
    if mode==2:snapshot['polygons'][0]['corners'][0]=(0,MISSING,0)
    actual=snapshot_corner_normals(snapshot)
    assert len(actual)==len(case['normal_bits'])
    for calculated,bits in zip(actual,case['normal_bits']):
        x,y,z=[struct.unpack('<f',struct.pack('<I',b))[0] for b in bits]
        expected=(-z,-x,y)
        maximum=max(maximum,max(abs(a-b) for a,b in zip(calculated,expected)))
        assert tuple(struct.unpack('<f',struct.pack('<f',a))[0] for a in calculated)==expected
report=dict(cases=len(cases),corners=18,float32_components_equal=True,max_double_to_float_error=maximum,
    scope='Compiled native flat, area-weighted shared-group and supplied-normal precedence fixtures; broader assets unverified.')
(root/'outputs/cycles-conversion/normal-oracle-validation.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report))
