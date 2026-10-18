"""Inspect installed SFM packed VTX7 topology (33/35-byte group/strip).

This is an explicit branch candidate, not a generic VTX7 decoder.
Indices remain local to each MDL mesh. Only triangle-list strips are admitted.
"""
import argparse
import json
import struct
from pathlib import Path


def inspect(data, checksum):
    def span(offset, count, stride):
        if offset < 0 or count < 0 or offset > len(data) or count * stride > len(data) - offset:
            raise ValueError('VTX range')
        return range(count)

    def i(offset):
        span(offset, 1, 4)
        return struct.unpack_from('<i', data, offset)[0]

    def table(base, count_offset, relative_offset, stride):
        count = i(base + count_offset)
        start = base + i(base + relative_offset)
        return [start + n * stride for n in span(start, count, stride)]

    span(0, 1, 36)
    if i(0) != 7 or (i(16) & 0xffffffff) != checksum:
        raise ValueError('VTX version/checksum')
    lod_count = i(20)
    if not 1 <= lod_count <= 8:
        raise ValueError('VTX LOD count')
    meshes = []
    for bp, body in enumerate(table(0, 28, 32, 8)):
        for model_id, model in enumerate(table(body, 0, 4, 8)):
            lods = table(model, 0, 4, 12)
            if len(lods) != lod_count:
                raise ValueError('inconsistent VTX LOD count')
            for lod_id, lod in enumerate(lods):
                for mesh_id, mesh in enumerate(table(lod, 0, 4, 9)):
                    triangles = []
                    group_count = 0
                    strip_count = 0
                    for group in table(mesh, 0, 4, 33):
                        group_count += 1
                        table(group, 25, 29, 2)
                        vertices = table(group, 0, 4, 9)
                        original = [struct.unpack_from('<H', data, v + 4)[0] for v in vertices]
                        indices = table(group, 8, 12, 2)
                        index_values = [struct.unpack_from('<H', data, v)[0] for v in indices]
                        if any(v >= len(vertices) for v in index_values):
                            raise ValueError('VTX vertex index')
                        for strip in table(group, 16, 20, 35):
                            strip_count += 1
                            count, first, vertex_count, vertex_first = struct.unpack_from('<4i', data, strip)
                            if min(count, first, vertex_count, vertex_first) < 0 or first + count > len(indices) or vertex_first + vertex_count > len(vertices):
                                raise ValueError('VTX strip range')
                            # Validate the bone-change span, even though this audit does not skin.
                            table(strip, 19, 23, 8)
                            topology_count, topology_first = struct.unpack_from('<2i', data, strip + 27)
                            if min(topology_count, topology_first) < 0 or topology_first + topology_count > i(group + 25):
                                raise ValueError('VTX strip topology range')
                            flags = data[strip + 18]
                            values = [original[v] for v in index_values[first:first + count]]
                            if flags == 1:
                                if count % 3:
                                    raise ValueError('VTX triangle-list count')
                                triangles.extend([values[n:n + 3] for n in range(0, count, 3)])
                            else:
                                raise ValueError('unsupported VTX strip flags')
                    meshes.append(dict(body=bp, model=model_id, lod=lod_id, mesh=mesh_id,
                                       groups=group_count, strips=strip_count,
                                       triangle_count=len(triangles), triangles=triangles))
    return dict(checksum=checksum, lod_count=lod_count, meshes=meshes,
                triangle_count=sum(m['triangle_count'] for m in meshes))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('input')
    parser.add_argument('output')
    parser.add_argument('--checksum', type=int, required=True)
    args = parser.parse_args()
    result = inspect(Path(args.input).read_bytes(), args.checksum)
    Path(args.output).write_text(json.dumps(result, separators=(',', ':')), encoding='utf-8')
    print(f"VTX topology: meshes={len(result['meshes'])} triangles={result['triangle_count']}")
