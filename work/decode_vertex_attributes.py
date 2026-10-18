"""Decode raw VVD4 vertices for the geometry candidate; never normalize inputs."""
import math
import struct


def decode_vertices(vvd, bone_count):
    if len(vvd) < 64 or vvd[:4] != b'IDSV' or struct.unpack_from('<i', vvd, 4)[0] != 4:
        raise ValueError('VVD4 header')
    if bone_count < 0 or struct.unpack_from('<i', vvd, 48)[0] != 0:
        raise ValueError('unsupported vertex context')
    count = struct.unpack_from('<i', vvd, 16)[0]
    start, tangent_start = struct.unpack_from('<2i', vvd, 56)
    if count < 0 or start < 64 or start > len(vvd) or count * 48 > len(vvd) - start:
        raise ValueError('VVD vertex span')
    if tangent_start and (tangent_start < 64 or tangent_start > len(vvd) or count * 16 > len(vvd) - tangent_start):
        raise ValueError('VVD tangent span')
    vertices = []
    for n in range(count):
        offset = start + n * 48
        weights = struct.unpack_from('<3f', vvd, offset)
        bones = tuple(vvd[offset + 12:offset + 15])
        influences = vvd[offset + 15]
        position = struct.unpack_from('<3f', vvd, offset + 16)
        normal = struct.unpack_from('<3f', vvd, offset + 28)
        uv = struct.unpack_from('<2f', vvd, offset + 40)
        tangent = struct.unpack_from('<4f', vvd, tangent_start + n * 16) if tangent_start else None
        if not all(math.isfinite(x) for x in weights + position + normal + uv + (tangent or ())):
            raise ValueError('nonfinite vertex attribute')
        if influences > 3 or any(b >= bone_count for b in bones[:influences]):
            raise ValueError('vertex bone range')
        if any(w < 0 or w > 1 for w in weights[:influences]):
            raise ValueError('vertex weight range')
        vertices.append(dict(position=position, normal=normal, uv=uv, tangent=tangent,
                             weights=weights, bones=bones, influences=influences))
    return vertices
