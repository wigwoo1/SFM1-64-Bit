import struct
from decode_vertex_attributes import decode_vertices

data = bytearray(128)
data[:4] = b'IDSV'
struct.pack_into('<4i', data, 4, 4, 123, 1, 1)
struct.pack_into('<2i', data, 56, 64, 112)
struct.pack_into('<3f', data, 64, .25, .75, 0)
data[76:80] = bytes((2, 1, 255, 2))
struct.pack_into('<8f', data, 80, 1, 2, 3, 0, 0, 1, -.5, 1.5)
struct.pack_into('<4f', data, 112, 1, 0, 0, -1)
vertex = decode_vertices(data, 3)[0]
assert vertex == dict(position=(1, 2, 3), normal=(0, 0, 1), uv=(-.5, 1.5),
                      tangent=(1, 0, 0, -1), weights=(.25, .75, 0),
                      bones=(2, 1, 255), influences=2)
without_tangent = bytearray(data[:112])
struct.pack_into('<i', without_tangent, 60, 0)
assert decode_vertices(without_tangent, 3)[0]['tangent'] is None
rejected = 0
mutations = [(offset, '<f', float('nan')) for offset in (64, 80, 92, 104, 112)]
mutations += [(64, '<f', -.1), (68, '<f', 1.1), (79, '<B', 4),
              (76, '<B', 3), (56, '<i', -1), (60, '<i', 113), (48, '<i', 1)]
for offset, fmt, value in mutations:
    changed = bytearray(data)
    struct.pack_into(fmt, changed, offset, value)
    try:
        decode_vertices(changed, 3)
    except ValueError:
        rejected += 1
    else:
        raise AssertionError((offset, value))
for length in range(len(data)):
    try:
        decode_vertices(data[:length], 3)
    except ValueError:
        rejected += 1
    else:
        raise AssertionError(('accepted truncated attributes', length))
print(f'Vertex attributes exact decode passed; malformed rejections={rejected}')
