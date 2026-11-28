"""Check prepared public swept point traces against intact private originals.

No original bytes/assets/table data are embedded. Work acquire/release storage
and material property identities are explicit fixtures, not production loading.
"""
import argparse
import ast
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
from verify_stationary_hull_leaf import Benchmark
from verify_stationary_displacement_hull_policy import f32
from verify_stationary_point_policy import compile_probe
from verify_stationary_point_world_trace import default_record

TOTAL, BENCHMARK, CONTROLLED, STRIDE = 53832, 38528, 15232, 176


def inspect_records(payload, benchmark):
    assert struct.unpack_from('<8I', payload) == (0x53575031, TOTAL, BENCHMARK, CONTROLLED, 8, STRIDE, 64, 0)
    assert len(payload) == 32+TOTAL*STRIDE
    middle = 32+BENCHMARK//2*STRIDE
    assert payload[32:middle] == payload[middle:32+BENCHMARK*STRIDE], 'LDR/HDR point trace records differ'
    counts = dict(cases=0, benchmark_cases=BENCHMARK, controlled_cases=CONTROLLED,
                  empty_flag_cases=8, provided_ray_cases=64, hits=0, start_solid=0,
                  all_solid=0, empty_worlds=0, positions_disabled=0, tiny_extents=0,
                  nonzero_offsets=0, rounded_deltas=0, stale_parent_storage=0,
                  zero_masks=0, terrain_names=0, brush_names=0, negative_zero_positions=0, passed=True)
    checks = dict(point_and_swept_flags=0, position_predictions=0,
                  zero_mask_full_trace_predictions=0, empty_world_defaults=0,
                  empty_world_flag_ordering=0, stale_fresh_pairs=0,
                  compute_mode_pairs=0, terrain_material_predictions=0,
                  brush_material_predictions=0, provided_signed_zero_cancellation=0,
                  analytic_terrain_predictions=0, analytic_terrain_hits=0,
                  analytic_one_sided_misses=0, analytic_content_ineligible_misses=0,
                  analytic_flag8_disabled=0, analytic_flag4_hits=0,
                  analytic_horizontal_full_metadata=0, retained_brush_world_index=0,
                  identical_ldr_hdr_queries=BENCHMARK//2)
    blank = bytes(default_record()); phases = [0]*8; stale_off = {}; compute_off = {}
    for case in range(TOTAL):
        begin = 32+case*STRIDE
        source, row = payload[begin:begin+88], payload[begin+88:begin+STRIDE]
        values = struct.unpack_from('<12f', source)
        center, delta, offset, extent = [values[i:i+3] for i in (0, 3, 6, 9)]
        flags, mask, compute, available, stale, phase, index, q, variant, disabled = struct.unpack_from('<10I', source, 48)
        assert phase < 8 and all(v in (0, 1) for v in (compute, available, stale, disabled))
        assert all(math.isfinite(v) for v in values) and all(v >= 0 for v in extent)
        local = phases[phase]; phases[phase] += 1
        if phase == 5:
            assert not available and not stale and flags == local//2 and compute == local % 2
        else:
            length = f32(f32(f32(delta[0]*delta[0])+f32(delta[1]*delta[1]))+f32(delta[2]*delta[2]))
            size = f32(f32(f32(extent[0]*extent[0])+f32(extent[1]*extent[1]))+f32(extent[2]*extent[2]))
            assert flags == int(size < 1e-6) | (int(length != 0.) << 1)
            assert flags == 3
            checks['point_and_swept_flags'] += 1
        if phase == 6:
            expected = [
                ((-0., -0., 0.), (-0., 2., -3.), (-0., 0., -0.)),
                ((100000000., -100000000., 17.), (1., -3., .25), (-100000000., 100000000., -17.)),
                ((-2., 3., -4.), (5., -6., 7.), (.5, -.25, 1.)),
                ((.03125, -.03125, 2.), (.125, .25, -.5), (-.0625, .0625, -2.)),
            ][variant]
            assert source[:48] == struct.pack('<12f', *expected[0], *expected[1], *expected[2],
                                             *((.000732421875, .000244140625, 0.) if variant & 1 else (0., 0., 0.)))
            checks['provided_signed_zero_cancellation'] += 1
        elif phase != 5:
            expected_extent = ((0., 0., 0.), (.000732421875, .000244140625, 0.),
                               (.000244140625, .00048828125, .0001220703125), (0., 0., 0.))[variant]
            assert extent == expected_extent
        fraction = struct.unpack_from('<f', row, 0x2c)[0]
        contents = struct.unpack_from('<I', row, 0x30)[0]
        all_solid, start_solid = row[0x36:0x38]
        left = struct.unpack_from('<f', row, 0x38)[0]
        name = struct.unpack_from('<I', row, 0x3c)[0]
        prop, surface_flags = struct.unpack_from('<2H', row, 0x40)
        disp_flags = struct.unpack_from('<H', row, 0x34)[0]
        world_surface = struct.unpack_from('<H', row, 0x4a)[0]
        marker = struct.unpack_from('<I', row, 0x54)[0]
        assert math.isfinite(fraction) and 0 <= fraction <= 1 and 0 <= left <= 1
        assert all_solid in (0, 1) and start_solid in (0, 1) and marker in (0, 1)
        assert not all_solid or (start_solid and fraction == 0.)
        assert row[0x44:0x4a] == bytes(6) and row[0x4c:0x54] == bytes(8)
        assert world_surface < 87 or world_surface == 65535
        if name == 0xffffffff:
            assert (prop, surface_flags) == (0x3344, 0)
        elif 0x10000000 <= name < 0x10000000+87:
            material = name-0x10000000
            assert (prop, surface_flags) == (1000+material, benchmark.flags[material])
            assert world_surface == material and marker == 0
            checks['brush_material_predictions'] += 1
        else:
            mesh = name-0x20000000
            assert 0 <= mesh < (43 if phase == 0 else 4)
            assert prop == (500 if disp_flags & 16 else 100)+mesh and surface_flags == 0 and marker == 1
            assert contents == (benchmark.disp_contents[mesh] if phase == 0 else (1, 0x80, 0x4000, 0x20)[mesh])
            checks['terrain_material_predictions'] += 1
            checks['retained_brush_world_index'] += world_surface != 0
        if not available:
            assert row == blank, ('empty default', case)
            checks['empty_world_defaults'] += 1
            checks['empty_world_flag_ordering'] += phase == 5
        else:
            if not mask:
                assert row[24:] == blank[24:], ('zero-mask point trace', case)
                checks['zero_mask_full_trace_predictions'] += 1
            if compute:
                base = [f32(offset[a]+center[a]) for a in range(3)]
                end = [f32(delta[a]+base[a]) if fraction == 1. else f32(f32(delta[a]*fraction)+base[a]) for a in range(3)]
                if left == 1.:
                    assert fraction == 0 and all_solid and start_solid
                    end = base
                start = [base[a] if left == 0. else f32(f32(delta[a]*left)+base[a]) for a in range(3)]
                assert row[:24] == struct.pack('<6f', *start, *end), ('coordinate predictions', case)
                checks['position_predictions'] += 1
            else:
                assert row[:24] == bytes(24)
            if phase == 7:
                # No brushes; authored planes z=0, z=24, z=8+.25x-.125y.
                # Mesh3 contents0x20 are rejected by the independent0x4081 gate.
                eligible = [i for i, content in enumerate((1, 0x80, 0x4000)) if content & mask and not (i == 1 and disabled)]
                winner = min(eligible, key=lambda i: (64.-(0., 24., 8.)[i])/128.) if eligible and q == 0 else None
                expected = bytearray(blank)
                if winner is not None:
                    height = (0., 24., 8.)[winner]
                    assert center == (0., 0., 64.) and delta == (0., 0., -128.)
                    hit = f32((64.-height)/128.)
                    assert fraction == hit and contents == (1, 0x80, 0x4000)[winner] and name == 0x20000000+winner
                    assert not all_solid and not start_solid and left == 0 and marker == 1 and world_surface == 0
                    assert disp_flags == (17 if winner == 1 else 9)
                    assert row[0x28:0x2c] == bytes(4)
                    if winner < 2:
                        struct.pack_into('<4f', expected, 0x18, 0., 0., 1., height)
                        struct.pack_into('<f', expected, 0x2c, hit)
                        struct.pack_into('<I', expected, 0x30, (1, 0x80)[winner])
                        struct.pack_into('<H', expected, 0x34, (9, 17)[winner])
                        struct.pack_into('<I', expected, 0x3c, 0x20000000+winner)
                        struct.pack_into('<H', expected, 0x40, (100, 501)[winner])
                        struct.pack_into('<I', expected, 0x54, 1)
                        assert row[24:] == expected[24:], ('analytic horizontal metadata', case)
                        checks['analytic_horizontal_full_metadata'] += 1
                    else:
                        normal = struct.unpack_from('<3f', row, 0x18)
                        scale = 1/math.sqrt(1+.25**2+.125**2)
                        assert all(math.isclose(a, b, abs_tol=1e-7) for a, b in zip(normal, (-.25*scale, .125*scale, scale)))
                        assert math.isclose(struct.unpack_from('<f', row, 0x24)[0], 8*scale, abs_tol=1e-6)
                        checks['analytic_flag4_hits'] += 1
                    checks['analytic_terrain_hits'] += 1
                else:
                    assert row[24:] == blank[24:], ('analytic terrain miss', case)
                    checks['analytic_one_sided_misses'] += q == 1
                    checks['analytic_content_ineligible_misses'] += mask == 0x20
                    checks['analytic_flag8_disabled'] += q == 0 and mask == 0x80 and disabled
                checks['analytic_terrain_predictions'] += 1
        # Compare options using input identity, including controlled runtime flags.
        if phase != 5:
            stale_key = source[:64]+source[68:]
            if stale:
                assert stale_off[stale_key] == row
                checks['stale_fresh_pairs'] += 1
            else:
                stale_off[stale_key] = row
        compute_key = source[:56]+source[60:]
        if compute:
            assert compute_off[compute_key] == row[24:], ('compute option changed collision', case)
            checks['compute_mode_pairs'] += 1
        else:
            compute_off[compute_key] = row[24:]
        counts['cases'] += 1
        counts['hits'] += fraction < 1
        counts['start_solid'] += start_solid
        counts['all_solid'] += all_solid
        counts['empty_worlds'] += not available
        counts['positions_disabled'] += not compute
        counts['tiny_extents'] += any(extent)
        counts['nonzero_offsets'] += any(offset)
        counts['rounded_deltas'] += any(f32(f32(center[a]+delta[a])-center[a]) != delta[a] for a in range(3))
        counts['stale_parent_storage'] += stale
        counts['zero_masks'] += not mask
        counts['terrain_names'] += 0x20000000 <= name < 0x20000000+(43 if phase == 0 else 4)
        counts['brush_names'] += 0x10000000 <= name < 0x10000000+87
        counts['negative_zero_positions'] += any(struct.unpack_from('<I', row, 4*a)[0] == 0x80000000 for a in range(6))
    assert phases == [38528, 3520, 3520, 3520, 3520, 8, 64, 1152]
    assert all(v > 0 for v in checks.values()), checks
    return counts, checks


def source_closure(root):
    sources = {}

    def visit(name):
        if name in sources:
            return
        path = root/name
        sources[name] = sha(path)
        text = path.read_text(encoding='utf-8')
        for include in re.findall(r'^\s*#include\s*"([^"\r\n]+)"', text, re.M):
            visit(include)
        if path.suffix == '.py':
            for node in ast.walk(ast.parse(text)):
                modules = ([node.module] if isinstance(node, ast.ImportFrom) else
                           [a.name for a in node.names] if isinstance(node, ast.Import) else [])
                for module in modules:
                    local = (module or '').replace('.', '/')+'.py'
                    if (root/local).is_file():
                        visit(local)

    for name in ('probe_swept_point_world_trace.cpp', 'build_swept_point_world_trace.cmd',
                 'verify_swept_point_world_trace.py', 'probe_stationary_point_world_trace.cpp',
                 'build_stationary_point_world_trace.cmd'):
        visit(name)
    return sources


def accepted_components(root, directory):
    result = []
    for relative in ('20261128-point-world/verify-1dj7ia0g/results.json',
                     '20261128-stationary-point-policy/verify-t5qdkvzf/results.json',
                     '20261128-stationary-point-world/verify-9w82n_7j/results.json',
                     '20261128-stationary-point-world-trace/verify-e2ae4vc_/results.json'):
        path = directory.parent/relative; previous = json.loads(path.read_text())
        assert all(sha(root/name) == value for name, value in previous['source_sha256'].items()), ('accepted source changed', relative)
        for build in previous['builds'].values():
            assert sha(Path(build['executable'])) == build['sha256']
        result.append(dict(receipt=str(path), record_sha256=previous['record_sha256'], source_and_binary_hashes_unchanged=True))
    return result


def stationary_regression(root, directory, run):
    path = directory.parent/'20261128-stationary-point-world-trace/verify-e2ae4vc_/results.json'
    previous = json.loads(path.read_text())
    builds = compile_probe(root, run/'stationary-point-regression', 'stationary_point_world_trace', run)
    for arch in ('x86', 'x64'):
        for repeat in (1, 2):
            output = run/f'stationary-point-{arch}-{repeat}.bin'
            process = subprocess.run([builds[arch]['executable'], str(ENGINE) if arch == 'x86' else '-', str(BSP), str(output)],
                                     capture_output=True, text=True, timeout=240)
            (run/f'stationary-point-{arch}-{repeat}.log').write_text(process.stdout+process.stderr)
            assert process.returncode == 0, (arch, repeat, process.stderr)
            assert sha(output) == previous['record_sha256'], ('stationary point public trace changed', arch, repeat)
            assert json.loads(process.stdout)['cases'] == 133192
    return dict(accepted_receipt=str(path), builds=builds, cases=133192, repeats_per_arch=2,
                record_sha256=previous['record_sha256'], accepted_record_hash_preserved=True, warnings=0)


def main(directory):
    assert directory.is_relative_to(Path('D:/SFM64-development-tests').resolve()), 'use the dedicated disposable test directory'
    directory.mkdir(parents=True, exist_ok=True); root = Path(__file__).resolve().parent
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    frozen = source_closure(root)
    components = accepted_components(root, directory)
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    builds = compile_probe(root, directory, 'swept_point_world_trace', run)
    _, evidence = verify_installed_ranges(ENGINE, run/'public-swept-point-reference.local.txt', [
        ('complete public world trace wrapper', 0x10149eb0, 0x1014a12c),
        ('complete original coordinate finisher', 0x10145e20, 0x10145f33),
        ('complete base trace copy', 0x10146de0, 0x10146e63),
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
    benchmark = Benchmark(BSP.read_bytes())
    baseline = counts = checks = None; native_counts = []
    for arch in ('x86', 'x64'):
        for repeat in (1, 2):
            output = run/f'{arch}-{repeat}.bin'
            process = subprocess.run([builds[arch]['executable'], str(ENGINE) if arch == 'x86' else '-', str(BSP), str(output)],
                                     capture_output=True, text=True, timeout=240)
            (run/f'{arch}-{repeat}.log').write_text(process.stdout+process.stderr)
            assert process.returncode == 0, (arch, repeat, process.stderr)
            actual = json.loads(process.stdout)
            if arch == 'x86':
                observed = {name:actual.pop(name) for name in ('native_setup_observations', 'native_leaf_observations', 'native_finish_observations')}
                assert observed['native_setup_observations'] == actual['cases']-actual['empty_worlds']
                assert observed['native_finish_observations'] == observed['native_setup_observations']//2
                assert observed['native_leaf_observations'] >= observed['native_setup_observations']
                native_counts.append(observed); assert all(value == observed for value in native_counts)
            payload = output.read_bytes()
            if baseline is None:
                counts, checks = inspect_records(payload, benchmark); baseline = payload
            assert actual == counts and payload == baseline, (arch, repeat, 'public swept point records differ', actual, counts)
    regression = stationary_regression(root, directory, run)
    assert accepted_components(root, directory) == components
    assert source_closure(root) == frozen, 'source changed during verification'
    editor = json.loads((root.parent/'publication/stage/docs/build-status.json').read_text())
    assert len(editor['editor_source_dependencies']) == 543
    assert all(sha(root.parent/name) == value for name, value in editor['editor_source_dependencies'].items())
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    receipt = dict(status='VERIFIED PREPARED PUBLIC SWEPT POINT TRACE', counts=counts, independent_checks=checks,
                   builds=builds, repeats_per_arch=2, source_sha256=frozen, record_bytes=len(baseline),
                   record_sha256=hashlib.sha256(baseline).hexdigest(), native_observations_per_run=native_counts,
                   original_engine_sha256=ENGINE_SHA256, original_bsp_sha256=BSP_SHA256, originals_unchanged=True,
                   editor_dependencies_unchanged=543, verified_spans=[row['name'] for row in evidence],
                   accepted_point_components=components, stationary_public_regression=regression,
                   replaced_native_services=['public work acquire10149230/release1000e020 only'],
                   findings=[
                       'Separate swept point adapter reuses the existing full public result and coordinate finisher; existing stationary and swept hull source files are unchanged.',
                       'Complete original10149eb0 executes reset/default setup, actual point world geometry, optional original coordinate finishing and full84-byte result copy. Original executable sections remain unchanged.',
                       'Hardware observations verify every setup field bit, reset trace, mask, point/swept flags, direct caller/root/interval/start/end arguments, actual leaf execution and original finishing. Input/geometry/work/output guards and paired pool-service boundaries pass.',
                       'All84public trace bytes plus the separately observed internal displacement marker match. Surface names normalize through distinguishing material identities. Point primary and stationary point regression require no private hull cache/permutation input.',
                       'Independent coordinates, zero-mask/defaults, material/contents, retained brush world index, stale/fresh storage and compute-mode pairs, signed zero and cancellation checks pass. Brush-free analytic planes cover positive hits, one-sided misses, contents0x20 ineligibility and different runtime flag4/flag8 behavior.',
                       'Fresh stationary point public regression repeats both architectures twice with accepted complete-record hash unchanged. Earlier point source closures and binaries remain unchanged.'],
                   limits=[
                       'Prepared finite swept point inputs only for available-world primary queries; other flag combinations are tested only at the early empty-world return.',
                       'Original acquire/release, material property IDs, geometry storage bindings and parent generation setup are explicit fixtures. Production pool allocation/generation lifetime/concurrency and effective material/physics binding are not verified.',
                       'Stale storage comparisons use inactive generation17 with current generation1; unused child state is preserved. This does not execute original global pool reuse or rollover.',
                       'The internal marker is separately observed and is not part of the84-byte public trace. Its empty-world fixture initial value is0.',
                       'This does not establish every float domain/control mode, malformed-input behavior, engine/server entities/static props/filters, actual spawn or editor acceptance. All543editor dependencies remain unchanged.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'], counts=counts, independent_checks=checks, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
