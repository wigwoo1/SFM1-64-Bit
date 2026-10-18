import struct
from map_model_geometry import map_geometry
from test_vtx_geometry_audit import fixture

mdl = bytearray(520)
vvd = bytearray(64 + 15 * 48)
vtx = fixture()
def words(data, offset, *values):
    struct.pack_into('<' + 'i' * len(values), data, offset, *values)
mdl[:4] = b'IDST'
words(mdl, 4, 49, 123)
words(mdl, 76, len(mdl))
words(mdl, 232, 1, 240)
words(mdl, 244, 1, 1, 16)
words(mdl, 328, 1, 148, 13, 96)
words(mdl, 404, 0, -148, 10, 3)
vvd[:4] = b'IDSV'
words(vvd, 4, 4, 123, 1, 15)
words(vvd, 56, 64)
positions, meshes = map_geometry(mdl, vvd, vtx)
assert len(positions) == 15
assert meshes[0]['triangles'] == [[14, 12, 7]]
rejected = 0
for source, offset, value in ((0, 4, 48), (0, 76, 519), (0, 232, -1),
                              (0, 340, 97), (0, 336, 14), (0, 408, -147),
                              (0, 412, 9), (0, 416, 4), (1, 8, 124),
                              (1, 48, 1), (1, 56, -1), (1, 16, 14)):
    inputs = [bytearray(mdl), bytearray(vvd), bytearray(vtx)]
    words(inputs[source], offset, value)
    try:
        map_geometry(*inputs)
    except ValueError:
        rejected += 1
    else:
        raise AssertionError((source, offset, value))
print(f'MDL/VVD/VTX nonzero offset mapping passed; malformed rejections={rejected}')
