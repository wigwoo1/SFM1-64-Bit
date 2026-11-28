"""Verify complete prepared point-world geometry, metadata and visit behavior.

Original engine/map bytes remain private external inputs. Native functions are
observed through hardware breakpoints; no geometry or storage callee is replaced.
This does not claim public Ray_t dispatch, production loading or editor parity.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import subprocess
import tempfile

from verify_sliding_door_spawn import ENGINE, BSP, sha, verify_installed_ranges
from verify_benchmark_world_hull import ENGINE_SHA256, BSP_SHA256
from verify_stationary_hull_leaf import Benchmark, initial_trace, visit_bits
from verify_stationary_displacement_hull_policy import f32

TOTAL = 109248
BENCHMARK_CASES = 66048
CONTROLLED_CASES = 43200
CONTROL_BRUSHES = (
    ((0, 1, 0, 2, 3, 4, 5, 6, 7), (7, 6, 5, 4, 3, 2, 1, 0, 7),
     (0, 2, 4, 6, 0, 1, 3, 5, 7), (1, 1, 3, 3, 5, 5, 7, 0, 2, 4, 6)),
    ((), (), (1, 1), (1,)),
    ((1, 1), (1,), (), ()),
    ((0, 1, 2, 3, 4, 5, 6, 7, 0, 1),),
)
CONTROL_DISPLACEMENTS = (
    ((0, 1, 0, 2, 3), (3, 2, 1, 0, 1), (0, 1, 2, 3, 0), (1, 0, 1, 2, 3)),
    ((1, 0, 1), (1, 0), (2, 3), (3, 2)),
    ((2, 3), (3, 2), (1, 0, 1), (1, 0)),
    ((0, 1, 0, 2, 3, 1),),
)


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def marked(count, index, mode, displacement=False):
    if not mode:
        return set()
    if displacement:
        return {(index + (mode == 2)) % count}
    return {index % count} if mode == 1 else {i for i in range(count) if i % 3 == index % 3}


def read_marks(data, count):
    result = {i for i in range(count) if data[i//8] & (1 << (i % 8))}
    assert visit_bits(result, count) == data, 'nonzero visit-table padding'
    return result


def benchmark_exclusions(data, benchmark):
    planes, count = struct.unpack_from('<2I', data, 8+18*16)
    sides, _ = struct.unpack_from('<2I', data, 8+19*16)
    textures, _ = struct.unpack_from('<2I', data, 8+6*16)
    result = []
    for i in range(count//12):
        first, length = struct.unpack_from('<2I', data, planes+12*i)
        excluded = False
        for j in range(length):
            texture = struct.unpack_from('<h', data, sides+8*(first+j)+2)[0]
            if texture >= 0:
                material = struct.unpack_from('<I', data, textures+72*texture+68)[0]
                excluded |= bool(benchmark.flags[material] & 0x80)
        result.append(excluded)
    assert len(result) == 1866 and any(result) and not all(result)
    return result


def inspect_records(payload, benchmark, bsp):
    assert struct.unpack_from('<8I', payload) == (0x50574d31, BENCHMARK_CASES, CONTROLLED_CASES, 560, 1866, 43, 8, 4)
    exclusions = benchmark_exclusions(bsp, benchmark)
    counts = dict(cases=0, benchmark_cases=BENCHMARK_CASES, controlled_cases=CONTROLLED_CASES,
                  changed_fraction=0, brush_winners=0, displacement_winners=0, start_solid=0,
                  marker_cleared=0, rounded_delta_cases=0, zero_masks=0, nonswept_visit_queries=0,
                  tiny_extents=0, premarked_queries=0, leaf_calls=0, brush_calls=0,
                  displacement_calls=0, events_disabled_checks=(TOTAL+30)//31, passed=True)
    independent = dict(full_brush_policy_replays=0, zero_fraction_full_preservation=0,
                       nonswept_visit_preservation=0, zero_mask_metadata_preservation=0,
                       terrain_material_predictions=0, brush_material_predictions=0,
                       tiny_point_classification=0, all_query_input_and_guard_checks=0,
                       controlled_terrain_reference_checks=0, retained_plane_tag_checks=0,
                       identical_ldr_hdr_queries=BENCHMARK_CASES//2)
    offset = 32
    half_boundary = None
    phase_counts = [0]*5
    max_leaf = len(benchmark.brushes)
    for case in range(TOTAL):
        row = struct.unpack_from('<16I', payload, offset)
        start, delta, extent = [struct.unpack('<3f', struct.pack('<3I', *row[i:i+3])) for i in (0, 3, 6)]
        mask, mode, swept, visit, phase, index, root = row[9:]
        assert phase <= 4 and (phase == 0) == (case < BENCHMARK_CASES)
        assert swept in (0, 1) and visit in (0, 1, 2)
        assert root == (0xffffffff if phase == 4 else 0)
        phase_counts[phase] += 1
        brush_count, disp_count = (1866, 43) if not phase else (8, 4)
        end = tuple(f32(start[a]+delta[a]) for a in range(3))
        inverse = tuple(f32(1./d) if d else struct.unpack('<f', bytes.fromhex('ffff7f7f'))[0] for d in delta)
        offset += 64
        lc, bc, dc = struct.unpack_from('<3I', payload, offset)
        offset += 12
        leaves = struct.unpack_from('<'+'I'*lc, payload, offset); offset += 4*lc
        brushes = struct.unpack_from('<'+'I'*bc, payload, offset); offset += 4*bc
        displacements = struct.unpack_from('<'+'I'*dc, payload, offset); offset += 4*dc
        intervals = [struct.unpack_from('<2f', payload, offset+8*i) for i in range(lc)]
        offset += 8*lc
        work = payload[offset:offset+560]; offset += 560
        brush_bits = payload[offset:offset+(brush_count+7)//8]; offset += len(brush_bits)
        disp_bits = payload[offset:offset+(disp_count+7)//8]; offset += len(disp_bits)
        brush_marks = read_marks(brush_bits, brush_count)
        disp_marks = read_marks(disp_bits, disp_count)
        original_brush_marks = marked(brush_count, index, visit)
        original_disp_marks = marked(disp_count, index, visit, True)
        assert original_brush_marks <= brush_marks and original_disp_marks <= disp_marks
        assert all(0 <= a <= b <= 1 for a, b in intervals)
        assert all(i < (max_leaf if not phase else len(CONTROL_BRUSHES[phase-1])) for i in leaves)
        assert all(i < brush_count for i in brushes) and all(i < disp_count for i in displacements)
        assert len(brushes) == len(set(brushes)) and not set(brushes) & original_brush_marks
        assert set(brushes) <= brush_marks
        contents = benchmark.contents if not phase else (1, 0x80, 0x81, 0x2080, 0x80, 1, 0x4000, 0x80)
        disp_contents = benchmark.disp_contents if not phase else (1, 0x80, 0x4000, 0x20)
        excluded = exclusions if not phase else (True, False, True, False, True, False, True, False)
        assert all(contents[i] & mask for i in brushes)
        assert not any((contents[i]&mask) == 0x80 and mask&0x2000 and excluded[i] for i in brushes)
        assert all(disp_contents[i] & mask for i in displacements)
        assert all(disp_contents[i] & mask for i in disp_marks-original_disp_marks)
        if swept:
            assert len(displacements) == len(set(displacements))
            assert not set(displacements) & original_disp_marks
            assert set(displacements) <= disp_marks
        else:
            assert disp_marks == original_disp_marks
            independent['nonswept_visit_preservation'] += 1
        if phase:
            candidates = {i for leaf in leaves for i in CONTROL_DISPLACEMENTS[phase-1][leaf]}
            assert set(displacements) <= candidates
            assert disp_marks-original_disp_marks <= candidates
            independent['controlled_terrain_reference_checks'] += 1
        fraction = struct.unpack_from('<f', work, 0x80)[0]
        assert math.isfinite(fraction) and 0 <= fraction <= 1
        # A positive final fraction means no brush loop terminated on zero.
        # Replay contents/exclusion/dedup without calling any geometry helper.
        if fraction > 0:
            expected_marks = set(original_brush_marks)
            expected_calls = []
            for leaf in leaves:
                ordered = benchmark.brushes[leaf] if not phase else CONTROL_BRUSHES[phase-1][leaf]
                for brush in ordered:
                    if brush in expected_marks:
                        continue
                    expected_marks.add(brush)
                    overlap = contents[brush] & mask
                    if overlap and not (overlap == 0x80 and mask&0x2000 and excluded[brush]):
                        expected_calls.append(brush)
            assert expected_marks == brush_marks and tuple(expected_calls) == brushes, ('brush policy', case)
            independent['full_brush_policy_replays'] += 1
        seed = bytearray(b'\xa7'*560)
        for at, values in ((0, start), (12, end), (0x18, tuple(-v for v in extent)),
                           (0x24, extent), (0x30, extent), (0x3c, delta), (0x48, inverse)):
            struct.pack_into('<3f', seed, at, *values)
        seed[0x54:0xa8] = initial_trace(mode)
        struct.pack_into('<I', seed, 0xfc, mask)
        seed[0x100:0x102] = bytes((1, swept))
        for at, value in ((0x104, 0), (0x114, 0xffffffff if mode else 0), (0x11c, 0),
                          (0x120, 17), (0x124, 0), (0x128, 0), (0x13c, 0),
                          (0x148, brush_count), (0x150, 0), (0x164, 0), (0x170, disp_count)):
            struct.pack_into('<I', seed, at, value)
        allowed = set(range(0x6c, 0x98)) | set(range(0x9e, 0xa0)) | set(range(0x114, 0x118))
        assert all(a == b for i, (a, b) in enumerate(zip(seed, work)) if i not in allowed), ('work guards', case)
        assert sum(float(v)*v for v in extent) < 1e-6, 'input is not a point Ray_t hull'
        if any(extent):
            independent['tiny_point_classification'] += 1
        original_fraction = (1., .25, 0.)[mode % 3]
        if not original_fraction:
            assert work == seed and not (lc or bc or dc)
            assert brush_marks == original_brush_marks and disp_marks == original_disp_marks
            independent['zero_fraction_full_preservation'] += 1
        name = struct.unpack_from('<I', work, 0x90)[0]
        marker = struct.unpack_from('<I', work, 0x114)[0]
        assert marker in (0, 1, 0xffffffff)
        property_id, flags = struct.unpack_from('<2H', work, 0x94)
        surface_index = struct.unpack_from('<H', work, 0x9e)[0]
        assert surface_index < 87 or surface_index == 65535
        if 0x20000000 <= name < 0x20000000+disp_count:
            mesh = name-0x20000000
            disp_flags = struct.unpack_from('<H', work, 0x88)[0]
            assert property_id == (500 if disp_flags & 16 else 100)+mesh and flags == 0 and marker == 1
            assert struct.unpack_from('<I', work, 0x84)[0] == disp_contents[mesh]
            # Terrain updates normal/distance but deliberately leaves plane tags.
            # Without a preceding entering brush, these are the seeded tag bytes.
            if not brushes:
                assert work[0x7c:0x80] == seed[0x7c:0x80]
                independent['retained_plane_tag_checks'] += 1
            independent['terrain_material_predictions'] += 1
        elif name == 0xffffffff:
            assert (property_id, flags) == (0x3344, 0)
        else:
            assert 0x10000000 <= name < 0x10000000+87
            material = name-0x10000000
            if (property_id, flags) != (0xabcd, 0xef01):
                assert property_id == 1000+material and flags == benchmark.flags[material]
                independent['brush_material_predictions'] += 1
        if mask == 0:
            assert not (bc or dc)
            for a, b in ((0x6c, 0x8a), (0x8c, 0xa8), (0x114, 0x118)):
                assert work[a:b] == seed[a:b]
            independent['zero_mask_metadata_preservation'] += 1
        changed = fraction != original_fraction
        counts['changed_fraction'] += changed
        counts['brush_winners'] += changed and 0x10000000 <= name < 0x10000000+87
        counts['displacement_winners'] += changed and 0x20000000 <= name < 0x20000000+disp_count
        counts['start_solid'] += work[0x8b] != 0
        counts['marker_cleared'] += bool(mode) and marker == 0
        counts['rounded_delta_cases'] += any(f32(end[a]-start[a]) != delta[a] for a in range(3))
        counts['zero_masks'] += mask == 0
        counts['nonswept_visit_queries'] += not swept
        counts['tiny_extents'] += any(extent)
        counts['premarked_queries'] += visit != 0
        counts['leaf_calls'] += lc; counts['brush_calls'] += bc; counts['displacement_calls'] += dc
        counts['cases'] += 1
        independent['all_query_input_and_guard_checks'] += 1
        if case+1 == BENCHMARK_CASES//2:
            half_boundary = offset
        if case+1 == BENCHMARK_CASES:
            assert payload[32:half_boundary] == payload[half_boundary:offset], 'LDR/HDR records differ'
    assert offset == len(payload), (offset, len(payload))
    assert phase_counts == [BENCHMARK_CASES]+[CONTROLLED_CASES//4]*4
    assert all(value > 0 for value in independent.values())
    return counts, independent


def source_closure(root):
    sources = {}

    def visit(name):
        if name in sources:
            return
        path = root/name; sources[name] = sha(path)
        for include in re.findall(r'^\s*#include\s*"([^"\r\n]+)"', path.read_text(), re.M):
            visit(include)

    for name in ('probe_prepared_point_world.cpp', 'build_prepared_point_world.cmd', 'verify_prepared_point_world.py'):
        visit(name)
    return sources


def main(directory):
    directory.mkdir(parents=True, exist_ok=True)
    root = Path(__file__).resolve().parent
    frozen_sources = source_closure(root)
    assert not directory.is_relative_to(Path('F:/').resolve()), 'original installation is read-only'
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    build = subprocess.run(['cmd', '/c', str(root/'build_prepared_point_world.cmd'), str(directory)],
                           capture_output=True, text=True, timeout=120)
    (run/'build.log').write_text(build.stdout+build.stderr)
    assert build.returncode == 0, (build.returncode, build.stdout, build.stderr)
    _, evidence = verify_installed_ranges(ENGINE, run/'point-world-reference.local.txt', [
        ('complete point BSP world', 0x10148950, 0x10148c2c),
        ('complete point leaf', 0x10148270, 0x10148537),
        ('complete brush point dispatch', 0x10147690, 0x1014791f),
        ('complete optimized relative slab primitive', 0x10146850, 0x10146c24),
        ('complete outer terrain point adapter', 0x1014d010, 0x1014d0f8),
        ('complete terrain ray eligibility and result', 0x1015b520, 0x1015b5a3),
        ('complete terrain point tree', 0x1015a130, 0x1015a565),
        ('complete leaf terrain finish', 0x1014cfa0, 0x1014cfe9),
        ('complete leaf terrain bounds gate', 0x1014e790, 0x1014e899),
    ])
    bsp = BSP.read_bytes(); benchmark = Benchmark(bsp)
    builds = {}; baseline = counts = independent = None; native_counts = []
    for arch, machine in (('x86', 0x14c), ('x64', 0x8664)):
        exe = directory/f'prepared_point_world_{arch}.exe'
        log = (directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log, log
        raw = exe.read_bytes(); pe = struct.unpack_from('<I', raw, 60)[0]
        assert raw[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H', raw, pe+4)[0] == machine
        assert struct.unpack_from('<H', raw, pe+24)[0] == (0x10b if arch == 'x86' else 0x20b)
        builds[arch] = dict(executable=str(exe), sha256=sha(exe), machine=hex(machine), warnings=0)
        for repeat in (1, 2):
            record = run/f'{arch}-{repeat}.bin'
            result = subprocess.run([str(exe), str(ENGINE) if arch == 'x86' else '-', str(BSP), str(record)],
                                    capture_output=True, text=True, timeout=180)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode == 0, (arch, repeat, result.returncode, result.stderr)
            actual = json.loads(result.stdout)
            if arch == 'x86':
                observed = {key:actual.pop(key) for key in ('native_brush_then_displacement', 'native_displacement_then_brush', 'native_solid_exit_resets')}
                assert all(value > 0 for value in observed.values())
                native_counts.append(observed); assert all(value == observed for value in native_counts)
            payload = record.read_bytes()
            if baseline is None:
                counts, independent = inspect_records(payload, benchmark, bsp)
                baseline = payload
            assert actual == counts and payload == baseline, (arch, repeat, 'point records differ', actual, counts)
    sources = source_closure(root)
    assert sources == frozen_sources, 'source changed while the verification was running'
    accepted = []
    for relative in ('20261128-stationary-leaf/verify-hlovln0i/results.json',
                     '20261128-stationary-world/verify-nug5kaze/results.json',
                     '20261128-stationary-world-trace/verify-ltm0hdiu/results.json'):
        path = directory.parent/relative; previous = json.loads(path.read_text())
        assert previous['repeats_per_arch'] == 2
        for name, value in previous['source_sha256'].items():
            assert sha(root/name) == value, ('accepted source changed', name)
        for value in previous['builds'].values():
            assert sha(Path(value['executable'])) == value['sha256']
        accepted.append(dict(receipt=str(path), record_sha256=previous['record_sha256'], source_and_binary_hashes_unchanged=True))
    editor = json.loads((root.parent/'publication/stage/docs/build-status.json').read_text())
    assert len(editor['editor_source_dependencies']) == 543
    assert all(sha(root.parent/name) == value for name, value in editor['editor_source_dependencies'].items())
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    receipt = dict(status='VERIFIED PREPARED POINT WORLD GEOMETRY', counts=counts, independent_checks=independent,
                   builds=builds, repeats_per_arch=2, source_sha256=sources,
                   record_bytes=len(baseline), record_sha256=hashlib.sha256(baseline).hexdigest(),
                   original_engine_sha256=ENGINE_SHA256, original_bsp_sha256=BSP_SHA256,
                   originals_unchanged=True, editor_dependencies_unchanged=543,
                   native_observations_per_run=native_counts, replaced_native_callees=[],
                   verified_spans=[row['name'] for row in evidence], accepted_components=accepted,
                   findings=[
                       'Complete original point walker, leaf brush/terrain policy and actual geometry execute through unchanged instructions. Four hardware breakpoint slots observe leaf intervals, brush contacts, terrain contacts and cull returns; no callee is replaced.',
                       'Full560work bytes, full parent visit vectors and exact leaf/brush/terrain order plus interval bits match before serialization. Surface-name addresses normalize through distinct fixture material properties.',
                       'Point BSP traversal expands axial planes by tiny work extents but uses zero expansion for oblique planes. Optimized brush slabs expand by extents regardless of isRay, while preserving the point solid-exit rule; general brushes use the point plane kernel.',
                       'The point fixture originally bound node count incorrectly. Original tree1015a130 reads object98; the corrected prepared node count and intact original tree yield full equality.',
                       'Both brush-to-terrain and terrain-to-brush winner changes, solid-exit resets, runtime flags, contents/surface exclusions, point-plane tags, retained delta and nonswept visit behavior are exercised.',
                       'Independent brush policy replay, input/guard preservation, zero-fraction pruning, material metadata, controlled terrain membership, nonswept visits and identical LDR/HDR blocks pass.'],
                   limits=[
                       'Prepared point geometry only. Public setup/finish, stationary isRay=true normal/direct-overlap/fallback dispatch and integration with full traces remain separate gates.',
                       'Finite point-classified extents with finite initial inverse components; overflow/underflow public setup and malformed data/topology are not established by these fixtures.',
                       'Native query receives owned-imported/prepared BSP and terrain records. Full original loader and effective material/physics binding are not executed; property IDs are distinguishing fixtures.',
                       'Parent visits are supplied with generation17. Original global work/cache lifecycle, engine/server entities/static props/filters, real spawn and editor integration remain unverified.',
                       'No original instruction/table bytes or assets are embedded, and no permutation table is required by this point kernel. All543editor dependencies and previous accepted stationary components remain unchanged.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'], counts=counts, independent_checks=independent, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
