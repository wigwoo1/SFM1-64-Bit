"""Verify full public stationary hull traces and retained underflow deltas.

Original engine/map/table data remains external private test input. Compare all
84 public trace bytes plus the separately identified internal displacement flag.
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
from verify_benchmark_world_hull import ENGINE_SHA256, BSP_SHA256, PERMUTATION_SHA256
from verify_stationary_hull_leaf import Benchmark
from verify_stationary_hull_world import select_leaves, contents_for
from verify_stationary_displacement_hull_policy import f32

STRIDE = 160


def default_record():
    value = bytearray(88)
    struct.pack_into('<f', value, 0x2c, 1.)
    struct.pack_into('<I', value, 0x3c, 0xffffffff)
    struct.pack_into('<H', value, 0x40, 0x3344)
    return value


def inspect_records(payload, benchmark, bsp):
    assert struct.unpack_from('<8I', payload) == (0x53544831, 133192, 88064, 45056, 8, 160, 64, 0)
    assert len(payload) == 32 + 133192*STRIDE
    midpoint = 32 + 44032*STRIDE
    assert payload[32:midpoint] == payload[midpoint:32+88064*STRIDE], 'LDR/HDR public stationary records differ'
    leaf_offset, leaf_size = struct.unpack_from('<2I', bsp, 8+10*16)
    benchmark_contents = [struct.unpack_from('<I', bsp, leaf_offset+32*i)[0] for i in range(leaf_size//32)]
    counts = dict(cases=0, benchmark_cases=88064, controlled_cases=45056, empty_flag_cases=8,
                  provided_ray_cases=64, negative_zero_positions=0, hits=0, start_solid=0, all_solid=0,
                  empty_worlds=0, positions_disabled=0, nonzero_stationary_delta=0, nonzero_offsets=0,
                  overflowed_setup_inverse=0, wrapped_child_generation=0, zero_masks=0, passed=True)
    independent = dict(ray_flag_predictions=0, position_predictions=0, zero_mask_full_trace_predictions=0,
                       empty_world_defaults=0, empty_world_flag_ordering=0, wrapped_fresh_pairs=0,
                       compute_mode_metadata_pairs=0, retained_delta_solid_positions=0,
                       provided_signed_zero_cancellation_rays=0, identical_ldr_hdr_queries=44032)
    phase_counts = [0]*5
    previous = None
    compute_off = {}
    blank = default_record()
    maximum = struct.unpack('<f', bytes.fromhex('ffff7f7f'))[0]
    for case in range(133192):
        begin = 32+case*STRIDE
        source = payload[begin:begin+72]
        row = payload[begin+72:begin+STRIDE]
        values = struct.unpack_from('<12f', source)
        center, delta, offset, extent = [values[i:i+3] for i in (0, 3, 6, 9)]
        flags, mask, compute, available, wrap, phase = struct.unpack_from('<6I', source, 48)
        assert compute in (0, 1) and available in (0, 1) and wrap in (0, 1) and phase < 5
        assert all(math.isfinite(v) for v in values) and all(v >= 0 for v in extent)
        local_case = phase_counts[phase]
        phase_counts[phase] += 1
        solidity = local_case//5632 if phase in (1, 2) else 3
        if phase == 3:
            assert not available and not wrap and mask == 0xffffffff
            assert flags == local_case//2 and compute == local_case % 2
        else:
            length = f32(f32(f32(delta[0]*delta[0])+f32(delta[1]*delta[1]))+f32(delta[2]*delta[2]))
            size = f32(f32(f32(extent[0]*extent[0])+f32(extent[1]*extent[1]))+f32(extent[2]*extent[2]))
            assert flags == int(size < 1e-6) | (int(length != 0.) << 1)
            assert flags == 0
            independent['ray_flag_predictions'] += 1
        if phase == 4:
            variant = local_case//16
            expected = [
                ((-0., -0., 0.), (-0., 0., -0.), (-0., 0., -0.)),
                ((100000000., -100000000., 17.), (-1e-30, 0., 1e-40), (-100000000., 100000000., -17.)),
                ((-2., 3., -4.), (-0., 0., -0.), (.5, -.25, 1.)),
                ((.03125, -.03125, 2.), (0., 1e-30, 0.), (-.0625, .0625, -2.)),
            ][variant]
            assert source[:48] == struct.pack('<12f', *expected[0], *expected[1], *expected[2], 2., 3., 5.)
            independent['provided_signed_zero_cancellation_rays'] += 1
        fraction = struct.unpack_from('<f', row, 0x2c)[0]
        contents = struct.unpack_from('<I', row, 0x30)[0]
        all_solid, start_solid = row[0x36:0x38]
        left = struct.unpack_from('<f', row, 0x38)[0]
        name = struct.unpack_from('<I', row, 0x3c)[0]
        prop, surface_flags = struct.unpack_from('<2H', row, 0x40)
        world_surface = struct.unpack_from('<H', row, 0x4a)[0]
        marker = struct.unpack_from('<I', row, 0x54)[0]
        assert fraction in (0., 1.) and left in (0., 1.)
        assert all_solid == start_solid == int(fraction == 0.) and marker in (0, 1)
        assert row[0x44:0x4a] == bytes(6) and row[0x4c:0x54] == bytes(8)
        assert world_surface < 87 or world_surface == 65535
        if name == 0xffffffff:
            assert (prop, surface_flags) == (0x3344, 0)
        elif 0x10000000 <= name < 0x10000000+87:
            material = name-0x10000000
            assert (prop, surface_flags) == (1000+material, benchmark.flags[material])
        else:
            mesh = name-0x20000000
            assert 0 <= mesh < (43 if phase == 0 else 2)
            assert prop in (100+mesh, 500+mesh) and surface_flags == 0
        if not available:
            assert row == blank, ('empty world default', case)
            independent['empty_world_defaults'] += 1
            independent['empty_world_flag_ordering'] += phase == 3
        else:
            if mask == 0:
                extents = tuple(f32(v+1.) for v in extent)
                if phase == 0:
                    _, leaves = select_leaves(benchmark.planes, benchmark.nodes, 0, center, extents)
                    non_solid = any(not benchmark_contents[i] & 1 for i in leaves)
                else:
                    nx, ny = (1., 0.) if phase == 1 else (f32(.6), f32(.8))
                    distance = f32(f32(f32(center[1]*ny)+f32(center[0]*nx))+f32(center[2]*0.))
                    radius = f32(extents[1]*ny+extents[0]*nx)
                    leaves = (0,) if distance >= radius else (1,) if distance < -radius else (1, 0)
                    leaf_contents = contents_for(2, solidity)
                    non_solid = any(not leaf_contents[i] & 1 for i in leaves)
                expected = bytearray(blank)
                if not non_solid:
                    struct.pack_into('<f', expected, 0x2c, 0.)
                    expected[0x36:0x38] = b'\1\1'
                    struct.pack_into('<f', expected, 0x38, 1.)
                assert row[24:] == expected[24:], ('independent zero-mask world finish', case)
                independent['zero_mask_full_trace_predictions'] += 1
            if compute:
                base = [f32(offset[a]+center[a]) for a in range(3)]
                end = [f32(delta[a]+base[a]) if fraction == 1. else f32(f32(delta[a]*fraction)+base[a]) for a in range(3)]
                if left == 1.:
                    assert fraction == 0 and all_solid and start_solid
                    end = base
                start = [base[a] if left == 0. else f32(f32(delta[a]*left)+base[a]) for a in range(3)]
                assert row[:24] == struct.pack('<6f', *start, *end), ('independent stationary coordinates', case)
                independent['position_predictions'] += 1
                independent['retained_delta_solid_positions'] += left == 1. and start != end
            else:
                assert row[:24] == bytes(24), ('coordinate output disabled', case)
        if phase != 3 and wrap:
            assert previous is not None
            prev_source, prev_row = previous
            assert struct.unpack_from('<I', prev_source, 64)[0] == 0
            assert source[:64] == prev_source[:64] and source[68:] == prev_source[68:] and row == prev_row
            independent['wrapped_fresh_pairs'] += 1
        key = source[:56]+source[60:]+bytes([solidity])
        if not compute:
            compute_off[key] = row[24:]
        else:
            assert compute_off[key] == row[24:], ('compute option changed collision fields', case)
            independent['compute_mode_metadata_pairs'] += 1
        previous = source, row
        nonzero_delta = any(v != 0. for v in delta)
        negative_zero = any(struct.unpack_from('<I', row, 4*a)[0] == 0x80000000 for a in range(6))
        counts['cases'] += 1
        counts['negative_zero_positions'] += negative_zero
        counts['hits'] += fraction < 1
        counts['start_solid'] += start_solid
        counts['all_solid'] += all_solid
        counts['empty_worlds'] += not available
        counts['positions_disabled'] += not compute
        counts['nonzero_stationary_delta'] += not flags & 2 and nonzero_delta
        counts['nonzero_offsets'] += any(v != 0. for v in offset)
        counts['overflowed_setup_inverse'] += any(v and abs(1./v) > maximum for v in delta)
        counts['wrapped_child_generation'] += wrap
        counts['zero_masks'] += mask == 0
    assert phase_counts == [88064, 22528, 22528, 8, 64]
    assert counts['negative_zero_positions'] > 0 and independent['retained_delta_solid_positions'] > 0
    assert counts['overflowed_setup_inverse'] > 0
    return counts, independent


def main(directory):
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    image, evidence = verify_installed_ranges(ENGINE, run/'stationary-public-reference.local.txt', [
        ('complete public world wrapper', 0x10149eb0, 0x1014a12c),
        ('complete coordinate finisher', 0x10145e20, 0x10145f33),
        ('complete base trace copy', 0x10146de0, 0x10146e63),
        ('complete stationary world dispatch', 0x10148030, 0x10148156),
        ('complete stationary leaf selector', 0x10145810, 0x101459a4),
        ('complete stationary non-ray leaf', 0x10147f40, 0x1014802a),
        ('complete stationary displacement dispatcher', 0x1014d320, 0x1014d535),
        ('complete fallback wrapper', 0x1014ce00, 0x1014cf9c),
        ('complete nested visit setup', 0x10147c30, 0x10147c97),
        ('complete nested depth finish', 0x10145510, 0x1014551e),
        ('complete mixed hull world geometry', 0x10148c30, 0x10148f4a),
    ])
    table = image.b[image.offset(0x104129b0):image.offset(0x104129b0)+1024]
    assert hashlib.sha256(table).hexdigest() == PERMUTATION_SHA256
    fixture = run/'permutation-reference.local.bin'
    fixture.write_bytes(table)
    bsp = BSP.read_bytes()
    benchmark = Benchmark(bsp)
    baseline = counts = independent = None
    builds = {}
    native_counts = []
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = directory/f'stationary_hull_world_trace_{arch}.exe'
        log = (directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log, log
        binary = exe.read_bytes()
        pe = struct.unpack_from('<I', binary, 60)[0]
        assert binary[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H', binary, pe+4)[0] == machine
        assert struct.unpack_from('<H', binary, pe+24)[0] == (0x10b if arch == 'x86' else 0x20b)
        builds[arch] = dict(executable=str(exe), sha256=sha(exe), machine=hex(machine), warnings=0)
        for repeat in (1, 2):
            record = run/f'{arch}-{repeat}.bin'
            result = subprocess.run([str(exe), str(ENGINE) if arch == 'x86' else '-', str(BSP), str(fixture), str(record)], capture_output=True, text=True, timeout=120)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode == 0, (arch, repeat, result.returncode, result.stderr)
            current = json.loads(result.stdout)
            services = {name:current.pop(name) for name in ('native_cache_acquires', 'native_setup_observations', 'native_fallback_calls')}
            if arch == 'x86':
                assert services['native_setup_observations'] == current['cases']-current['empty_worlds']
                assert services['native_cache_acquires'] > 0 and services['native_fallback_calls'] > 0
                native_counts.append(services)
                assert all(value == services for value in native_counts)
            else:
                assert services == dict.fromkeys(services, 0)
            payload = record.read_bytes()
            if baseline is None:
                counts, independent = inspect_records(payload, benchmark, bsp)
                baseline = payload
            assert current == counts and payload == baseline, (arch, repeat, 'full public stationary records differ', current, counts)
    root = Path(__file__).resolve().parent
    sources = {}

    def visit(name):
        if name in sources:
            return
        p = root/name
        sources[name] = sha(p)
        for include in re.findall(r'^\s*#include\s*"([^"\r\n]+)"', p.read_text(), re.M):
            visit(include)

    for name in ['probe_stationary_hull_world_trace.cpp', 'build_stationary_hull_world_trace.cmd', 'verify_stationary_hull_world_trace.py',
                 'verify_stationary_hull_world.py', 'verify_stationary_hull_leaf.py', 'verify_stationary_displacement_hull_policy.py']:
        visit(name)
    components = []
    for relative, expected in [('20261128-stationary-world/verify-nug5kaze/results.json', 147240),
                               ('20261127-stationary-hull-normal/swept-regression/verify-w7v2jz67/results.json', 19264)]:
        p = directory.parent/relative
        previous = json.loads(p.read_text())
        assert previous['repeats_per_arch'] == 2 and previous['counts']['cases'] == expected
        for name, value in previous['source_sha256'].items():
            assert sha(root/name) == value, ('accepted component source changed', name)
        for build in previous['builds'].values():
            assert sha(Path(build['executable'])) == build['sha256']
        components.append(dict(receipt=str(p), record_sha256=previous['record_sha256'], source_and_binary_hashes_unchanged=True))
    editor = json.loads((root.parent/'publication/stage/docs/build-status.json').read_text())
    assert len(editor['editor_source_dependencies']) == 543
    assert all(sha(root.parent/name) == value for name, value in editor['editor_source_dependencies'].items())
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    receipt = dict(status='VERIFIED PREPARED PUBLIC STATIONARY NON-RAY HULL TRACE', counts=counts, independent_checks=independent,
                   repeats_per_arch=2, builds=builds, source_sha256=sources, record_bytes=len(baseline),
                   record_sha256=hashlib.sha256(baseline).hexdigest(), original_engine_sha256=ENGINE_SHA256,
                   original_bsp_sha256=BSP_SHA256, originals_unchanged=True, permutation_sha256=PERMUTATION_SHA256,
                   production_permutation_bundled=False, editor_dependencies_unchanged=543,
                   native_services_per_run=native_counts, verified_spans=[row['name'] for row in evidence], accepted_components=components,
                   replaced_services=['public work acquire10149230/release1000e020', 'cache build bucket insertion1015b150/plane append10001610', 'prepared cache acquire1015c760/release10261300'],
                   findings=[
                       'Full original10149eb0 executes memset/default initialization, real stationary world collision, optional original coordinate finishing and full84-byte copy. Work/cache storage services are declared fixtures.',
                       'Hardware observation at original10148030 verifies every center/end/min/max/extent/delta/inverse setup component, full84-byte reset, mask, non-ray/nonswept flags, marker/normal reset and actual caller.',
                       'Parent/child visit buffers and stale rollover storage are supplied; original nested generation/depth setup/finish executes. Query input and work/output guards are unchanged, services balance and final public depth decrements to-1.',
                       'All84public trace bytes plus the separately observed internal displacement marker match. Distinct fixture properties normalize native surface-name addresses; world surface index is retained through its semantic resolver.',
                       'Finite nonzero deltas whose squared length underflows to0remain intact. Some initial inverse components overflow, but the nonswept route ignores them until a real fallback replaces its temporary ray; output uses original supplied delta.',
                       'Independent float-rounded coordinates, zero-mask world finishes, empty-world defaults, classification, wrapped/fresh equality, compute-mode metadata and signed-zero/cancellation fixtures pass.',
                       'Both44032-query benchmark face blocks match; both fresh warning-free architectures repeated twice produce identical records. Existing world and swept components are unchanged.'],
                   limits=[
                       'Available worlds admit nonswept non-ray hulls only. Stationary isRay=true changes normal selection and nested point geometry and is explicitly not claimed by this helper.',
                       'Eight empty-world queries cover all flag combinations before ray validation. Their internal marker starts0as part of the work fixture; only84bytes are the public trace contract.',
                       'Constructed hull queries cover symmetric/asymmetric offsets and underflow deltas;64provided Ray_t fixtures additionally cover signed zero and large center/offset cancellation.',
                       'Original full loader, effective material/physics binding, work/cache pool allocation/lifetime/concurrency and production permutation resource distribution remain separate gates.',
                       'Engine/server entity/static-prop/filtering, actual spawn acceptance, point-ray parity and editor integration remain unverified. All543editor dependencies and its executable are unchanged.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'], counts=counts, independent_checks=independent, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
