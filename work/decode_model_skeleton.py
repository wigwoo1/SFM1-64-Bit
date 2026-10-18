"""MDL49 disk skeleton and default-pose candidate; no procedural evaluation."""
import argparse
import json
import math
import struct
from pathlib import Path


def compose(a, b):
    return tuple(sum(a[r * 4 + k] * b[k * 4 + c] for k in range(3)) +
                 (a[r * 4 + 3] if c == 3 else 0)
                 for r in range(3) for c in range(4))


def local_matrix(position, q):
    x, y, z, w = q
    return (1-2*y*y-2*z*z, 2*x*y-2*z*w, 2*x*z+2*y*w, position[0],
            2*x*y+2*z*w, 1-2*x*x-2*z*z, 2*y*z-2*x*w, position[1],
            2*x*z-2*y*w, 2*y*z+2*x*w, 1-2*x*x-2*y*y, position[2])


def decode_skeleton(mdl):
    if len(mdl) < 164 or mdl[:4] != b'IDST' or struct.unpack_from('<i', mdl, 4)[0] != 49:
        raise ValueError('MDL49 skeleton header')
    count, start = struct.unpack_from('<2i', mdl, 156)
    if count < 0 or start < 0 or start > len(mdl) or count * 216 > len(mdl) - start:
        raise ValueError('MDL bone span')
    bones = []
    for n in range(count):
        base = start + n * 216
        name_offset, parent = struct.unpack_from('<2i', mdl, base)
        name_start = base + name_offset
        if not 0 <= name_start < len(mdl):
            raise ValueError('bone name span')
        end = mdl.find(b'\0', name_start)
        if end < 0:
            raise ValueError('unterminated bone name')
        if parent < -1 or parent >= count or parent == n:
            raise ValueError('bone parent')
        position = struct.unpack_from('<3f', mdl, base + 32)
        quaternion = struct.unpack_from('<4f', mdl, base + 44)
        inverse_bind = struct.unpack_from('<12f', mdl, base + 96)
        if not all(math.isfinite(v) for v in position + quaternion + inverse_bind):
            raise ValueError('nonfinite bone transform')
        if abs(sum(v*v for v in quaternion) - 1) > 1e-3:
            raise ValueError('nonunit bone quaternion')
        bones.append(dict(name=mdl[name_start:end].decode('utf-8', errors='replace'), parent=parent,
                          position=position, quaternion=quaternion, inverse_bind=inverse_bind,
                          flags=struct.unpack_from('<i', mdl, base + 160)[0],
                          procedure_type=struct.unpack_from('<i', mdl, base + 164)[0]))
    # Resolve arbitrary parent order without recursion; detect cycles.
    pending = set(range(count))
    while pending:
        ready = [n for n in sorted(pending) if bones[n]['parent'] == -1 or bones[n]['parent'] not in pending]
        if not ready:
            raise ValueError('cyclic skeleton')
        for n in ready:
            bone = bones[n]
            local = local_matrix(bone['position'], bone['quaternion'])
            bone['global_bind'] = local if bone['parent'] == -1 else compose(bones[bone['parent']]['global_bind'], local)
            bone['bind_skin_matrix'] = compose(bone['global_bind'], bone['inverse_bind'])
            pending.remove(n)
    return bones


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('input')
    parser.add_argument('output')
    args = parser.parse_args()
    bones = decode_skeleton(Path(args.input).read_bytes())
    identity = (1,0,0,0,0,1,0,0,0,0,1,0)
    error = max((abs(v - expected) for b in bones for v, expected in zip(b['bind_skin_matrix'], identity)), default=0)
    result = dict(bone_count=len(bones), roots=sum(b['parent'] == -1 for b in bones),
                  procedural_bones=sum(b['procedure_type'] != 0 for b in bones),
                  maximum_bind_identity_error=error, bones=bones)
    Path(args.output).write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(f"Skeleton: bones={len(bones)} roots={result['roots']} procedural={result['procedural_bones']} bind_identity_error={error:.9g}")
