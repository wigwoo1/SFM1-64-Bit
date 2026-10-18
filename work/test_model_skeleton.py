import struct
from decode_model_skeleton import decode_skeleton, compose, local_matrix

mdl = bytearray(600)
mdl[:4] = b'IDST'
struct.pack_into('<i', mdl, 4, 49)
struct.pack_into('<2i', mdl, 156, 2, 164)
mdl[596:600] = b'a\0b\0'
identity = (1,0,0,0,0,1,0,0,0,0,1,0)
for base, parent, position, inverse_translation, name in (
        (164, 1, (4,5,6), (-5,-7,-9), 596),
        (380, -1, (1,2,3), (-1,-2,-3), 598)):
    struct.pack_into('<2i', mdl, base, name-base, parent)
    struct.pack_into('<3f', mdl, base+32, *position)
    struct.pack_into('<4f', mdl, base+44, 0,0,0,1)
    inverse = list(identity)
    inverse[3], inverse[7], inverse[11] = inverse_translation
    struct.pack_into('<12f', mdl, base+96, *inverse)
bones = decode_skeleton(mdl)
assert bones[0]['global_bind'][3::4] == (5,7,9)
assert all(b['bind_skin_matrix'] == identity for b in bones)
rotation = local_matrix((0,0,0), (0,0,1,0))
assert compose(rotation, rotation) == identity
rejected = 0
for offset, fmt, value in ((156,'<i',-1),(160,'<i',601),(168,'<i',0),
                           (168,'<i',2),(384,'<i',0),(164,'<i',600),
                           (208,'<f',float('nan')),(260,'<f',float('inf')),
                           (220,'<f',0)):
    changed = bytearray(mdl)
    struct.pack_into(fmt, changed, offset, value)
    try:
        decode_skeleton(changed)
    except ValueError:
        rejected += 1
    else:
        raise AssertionError((offset,value))
print(f'Skeleton parent-order, composition and bind identity passed; malformed rejections={rejected}')
