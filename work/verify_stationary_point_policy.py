"""Verify stationary point dispatch with actual direct overlap and nested rays.

Original binaries, map/table inputs and evidence stay external and private.
The point fixture executes intact native callees without substituted services.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile

from verify_sliding_door_spawn import ENGINE, BSP, sha, verify_installed_ranges
from verify_benchmark_world_hull import ENGINE_SHA256, BSP_SHA256, PERMUTATION_SHA256
from verify_stationary_hull_leaf import Benchmark, initial_trace, visit_bits
from verify_stationary_displacement_hull_policy import f32
from verify_prepared_point_world import read_marks

TOTAL, BENCHMARK, CONTROLLED = 60354, 43344, 17010
CHANGED_SHARED = {'source_stationary_displacement_hull_policy_candidate.h'}


def inspect_records(payload, benchmark):
    assert struct.unpack_from('<8I', payload) == (0x53505031, BENCHMARK, CONTROLLED, 560, 1866, 43, 1, 2)
    counts = dict(cases=0, benchmark_cases=BENCHMARK, controlled_cases=CONTROLLED,
                  direct_overlaps=0, fallbacks=0, fallback_solid=0, cull_tests=0,
                  mesh_overlap_tests=0, zero_bounds=0, tiny_bounds=0, wrapped_generations=0,
                  binding_checks=(TOTAL+30)//31, distinct_contents_hits=0, passed=True)
    checks = dict(parent_visit_policy=0, full_direct_work_predictions=0,
                  fallback_ray_and_guard_predictions=0, nonswept_child_disp_preservation=0,
                  controlled_cull_overlap_normal_predictions=0, controlled_child_brush_predictions=0,
                  known_positive_point_fallbacks=0, tiny_point_classification=0,
                  identical_ldr_hdr_queries=BENCHMARK//2)
    offset = 32; half = None
    for case in range(TOTAL):
        row = struct.unpack_from('<17I', payload, offset); offset += 68
        point, extents = struct.unpack('<6f', struct.pack('<6I', *row[:6]))[:3], struct.unpack('<3f', struct.pack('<3I', *row[3:6]))
        mask, mode, visit, index, phase, q, listing, config, disabled, nc, no = row[6:]
        assert mode in (0, 1) and visit in (0, 1, 2) and phase == int(case >= BENCHMARK)
        assert config in (0, 1, 2) and disabled in (0, 1, 2) and q < 7
        brush_count, disp_count = (1, 2) if phase else (1866, 43)
        culls = struct.unpack_from('<'+'I'*nc, payload, offset); offset += 4*nc
        overlaps = struct.unpack_from('<'+'I'*no, payload, offset); offset += 4*no
        direct, fallback, nx, ny, nz, normal_contents = struct.unpack_from('<6I', payload, offset); offset += 24
        normal = struct.unpack('<3f', struct.pack('<3I', nx, ny, nz))
        work = payload[offset:offset+560]; offset += 560
        tables = []
        for size in (disp_count, brush_count, disp_count):
            packed = payload[offset:offset+(size+7)//8]; offset += len(packed)
            tables.append(read_marks(packed, size))
        parent, child_brush, child_disp = tables
        assert fallback in (0, 1) and bool(fallback) == (direct == 0xffffffff)
        assert direct == 0xffffffff or direct < disp_count
        assert all(i < disp_count for i in culls+overlaps) and no <= nc
        assert sum(float(v)*v for v in extents) < 1e-6
        checks['tiny_point_classification'] += any(extents)
        if phase:
            refs = ((1, 0), (0, 1), (1, 1, 0))[listing]
            mesh_contents = (1, 2)
        else:
            nxt = (index+1) % disp_count
            refs = (nxt, index, nxt) if listing else (index, nxt, index)
            mesh_contents = benchmark.disp_contents
            assert listing == q % 2 and config == disabled == 0
        visit_index = [0 if config == 1 else i for i in range(disp_count)]
        cull_contents = list(mesh_contents)
        if config == 2:
            cull_contents[0] = 2
        expected_parent = set()
        if visit:
            expected_parent.add(visit_index[(index + (visit == 2)) % disp_count])
        predicted_culls = []
        if any(extents):
            for mesh in refs:
                slot = visit_index[mesh]
                if not cull_contents[mesh] & mask or slot in expected_parent:
                    continue
                expected_parent.add(slot); predicted_culls.append(mesh)
                if mesh == direct:
                    break
        assert parent == expected_parent and culls == tuple(predicted_culls), ('point visit policy', case)
        checks['parent_visit_policy'] += 1
        seed = bytearray(b'\xa7'*560)
        for at, values in ((0, point), (12, (-91., -92., -93.)), (0x18, tuple(-v for v in extents)),
                           (0x24, extents), (0x30, extents)):
            struct.pack_into('<3f', seed, at, *values)
        seed[0x54:0xa8] = initial_trace(mode)
        struct.pack_into('<I', seed, 0xfc, mask); seed[0x100:0x102] = b'\1\0'
        for at, value in ((0x104, 0), (0x114, 0xffffffff if mode else 0), (0x11c, 0),
                          (0x120, 17), (0x124, 0xffffffff if visit == 2 else 0), (0x128, 0),
                          (0x13c, 0), (0x148, brush_count), (0x150, 0), (0x164, 0), (0x170, disp_count)):
            struct.pack_into('<I', seed, at, value)
        if direct != 0xffffffff:
            assert direct == overlaps[-1] == culls[-1] and any(extents)
            expected = bytearray(seed)
            struct.pack_into('<fI', expected, 0x80, 0., mesh_contents[direct])
            expected[0x8a:0x8c] = b'\1\1'; struct.pack_into('<f', expected, 0x8c, 0.)
            assert work == expected, ('direct branch changed metadata', case)
            assert child_brush == (set(range(brush_count)) if visit == 2 else set())
            assert child_disp == (set(range(disp_count)) if visit == 2 else set())
            assert (normal, normal_contents) == ((0., 0., 0.), 0)
            checks['full_direct_work_predictions'] += 1
        else:
            expected = bytearray(seed)
            delta = []
            for axis in range(3):
                step = f32(normal[axis]*99999.8984375)
                end = f32(point[axis]+step)
                d = f32(end-point[axis]); delta.append(d)
                inverse = f32(1./d) if d else struct.unpack('<f', bytes.fromhex('ffff7f7f'))[0]
                struct.pack_into('<f', expected, 0x3c+4*axis, d)
                struct.pack_into('<f', expected, 0x48+4*axis, inverse)
            struct.pack_into('<3f', expected, 0x108, *normal)
            struct.pack_into('<I', expected, 0x124, 1)
            allowed = set(range(0x6c, 0x98)) | set(range(0x9e, 0xa0)) | set(range(0x114, 0x118))
            assert all(a == b for i, (a, b) in enumerate(zip(expected, work)) if i not in allowed), ('fallback work guards', case)
            assert not child_disp
            assert struct.unpack_from('<f', work, 0x80)[0] in (0., 1.)
            assert struct.unpack_from('<f', work, 0x8c)[0] == 0.
            marker = struct.unpack_from('<I', work, 0x114)[0]
            assert marker in (0, 1)
            solid = bool(work[0x8b])
            assert bool(work[0x8a]) == solid and struct.unpack_from('<f', work, 0x80)[0] == (0. if solid else 1.)
            if solid:
                assert marker == 1
            else:
                assert struct.unpack_from('<I', work, 0x84)[0] == 0
            name = struct.unpack_from('<I', work, 0x90)[0]
            prop, flags = struct.unpack_from('<2H', work, 0x94)
            if name == 0xffffffff:
                assert (prop, flags) == (0x3344, 0)
            elif 0x20000000 <= name < 0x20000000+disp_count:
                triangle_flags = struct.unpack_from('<H', work, 0x88)[0]
                assert prop == (500 if triangle_flags & 16 else 100)+name-0x20000000 and flags == 0
            else:
                material = name-0x10000000
                assert 0 <= material < 87 and prop == 1000+material and flags == benchmark.flags[material]
            checks['fallback_ray_and_guard_predictions'] += 1
            checks['nonswept_child_disp_preservation'] += 1
        if phase:
            # Analytic fixture: vertical planes at X=4/-normal and X=50/+normal.
            # Inclusive cull boxes extend one unit beyond their vertex bounds.
            low = tuple(f32(point[a]-extents[a]) for a in range(3))
            high = tuple(f32(point[a]+extents[a]) for a in range(3))
            selected = set()
            if visit:
                selected.add(visit_index[(index+(visit == 2)) % 2])
            predicted_bounds = []; predicted_overlap = []; predicted_direct = 0xffffffff
            if any(extents):
                for mesh in refs:
                    slot = visit_index[mesh]
                    if not cull_contents[mesh] & mask or slot in selected:
                        continue
                    selected.add(slot); predicted_bounds.append(mesh)
                    x = 4. if mesh == 0 else 50.
                    bounds_low, bounds_high = (x-1., -9., -9.), (x+1., 9., 9.)
                    if any(low[a] > bounds_high[a] or bounds_low[a] > high[a] for a in range(3)):
                        continue
                    predicted_overlap.append(mesh)
                    if mesh == 0 and disabled != 1 and low[0] < x < high[0]:
                        predicted_direct = mesh; break
            assert (culls, overlaps, direct) == (tuple(predicted_bounds), tuple(predicted_overlap), predicted_direct), ('analytic point overlap', case)
            if fallback:
                predicted_normal = (1., 0., 0.) if refs[0] == 1 else (-1., 0., 0.)
                predicted_contents = 1
                for mesh in refs:
                    x = 4. if mesh == 0 else 50.
                    if mesh_contents[mesh] & mask and x-1 <= point[0] <= x+1:
                        predicted_normal = (-1., 0., 0.) if mesh == 0 else (1., 0., 0.)
                        predicted_contents = mesh_contents[mesh]; break
                assert normal == predicted_normal and normal_contents == predicted_contents
                assert child_brush == ({0} if point[0] <= extents[0] or normal[0] < 0 else set())
                checks['controlled_child_brush_predictions'] += 1
                if q in (0, 1) and listing != 1 and mask&1 and disabled != 2 and (config != 2 or mask&2):
                    assert work[0x8b] == 1 and struct.unpack_from('<I', work, 0x114)[0] == 1, (case, q, listing, mask, mode, visit, config, disabled, extents, normal, work[0x8b], struct.unpack_from('<I', work, 0x114)[0])
                    checks['known_positive_point_fallbacks'] += 1
            checks['controlled_cull_overlap_normal_predictions'] += 1
        counts['cases'] += 1
        counts['direct_overlaps'] += direct != 0xffffffff
        counts['fallbacks'] += fallback
        counts['fallback_solid'] += bool(fallback and work[0x8b])
        counts['cull_tests'] += nc; counts['mesh_overlap_tests'] += no
        counts['zero_bounds'] += not any(extents); counts['tiny_bounds'] += any(extents)
        counts['wrapped_generations'] += visit == 2
        counts['distinct_contents_hits'] += direct != 0xffffffff and cull_contents[direct] != mesh_contents[direct]
        if case+1 == BENCHMARK//2:
            half = offset
        if case+1 == BENCHMARK:
            assert payload[32:half] == payload[half:offset], 'LDR/HDR point policy differs'
    assert offset == len(payload)
    assert all(value > 0 for value in checks.values())
    return counts, checks


def source_closure(root):
    sources = {}

    def visit(name):
        if name in sources:
            return
        path = root/name; sources[name] = sha(path)
        for include in re.findall(r'^\s*#include\s*"([^"\r\n]+)"', path.read_text(), re.M):
            visit(include)

    for name in ('probe_stationary_point_policy.cpp', 'build_stationary_point_policy.cmd', 'verify_stationary_point_policy.py'):
        visit(name)
    return sources


def compile_probe(root, directory, stem, run):
    directory.mkdir(parents=True, exist_ok=True)
    build = subprocess.run(['cmd', '/c', str(root/f'build_{stem}.cmd'), str(directory)], capture_output=True, text=True, timeout=180)
    (run/f'{stem}-build.log').write_text(build.stdout+build.stderr)
    assert build.returncode == 0, (stem, build.stdout, build.stderr)
    result = {}
    for arch, machine in (('x86', 0x14c), ('x64', 0x8664)):
        log = (directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log, log
        exe = directory/f'{stem}_{arch}.exe'; raw = exe.read_bytes(); pe = struct.unpack_from('<I', raw, 60)[0]
        assert raw[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H', raw, pe+4)[0] == machine
        assert struct.unpack_from('<H', raw, pe+24)[0] == (0x10b if arch == 'x86' else 0x20b)
        result[arch] = dict(executable=str(exe), sha256=sha(exe), machine=hex(machine), warnings=0)
    return result


def regressions(root, directory, run, table):
    results = []
    for stem, relative in (
            ('stationary_displacement_hull_policy', '20261128-stationary-displacement-policy/verify-06p8dj5x/results.json'),
            ('stationary_hull_world_trace', '20261128-stationary-world-trace/verify-ltm0hdiu/results.json')):
        previous_path = directory.parent/relative; previous = json.loads(previous_path.read_text())
        actual_changes = {name for name, value in previous['source_sha256'].items() if sha(root/name) != value}
        assert actual_changes == CHANGED_SHARED, ('unrelated regression source change', stem, actual_changes)
        builds = compile_probe(root, run/f'{stem}-regression', stem, run)
        for arch in ('x86', 'x64'):
            for repeat in (1, 2):
                output = run/f'{stem}-{arch}-{repeat}.bin'
                args = [builds[arch]['executable'], str(ENGINE) if arch == 'x86' else '-', str(BSP), str(table), str(output)]
                result = subprocess.run(args, capture_output=True, text=True, timeout=180)
                (run/f'{stem}-{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
                assert result.returncode == 0, (stem, arch, repeat, result.stderr)
                assert sha(output) == previous['record_sha256'], ('accepted non-ray behavior changed', stem, arch, repeat)
                assert json.loads(result.stdout)['cases'] == previous['counts']['cases']
        results.append(dict(component=stem, accepted_receipt=str(previous_path), builds=builds,
                            cases=previous['counts']['cases'], repeats_per_arch=2, warnings=0,
                            record_sha256=previous['record_sha256'], changed_source=sorted(actual_changes),
                            accepted_record_hash_preserved=True))
    return results


def main(directory):
    directory.mkdir(parents=True, exist_ok=True); root = Path(__file__).resolve().parent
    assert not directory.is_relative_to(Path('F:/').resolve()), 'original files are read-only'
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    frozen = source_closure(root); run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    builds = compile_probe(root, directory, 'stationary_point_policy', run)
    image, evidence = verify_installed_ranges(ENGINE, run/'stationary-point-reference.local.txt', [
        ('complete stationary displacement dispatcher', 0x1014d320, 0x1014d535),
        ('complete stationary normal selection', 0x1014cd40, 0x1014cdf5),
        ('complete stationary normal bounds', 0x10158e80, 0x10158f4a),
        ('complete stationary ray fallback', 0x1014ce00, 0x1014cf9c),
        ('complete fallback point/hull routing', 0x10149030, 0x10149063),
        ('complete nested visit setup', 0x10147c30, 0x10147c97),
        ('complete nested depth finish', 0x10145510, 0x1014551e),
        ('complete direct displacement overlap including tail', 0x1015a7d0, 0x1015abac),
        ('complete point world geometry', 0x10148950, 0x10148c2c),
        ('complete point leaf policy', 0x10148270, 0x10148537),
    ])
    benchmark = Benchmark(BSP.read_bytes()); baseline = counts = independent = None; observations = []
    for arch in ('x86', 'x64'):
        for repeat in (1, 2):
            output = run/f'{arch}-{repeat}.bin'
            result = subprocess.run([builds[arch]['executable'], str(ENGINE) if arch == 'x86' else '-', str(BSP), str(output)],
                                    capture_output=True, text=True, timeout=180)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode == 0, (arch, repeat, result.stderr)
            actual = json.loads(result.stdout)
            if arch == 'x86':
                observed = {name:actual.pop(name) for name in ('native_route_observations', 'native_point_observations')}
                assert all(value == actual['fallbacks'] for value in observed.values())
                observations.append(observed); assert all(value == observed for value in observations)
            payload = output.read_bytes()
            if baseline is None:
                counts, independent = inspect_records(payload, benchmark); baseline = payload
            assert payload == baseline and actual == counts, (arch, repeat, 'point dispatcher differs', actual, counts)
    # Regression-only private permutation resource, never consumed by point code.
    table = run/'nonray-regression-permutation.local.bin'
    offset = image.offset(0x104129b0); raw = image.b[offset:offset+1024]
    assert hashlib.sha256(raw).hexdigest() == PERMUTATION_SHA256; table.write_bytes(raw)
    regression = regressions(root, directory, run, table)
    point_receipt = directory.parent/'20261128-point-world/verify-1dj7ia0g/results.json'
    point = json.loads(point_receipt.read_text())
    assert all(sha(root/name) == value for name, value in point['source_sha256'].items())
    assert source_closure(root) == frozen, 'source changed during verification'
    editor = json.loads((root.parent/'publication/stage/docs/build-status.json').read_text())
    assert len(editor['editor_source_dependencies']) == 543
    assert all(sha(root.parent/name) == value for name, value in editor['editor_source_dependencies'].items())
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    receipt = dict(status='VERIFIED PREPARED STATIONARY POINT DISPLACEMENT DISPATCH', builds=builds,
                   counts=counts, independent_checks=independent, repeats_per_arch=2, source_sha256=frozen,
                   record_bytes=len(baseline), record_sha256=hashlib.sha256(baseline).hexdigest(),
                   native_observations_per_run=observations, replaced_native_callees=[],
                   original_engine_sha256=ENGINE_SHA256, original_bsp_sha256=BSP_SHA256,
                   originals_unchanged=True, editor_dependencies_unchanged=543,
                   verified_spans=[row['name'] for row in evidence], nonray_regressions=regression,
                   point_component_unchanged=str(point_receipt),
                   findings=[
                       'Original full dispatcher, normal selection, direct overlap and nested fallback execute through actual point BSP/brush/terrain geometry. All560work bytes, parent/child visits and cull/overlap order match; no native callee or storage service is replaced.',
                       'Hardware observations prove every fallback retains isRay=true/nonswept flags, increments/restores original depth, wraps child generation to1, calls original point10148950 and restores prior end coordinates.',
                       'Direct overlap depends on nonzero relative bounds independently of point classification. Normal selection receives the point flag; runtime flags4/8 affect overlap and point ray eligibility differently.',
                       'The shared dispatcher adds only an explicit isRay parameter defaulting false. Fresh old non-ray dispatcher and public stationary trace builds repeat in both architectures and preserve their accepted complete-record hashes.',
                       'Independent visit/cull/overlap/normal predictions, full direct work preservation, fallback ray arithmetic/guards, controlled child brush marks and positive-solid point fallbacks pass.'],
                   limits=[
                       'Prepared single-displacement-list dispatcher with finite centered zero/tiny point-classified hulls. Complete stationary leaf/world/public point orchestration remains separate.',
                       'Native receives owned-imported/prepared BSP, mesh/cull/leaf records and distinguishing material properties; effective original loader/material/physics binding is not executed.',
                       'Point primary test uses no permutation table or cache substitution. Non-ray regression tests retain their explicitly declared private cache/storage adapters and private permutation input.',
                       'Production global allocation/lifetime/concurrency, engine/server entities/static props/filters, real spawn and editor integration remain unverified. All543editor dependencies and package are unchanged.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'], counts=counts, independent_checks=independent, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
