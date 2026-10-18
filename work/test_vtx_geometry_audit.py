import struct
from pathlib import Path
from audit_vtx_geometry import inspect


def fixture():
    data = bytearray(174)
    def ints(offset, *values):
        struct.pack_into('<' + 'i' * len(values), data, offset, *values)
    ints(0, 7)
    ints(16, 123, 1, 0, 1, 36)
    ints(36, 1, 8)  # body -> model
    ints(44, 1, 8)  # model -> LOD
    ints(52, 1, 12)  # LOD -> mesh
    ints(64, 1, 9)  # mesh -> group
    ints(73, 3, 68, 3, 95, 1, 33)  # group vertex/index/strip spans
    ints(106, 3, 0, 3, 0)
    data[124] = 1  # triangle list
    # Vertex IDs deliberately reorder the original MDL mesh vertices.
    for vertex, original in zip((141, 150, 159), (7, 2, 9)):
        struct.pack_into('<H', data, vertex + 4, original)
    struct.pack_into('<3H', data, 168, 2, 0, 1)
    return data


data = fixture()
assert inspect(data, 123)['meshes'][0]['triangles'] == [[9, 7, 2]]
rejections = 0
for length in range(len(data)):
    try:
        inspect(data[:length], 123)
    except ValueError:
        rejections += 1
    else:
        raise AssertionError(('accepted truncation', length))
for offset, value in ((0, 8), (16, 124), (20, 9), (28, -1),
                      (32, 175), (44, 2), (73, 4), (77, -100),
                      (106, 4), (110, -1), (114, 4), (125, -1),
                      (133, 1)):
    mutated = bytearray(data)
    struct.pack_into('<i', mutated, offset, value)
    try:
        inspect(mutated, 123)
    except ValueError:
        rejections += 1
    else:
        raise AssertionError(('accepted mutation', offset, value))
mutated = bytearray(data)
struct.pack_into('<H', mutated, 168, 3)
try:
    inspect(mutated, 123)
except ValueError:
    rejections += 1
else:
    raise AssertionError('accepted invalid index')
print(f'VTX candidate synthetic mapping passed; malformed rejections={rejections}')
