"""Verify the complete stationary displacement overlap and triangle predicates.

All original reference bytes stay in the local installed-image fixture/private
disassembly. The independent flat fixtures use exact dyadic coordinates.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile

from verify_sliding_door_spawn import ENGINE, BSP, sha, verify_installed_ranges
from verify_convex_brush_hull import ENGINE_SHA256, BSP_SHA256

ROW = struct.Struct('<9I')
FLAGS = (0, 1, 2, 4, 5, 8, 12, 0xffffffff)


def flat_predictions():
    for x, y, z, radius in itertools.product((-9., -8., -4., 0., 4., 8., 9.),
                                           (-9., -8., -4., 0., 4., 8., 9.),
                                           (-2., -1., 0., 1., 2.), (0., .5, 1., 2., 16.)):
        # Intersect the query's XY rectangle with x>=-8,y>=-8,x+y<=0.
        # Native stored +Z/type2 plane requires strict straddling of z=0.
        low_x, high_x = max(-8., x-radius), min(8., x+radius)
        low_y, high_y = max(-8., y-radius), min(8., y+radius)
        yield low_x <= high_x and low_y <= high_y and low_x+low_y <= 0. and z-radius < 0. < z+radius


def inspect_records(payload):
    assert len(payload) == 233161*ROW.size
    flat = list(flat_predictions())
    counts = dict(cases=0, direct_cases=0, flat_cases=0, mesh_cases=0, meshes=0,
                  hits=0, misses=0, disabled=0, triangle_calls=0,
                  second_triangle_hits=0, multiple_candidates=0, passed=True)
    mesh_ids = set()
    enabled_groups = {}
    face_groups = {}
    equal_face_cases = flat_hits = zero_radius_flat = 0
    for case, row in enumerate(ROW.iter_unpack(payload)):
        kind, mesh, query, variant, hit, attempts, order_hash, first, last = row
        assert kind in (0, 1, 2) and hit in (0, 1)
        counts['cases'] += 1
        counts['hits'] += hit
        counts['misses'] += not hit
        if kind == 2:
            assert case == query and case < 1225 and mesh == 0xffffffff and variant == 0
            assert bool(hit) == flat[query], ('independent flat clipping/plane predicate', query)
            flat_hits += hit
            if query % 5 == 0:
                assert hit == 0, 'zero-volume flat box must fail the plane-side gate'
                zero_radius_flat += 1
            counts['flat_cases'] += 1
        else:
            assert case >= 1225 and mesh < 102
            mesh_ids.add(mesh)
        if kind != 1:
            assert attempts == 0 and order_hash == 2166136261 and first == last == 0xffffffff
            counts['direct_cases'] += 1
            if kind == 0:
                assert query < 512 and variant >> 8 < 12
                assert (variant >> 4) & 15 < 4 and variant & 15 < 2
        else:
            q, hull = query & 255, query >> 8
            assert q < 24 and hull < 4 and variant in FLAGS
            assert attempts <= 512
            assert (attempts == 0) == (first == last == 0xffffffff)
            if attempts:
                assert first < 512 and last < 512 and first % 2 == 0
                assert attempts % 2 == ((last & 1) ^ 1) if hit else attempts % 2 == 0
            if hit:
                assert attempts > 0
            if variant & 4:
                assert hit == attempts == 0 and order_hash == 2166136261
                counts['disabled'] += 1
            else:
                key = mesh, query
                if variant == 0:
                    enabled_groups[key] = row[4:]
                else:
                    assert row[4:] == enabled_groups[key], ('unexpected non-disable flag behavior', key, variant)
            counts['mesh_cases'] += 1
            counts['triangle_calls'] += attempts
            counts['second_triangle_hits'] += hit and bool(last & 1)
            counts['multiple_candidates'] += attempts > 2
        deterministic = kind == 0 and variant >> 8 < 6 or kind == 1 and query & 255 < 8
        if mesh < 86 and deterministic:
            key = kind, mesh % 43, query, variant
            if mesh < 43:
                face_groups[key] = row[4:]
            else:
                assert row[4:] == face_groups[key], ('deterministic LDR/HDR comparison', key)
                equal_face_cases += 1
    counts['meshes'] = len(mesh_ids)
    assert mesh_ids == set(range(102)) and counts['flat_cases'] == 1225
    assert flat_hits > 0 and zero_radius_flat == 245 and equal_face_cases > 40000
    return counts, dict(flat_predictions=1225, flat_hits=flat_hits, zero_radius_flat_rejections=zero_radius_flat,
                        matching_deterministic_ldr_hdr_queries=equal_face_cases,
                        runtime_flag_groups=len(enabled_groups))


def main(directory):
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    image, evidence = verify_installed_ranges(ENGINE, run/'displacement-box-reference.local.txt', [
        ('complete mesh box overlap including reachable triangle tail', 0x1015a7d0, 0x1015abab),
        ('complete triangle box overlap', 0x1014f770, 0x1014fdb2),
        ('complete edge X axis helper', 0x1014f6e0, 0x1014f768),
        ('complete bounds plane side executable body', 0x102446e0, 0x102448f7),
        ('original cookie check', 0x102de362, 0x102de371),
    ])
    # Validate the native jump-table destinations without copying their literals
    # into public source. The table remains private data in the mapped image.
    table = image.b[image.offset(0x102448f8):image.offset(0x102448f8)+32]
    assert len(table) == 32 and all(0x102446e0 <= p < 0x102448f7 for p in struct.unpack('<8I', table))
    baseline = counts = independent = None
    builds = {}
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = directory/f'displacement_box_overlap_{arch}.exe'
        log = (directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log, log
        binary = exe.read_bytes()
        pe, = struct.unpack_from('<I', binary, 60)
        assert binary[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H', binary, pe+4)[0] == machine
        assert struct.unpack_from('<H', binary, pe+24)[0] == (0x10b if arch == 'x86' else 0x20b)
        builds[arch] = dict(executable=str(exe), sha256=sha(exe), machine=hex(machine), warnings=0)
        for repeat in (1, 2):
            target = run/f'{arch}-{repeat}.bin'
            result = subprocess.run([str(exe), str(ENGINE) if arch == 'x86' else '-', str(BSP), str(target)],
                                    capture_output=True, text=True, timeout=45)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode == 0, (arch, repeat, result.returncode, result.stderr)
            current = json.loads(result.stdout)
            assert current == dict(cases=233161, direct_cases=154825, flat_cases=1225, mesh_cases=78336,
                                  meshes=102, hits=64942, misses=168219, disabled=39168,
                                  triangle_calls=29672, second_triangle_hits=5088,
                                  multiple_candidates=980, passed=True)
            payload = target.read_bytes()
            if baseline is None:
                counts, independent = inspect_records(payload)
                assert counts == current, (counts, current)
                baseline = payload
            assert payload == baseline and current == counts, (arch, repeat, 'overlap predicate/order mismatch')
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    root = Path(__file__).resolve().parent
    sources = {}

    def visit(name):
        if name in sources:
            return
        p = root/name
        sources[name] = sha(p)
        for include in re.findall(r'^\s*#include\s*"([^"\r\n]+)"', p.read_text(), re.M):
            visit(include)

    for name in ['probe_displacement_box_overlap.cpp', 'build_displacement_box_overlap.cmd', 'verify_displacement_box_overlap.py']:
        visit(name)
    receipt = dict(status='VERIFIED PREPARED STATIONARY DISPLACEMENT BOX OVERLAP', counts=counts,
                   independent_checks=independent, repeats_per_arch=2, builds=builds, source_sha256=sources,
                   record_bytes=len(baseline), record_sha256=hashlib.sha256(baseline).hexdigest(),
                   originals_unchanged=True, engine_sha256=ENGINE_SHA256, bsp_sha256=BSP_SHA256,
                   verified_spans=[row['name'] for row in evidence], replaced_callees=[],
                   plane_side_jump_table_sha256=hashlib.sha256(table).hexdigest(),
                   production_permutation_required=False,
                   findings=[
                       'Complete original1015a7d0..1015abab executes, including both reachable triangle calls1014f770, edge helper1014f6e0, plane-side102446e0 and cookie check. No callee or geometry is substituted.',
                       'Earlier handoff incorrectly stopped at early ret8. Branch1015a908 reaches1015a9e1 and the same function continues through1015abaa. The handoff now records the complete reachable range.',
                       'Children are selected by inclusive AABB intersection, queued in ascending order, then leaf pairs are tested in native order with vertex winding0,2,1. Runtime flag4 bypasses the tree.',
                       'Native hardware entry observations execute through triangle geometry and validate actual triangle identities, full order, midpoint/extents, zero tolerance and stored plane tags. Complete order vectors, not only hashes, are compared inside the probe.',
                       'The triangle predicate preserves per-edge projection pairs, float rounding and the x87-accumulated Y-axis radius, then requires native plane-side result3. Generic inclusive triangle SAT would have different boundary behavior.',
                       'Direct triangle tests include zero/nonzero boxes, vertices/centroids/normal offsets/random offsets, four box sizes and tolerance0/.03125. Independent flat clipping/plane predictions cover1225dyadic cases.',
                       'Both43-mesh benchmark face modes plus16controlled meshes spanning powers1..4/flat/slope/curved/vertical surfaces are tested. 78336complete mesh queries include eight runtime flag combinations.',
                       'Four architecture runs have identical predicate/order records. Original executable sections, mesh objects and vertex/triangle/tree/pair bytes are unchanged.',
                       'The owned x64 test takes benchmark geometry only and opens no original engine or permutation fixture. No original code/table literals are added to project source.'],
                   limits=[
                       'Prepared mesh/triangle/tree records are supplied; this does not run original whole BSP loading or prove full import orchestration.',
                       'Finite ordered boxes, stored plane metadata and tested floating-point environment only; no universal NaN/infinity or alternate x87-mode claim.',
                       'Stationary leaf/world dispatch, cull/visit policy, displacement normal/ray fallback and final trace remapping remain separate unverified composition gates.',
                       'Server entities/static props/filtering, measured benchmark spawn, material binding and editor integration are not established. Full SFM compatibility remains incomplete.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'], counts=counts, independent_checks=independent, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
