"""Checked MDL49/VVD4/SFM-VTX7 LOD0 geometry candidate, OBJ diagnostic."""
import argparse
import math
import struct
from pathlib import Path
from audit_vtx_geometry import inspect
from decode_vertex_attributes import decode_vertices


def map_geometry(mdl, vvd, vtx):
    def span(data, offset, count, stride):
        if min(offset, count) < 0 or offset > len(data) or count * stride > len(data) - offset:
            raise ValueError('model geometry range')
    def word(data, offset):
        span(data, offset, 1, 4)
        return struct.unpack_from('<i', data, offset)[0]
    def table(base, count_offset, start_offset, stride):
        count = word(mdl, base + count_offset)
        start = base + word(mdl, base + start_offset)
        span(mdl, start, count, stride)
        return [start + n * stride for n in range(count)]
    span(mdl, 0, 1, 240)
    if mdl[:4] != b'IDST' or word(mdl, 4) != 49 or word(mdl, 76) != len(mdl):
        raise ValueError('MDL49 header')
    checksum = word(mdl, 8) & 0xffffffff
    span(vvd, 0, 1, 64)
    if vvd[:4] != b'IDSV' or word(vvd, 4) != 4 or word(vvd, 8) & 0xffffffff != checksum:
        raise ValueError('VVD4 header/checksum')
    if not 1 <= word(vvd, 12) <= 8 or word(vvd, 48) != 0:
        raise ValueError('candidate requires unfixed VVD LOD0')
    count, start = word(vvd, 16), word(vvd, 56)
    if start < 64:
        raise ValueError('VVD vertex offset')
    span(vvd, start, count, 48)
    positions = [struct.unpack_from('<3f', vvd, start + n * 48 + 16) for n in range(count)]
    if not all(math.isfinite(v) for p in positions for v in p):
        raise ValueError('nonfinite geometry')
    metadata = {}
    for body_id, body in enumerate(table(0, 232, 236, 16)):
        for model_id, model in enumerate(table(body, 4, 12, 148)):
            model_count, vertex_bytes = word(mdl, model + 80), word(mdl, model + 84)
            if vertex_bytes < 0 or vertex_bytes % 48 or model_count < 0:
                raise ValueError('MDL model vertex span')
            model_first = vertex_bytes // 48
            if model_first + model_count > count:
                raise ValueError('MDL model exceeds VVD')
            for mesh_id, mesh in enumerate(table(model, 72, 76, 116)):
                mesh_count, mesh_first = word(mdl, mesh + 8), word(mdl, mesh + 12)
                if min(mesh_count, mesh_first) < 0 or mesh_first + mesh_count > model_count:
                    raise ValueError('MDL mesh exceeds model')
                if mesh + word(mdl, mesh + 4) != model:
                    raise ValueError('MDL mesh parent')
                metadata[body_id, model_id, mesh_id] = (model_first + mesh_first, mesh_count, word(mdl, mesh))
    topology = inspect(vtx, checksum)
    meshes = []
    for source in topology['meshes']:
        if source['lod'] != 0:
            continue
        key = source['body'], source['model'], source['mesh']
        if key not in metadata:
            raise ValueError('VTX mesh absent from MDL')
        first, mesh_count, material = metadata.pop(key)
        triangles = source['triangles']
        if any(v < 0 or v >= mesh_count for tri in triangles for v in tri):
            raise ValueError('VTX original ID exceeds MDL mesh')
        meshes.append(dict(body=key[0], model=key[1], mesh=key[2], material=material,
                           triangles=[[first + v for v in tri] for tri in triangles]))
    if metadata:
        raise ValueError('MDL meshes absent from VTX')
    return positions, meshes


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('base')
    parser.add_argument('output')
    args = parser.parse_args()
    mdl, vvd, vtx = (Path(args.base + suffix).read_bytes() for suffix in ('.mdl', '.vvd', '.dx90.vtx'))
    positions, meshes = map_geometry(mdl, vvd, vtx)
    vertices = decode_vertices(vvd, struct.unpack_from('<i', mdl, 156)[0])
    with Path(args.output).open('w', encoding='ascii') as out:
        out.write('# Diagnostic bind-space geometry; all body/model alternatives, no materials or deformation\n')
        for p in positions:
            out.write('v %.9g %.9g %.9g\n' % p)
        for vertex in vertices:
            out.write('vt %.9g %.9g\n' % vertex['uv'])
        for vertex in vertices:
            out.write('vn %.9g %.9g %.9g\n' % vertex['normal'])
        for mesh in meshes:
            out.write('g body_%d_model_%d_mesh_%d\n' % (mesh['body'], mesh['model'], mesh['mesh']))
            for tri in mesh['triangles']:
                out.write('f ' + ' '.join(f'{v + 1}/{v + 1}/{v + 1}' for v in tri) + '\n')
    print(f"Mapped geometry: vertices={len(positions)} meshes={len(meshes)} triangles={sum(len(m['triangles']) for m in meshes)}")
