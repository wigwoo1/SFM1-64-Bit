"""Geometry checks for the generated XY-plane transparency fixture only."""
import math
import struct


def ordered_faces(data):
    if data[:4] != b'VBSP': raise ValueError('Owned fixture BSP signature')
    def lump(number):
        offset, length = struct.unpack_from('<II', data, 8 + number * 16)
        if offset > len(data) or length > len(data) - offset: raise ValueError('Owned fixture lump range')
        return offset, length
    models, length = lump(14)
    if length < 48: raise ValueError('Owned fixture model record')
    first, count = struct.unpack_from('<II', data, models + 40)
    faces, face_bytes = lump(58)
    vertices, vertex_bytes = lump(3)
    edges, edge_bytes = lump(12)
    surfedges, surfedge_bytes = lump(13)
    if first + count > face_bytes // 56: raise ValueError('Owned fixture face span')
    result = []
    for index in range(first, first + count):
        start, size = struct.unpack_from('<ih', data, faces + index * 56 + 4)
        if start < 0 or size != 4 or start + size > surfedge_bytes // 4:
            raise ValueError('Owned fixture must contain four-edge faces')
        ends = []
        for number in range(start, start + size):
            signed = struct.unpack_from('<i', data, surfedges + number * 4)[0]
            if abs(signed) >= edge_bytes // 4: raise ValueError('Owned fixture edge range')
            edge = struct.unpack_from('<HH', data, edges + abs(signed) * 4)
            ends.append(edge[::-1] if signed < 0 else edge)
        if any(ends[n][1] != ends[(n + 1) % size][0] for n in range(size)):
            raise ValueError('Owned fixture edge loop is not connected')
        points = []
        for a, _ in ends:
            if a >= vertex_bytes // 12: raise ValueError('Owned fixture vertex range')
            point = struct.unpack_from('<fff', data, vertices + a * 12)
            if not all(map(math.isfinite, point)) or point[2] != 0:
                raise ValueError('Owned fixture must be a finite XY plane')
            points.append(point)
        result.append(points)
    return result


def validate_translucent_quad(data, layers):
    faces = ordered_faces(data)
    if len(faces) != layers: raise ValueError('Owned fixture layer count')
    areas = []
    for points in faces:
        turns = []
        for n in range(4):
            a, b, c = [points[(n + k) % 4] for k in range(3)]
            turns.append((b[0]-a[0])*(c[1]-b[1]) - (b[1]-a[1])*(c[0]-b[0]))
        if len(set(points)) != 4 or not (all(v > 0 for v in turns) or all(v < 0 for v in turns)):
            raise ValueError('Owned fixture must be a simple strictly convex quad')
        if points != faces[0]: raise ValueError('Owned fixture layers must cover identical quads')
        twice_area = sum(points[n][0]*points[(n+1)%4][1] - points[n][1]*points[(n+1)%4][0] for n in range(4))
        areas.append(abs(twice_area) / 2)
    return dict(layers=layers, ordered_vertices=faces[0], face_areas=areas,
                triangles_per_face=2, simple_convex=True)


def repair_translucent_fixture_quad(data):
    # The inherited lighting-selection fixture uses 0 -> 1 -> 3 -> 4, whose
    # last edge crosses edge 1 -> 3. This test needs 0 -> 1 -> 3 -> 2 instead.
    edges = struct.unpack_from('<I', data, 8 + 12 * 16)[0]
    if struct.unpack_from('<HH', data, edges + 5 * 4) != (3, 4) or \
       struct.unpack_from('<HH', data, edges + 6 * 4) != (4, 0):
        raise ValueError('Unexpected inherited fixture edge identity')
    struct.pack_into('<HH', data, edges + 5 * 4, 3, 2)
    struct.pack_into('<HH', data, edges + 6 * 4, 2, 0)
