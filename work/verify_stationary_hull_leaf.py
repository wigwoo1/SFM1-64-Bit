"""Verify complete prepared stationary hull leaf composition and visit policy.

Native routines execute real brush/terrain geometry. Original code, lookup data,
map inputs and comparison records remain private external fixtures.
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
from verify_benchmark_world_hull import ENGINE_SHA256, BSP_SHA256, PERMUTATION_SHA256
from verify_stationary_displacement_hull_policy import f32, order_hash

MODES = (0, 1, 2, 4, 9)
MASKS = (0, 1, 0x2080, 0xffffffff)
BRUSH_LISTS = ((), (0,), (1,), (2, 0, 1, 0), (1, 0, 1), (2, 2))
DISP_LISTS = ((), (1, 0), (0, 1, 0))


def inputs(controlled):
    if controlled:
        for q, hull, bl, dl, mask, mode, visit, disabled in itertools.product(
                range(5), range(2), range(6), range(3), MASKS, MODES, range(3), range(2)):
            yield 0, q, hull, bl, dl, mask, mode, visit, disabled
    else:
        for _, index, q, hull, li, mask, mode, visit in itertools.product(
                range(2), range(43), range(3), range(2), range(4), MASKS, MODES, range(3)):
            yield index, q, hull, li, None, mask, mode, visit, 0


def visit_bits(indices, count):
    data = bytearray((count+7)//8)
    for index in indices:
        assert 0 <= index < count
        data[index//8] |= 1 << (index % 8)
    return bytes(data)


def initial_trace(mode):
    trace = bytearray(84)
    struct.pack_into('<6f4f4B', trace, 0, 11., 12., 13., 14., 15., 16., 9., 8., 7., 6., 5, 4, 3, 2)
    struct.pack_into('<fI', trace, 0x2c, (1., .25, 0.)[mode % 3], 0x12345678)
    struct.pack_into('<H', trace, 0x34, 0xbeef)
    trace[0x36] = bool(mode & 8)
    trace[0x37] = bool(mode & 4)
    struct.pack_into('<fI2H', trace, 0x38, .75 if mode & 2 else 0., 0x10000000+77, 0xabcd, 0xef01)
    struct.pack_into('<IH', trace, 0x44, 0x11112222, 0x3344)
    struct.pack_into('<H2I', trace, 0x4a, 77 if mode else 23, 0x55556666, 0x77778888)
    return trace


class Benchmark:
    def __init__(self, data):
        def lump(index):
            return struct.unpack_from('<2I', data, 8+16*index)
        o, n = lump(1)
        self.planes = [struct.unpack_from('<4fI', data, o+20*i) for i in range(n//20)]
        o, n = lump(5)
        self.nodes = [struct.unpack_from('<I2i', data, o+32*i) for i in range(n//32)]
        refs, _ = lump(17)
        o, n = lump(10)
        self.brushes = []
        for i in range(n//32):
            first, count = struct.unpack_from('<2H', data, o+32*i+24)
            self.brushes.append(struct.unpack_from('<' + str(count) + 'H', data, refs+2*first))
        o, n = lump(18)
        self.contents = [struct.unpack_from('<I', data, o+12*i+8)[0] for i in range(n//12)]
        o, n = lump(26)
        self.disp_contents = [struct.unpack_from('<I', data, o+176*i+32)[0] for i in range(n//176)]
        o, n = lump(6)
        self.flags = [0] * (lump(2)[1]//32)
        for i in range(n//72):
            flags, material = struct.unpack_from('<2I', data, o+72*i+64)
            self.flags[material] |= flags & 65535
        assert len(self.contents) == 1866 and len(self.disp_contents) == 43 and len(self.flags) == 87

    def point_brushes(self, point):
        node = 0
        while node >= 0:
            plane, front, back = self.nodes[node]
            x, y, z, distance, _ = self.planes[plane]
            dot = f32(f32(f32(point[0]*x)+f32(point[1]*y))+f32(point[2]*z))
            node = front if f32(dot-distance) >= 0 else back
        return self.brushes[-1-node]


def inspect_records(payload, benchmark):
    assert struct.unpack_from('<8I', payload) == (0x534c4632, 123840, 21600, 560, 1866, 43, 3, 2)
    counts = dict(cases=0, benchmark_cases=123840, controlled_cases=21600, brush_calls=0,
                  displacement_dispatches=0, direct_overlaps=0, fallbacks=0, start_solid=0,
                  fallback_solid=0, zero_masks=0, premarked_queries=0, empty_brush_dispatch=0,
                  initial_zero_dispatch=0, surface_exclusion_bypasses=0, public_binding_checks=4692, passed=True)
    checks = dict(brush_visit_replays=0, displacement_visit_replays=0, controlled_full_brush_predictions=0,
                  no_dispatch_metadata_checks=0, direct_metadata_checks=0, delta_inverse_predictions=0,
                  controlled_cull_overlap_order=0, identical_ldr_hdr_queries=61920)
    cursor = 32
    midpoint = None
    for controlled in (False, True):
        brush_contents = (1, 0x80, 0) if controlled else benchmark.contents
        disp_contents = (1, 2) if controlled else benchmark.disp_contents
        bc, dc = len(brush_contents), len(disp_contents)
        for index, q, hull, bl, dl, mask, mode, visit, disabled in inputs(controlled):
            brush_count, disp_count = struct.unpack_from('<2I', payload, cursor)
            cursor += 8
            brushes = struct.unpack_from('<' + str(brush_count) + 'I', payload, cursor)
            cursor += 4*brush_count
            displacements = struct.unpack_from('<' + str(disp_count) + 'I', payload, cursor)
            cursor += 4*disp_count
            work = payload[cursor:cursor+560]
            cursor += 560
            brush_bits = payload[cursor:cursor+(bc+7)//8]
            cursor += (bc+7)//8
            disp_bits = payload[cursor:cursor+(dc+7)//8]
            cursor += (dc+7)//8
            calls, call_hash, dispatched, direct, fallback, culls, overlaps, cull_hash, overlap_hash = struct.unpack_from('<9I', payload, cursor)
            cursor += 36
            assert dispatched in (0, 1) and fallback in (0, 1)
            assert all(b < bc for b in brushes) and all(d < dc for d in displacements)
            assert (direct == 0xffffffff or direct < dc) and bool(dispatched) == (direct != 0xffffffff or bool(fallback))
            point = struct.unpack_from('<3f', work)
            extents = ((1., 1., 1.) if hull == 0 else (2., 3., 5.)) if controlled else ((2., 3., 5.) if hull == 0 else (24., 24., 41.))
            assert work[12:24] == struct.pack('<3f', -91., -92., -93.)
            assert work[0x18:0x24] == struct.pack('<3f', *(-v for v in extents))
            assert work[0x24:0x30] == work[0x30:0x3c] == struct.pack('<3f', *extents)
            assert struct.unpack_from('<I', work, 0xfc)[0] == mask and work[0x100:0x102] == b'\0\0'
            assert struct.unpack_from('<2I', work, 0x11c) == (0, 17)
            assert struct.unpack_from('<I', work, 0x124)[0] == (1 if fallback else 0xffffffff if visit == 2 else 0)
            assert struct.unpack_from('<I', work, 0x148)[0] == bc
            assert struct.unpack_from('<I', work, 0x170)[0] == dc
            for field in (0x104, 0x128, 0x13c, 0x150, 0x164):
                assert work[field:field+4] == bytes(4)
            nxt = 1 if controlled else (index+1) % dc
            if controlled:
                assert point == ((-3., -2., -6., 4., 0.)[q], 0., 0.)
                assert brushes == BRUSH_LISTS[bl] and displacements == DISP_LISTS[dl]
            else:
                expected = benchmark.point_brushes(point)
                if bl == 0:
                    assert brushes == expected
                elif bl == 1:
                    assert not brushes and displacements == (index, index, nxt)
                elif bl == 2:
                    assert brushes == expected + (expected[:1] if expected else ()) and displacements == (nxt, index)
                else:
                    assert brushes == (index % 1866, (index+17) % 1866, index % 1866) and not displacements
            fraction = struct.unpack_from('<f', work, 0x80)[0]
            contents = struct.unpack_from('<I', work, 0x84)[0]
            all_solid, start_solid = work[0x8a:0x8c]
            left = struct.unpack_from('<f', work, 0x8c)[0]
            marker = struct.unpack_from('<I', work, 0x114)[0]
            seed = initial_trace(mode)
            brush_marks = {brushes[0 if visit == 1 else -1]} if visit and brushes else set()
            call_order = []
            predicted = bytearray(seed)
            stopped = False
            for brush in brushes:
                if brush in brush_marks:
                    continue
                brush_marks.add(brush)
                if not brush_contents[brush] & mask:
                    continue
                call_order.append(brush)
                if controlled:
                    hit = point[0]+extents[0] >= -4. and point[0]-extents[0] <= -1.
                    if hit:
                        struct.pack_into('<fI', predicted, 0x2c, 0., brush_contents[brush])
                        predicted[0x36:0x38] = b'\1\1'
                        struct.pack_into('<f', predicted, 0x38, 1.)
                    if struct.unpack_from('<f', predicted, 0x2c)[0] == 0:
                        stopped = True
                        break
                elif not dispatched and calls and fraction == 0 and len(call_order) == calls:
                    stopped = True
                    break
            assert (len(call_order), order_hash(call_order)) == (calls, call_hash)
            assert brush_bits == visit_bits(brush_marks, bc)
            if mode % 3 == 2:
                assert calls <= 1
            if stopped or mode & 4:
                assert not dispatched
            if controlled:
                assert bool(dispatched) == (not stopped and not predicted[0x37] and bool(displacements))
                checks['controlled_full_brush_predictions'] += 1
            else:
                if not stopped and not mode & 4:
                    assert bool(dispatched) == bool(displacements)
            if not dispatched:
                assert direct == 0xffffffff and not fallback and culls == overlaps == 0
                assert cull_hash == overlap_hash == 2166136261
                if controlled:
                    assert work[0x54:0xa8] == predicted
                else:
                    expected = bytearray(seed)
                    if work[0x54:0xa8] != expected:
                        assert calls and fraction == 0 and all_solid == start_solid == 1 and left == 1
                        struct.pack_into('<fI', expected, 0x2c, 0., brush_contents[call_order[-1]])
                        expected[0x36:0x38] = b'\1\1'
                        struct.pack_into('<f', expected, 0x38, 1.)
                    assert work[0x54:0xa8] == expected
                assert marker == (0xffffffff if mode else 0)
                checks['no_dispatch_metadata_checks'] += 1
            elif direct != 0xffffffff:
                expected = bytearray(seed)
                struct.pack_into('<fI', expected, 0x2c, 0., disp_contents[direct])
                expected[0x36:0x38] = b'\1\1'
                struct.pack_into('<f', expected, 0x38, 0.)
                assert work[0x54:0xa8] == expected and marker == (0xffffffff if mode else 0)
                checks['direct_metadata_checks'] += 1
            else:
                assert fraction in (0., 1.) and left == 0
                assert all_solid == start_solid == int(fraction == 0.)
                assert marker in (0, 1) and (marker == 1 if fraction == 0 else contents == 0)
                name = struct.unpack_from('<I', work, 0x90)[0]
                prop, surface_flags = struct.unpack_from('<2H', work, 0x94)
                if name == 0xffffffff:
                    assert (prop, surface_flags) == (0x3344, 0)
                elif 0x10000000 <= name < 0x10000000+87:
                    material = name-0x10000000
                    assert (prop, surface_flags) == (1000+material, benchmark.flags[material])
                else:
                    mesh = name-0x20000000
                    assert 0 <= mesh < dc and prop in (100+mesh, 500+mesh) and surface_flags == 0
                normal = struct.unpack_from('<3f', work, 0x108)
                delta = [f32(f32(point[a]+f32(normal[a]*99999.8984375))-point[a]) for a in range(3)]
                inverse = [f32(1./v) if v else struct.unpack('<f', bytes.fromhex('ffff7f7f'))[0] for v in delta]
                assert work[0x3c:0x54] == struct.pack('<6f', *delta, *inverse)
                checks['delta_inverse_predictions'] += 1
            if not fallback:
                assert work[0x3c:0x54] == b'\xa7'*24 and work[0x108:0x114] == b'\xa7'*12
            disp_marks = {index if visit == 1 else nxt} if visit else set()
            cull_order = []
            if dispatched:
                for disp in displacements:
                    if not disp_contents[disp] & mask or disp in disp_marks:
                        continue
                    disp_marks.add(disp)
                    cull_order.append(disp)
                    if disp == direct:
                        break
            assert disp_bits == visit_bits(disp_marks, dc)
            assert (len(cull_order), order_hash(cull_order)) == (culls, cull_hash)
            assert overlaps <= culls
            if controlled:
                bounds = ((3., -9., -9., 5., 9., 9.), (49., -9., -9., 51., 9., 9.))
                admitted = [d for d in cull_order if all(point[a]-extents[a] <= bounds[d][a+3] and bounds[d][a] <= point[a]+extents[a] for a in range(3))]
                assert (len(admitted), order_hash(admitted)) == (overlaps, overlap_hash)
                if disabled:
                    assert direct != 0
                checks['controlled_cull_overlap_order'] += 1
            counts['cases'] += 1
            counts['brush_calls'] += calls
            counts['displacement_dispatches'] += dispatched
            counts['direct_overlaps'] += direct != 0xffffffff
            counts['fallbacks'] += fallback
            counts['start_solid'] += bool(start_solid)
            counts['fallback_solid'] += bool(fallback and start_solid)
            counts['zero_masks'] += mask == 0
            counts['premarked_queries'] += visit != 0
            counts['empty_brush_dispatch'] += not brushes and bool(dispatched)
            counts['initial_zero_dispatch'] += mode % 3 == 2 and bool(dispatched)
            counts['surface_exclusion_bypasses'] += controlled and mask == 0x2080 and contents == 0x80 and 1 in call_order
            checks['brush_visit_replays'] += 1
            checks['displacement_visit_replays'] += 1
            if counts['cases'] == 61920:
                midpoint = cursor
        if not controlled:
            assert payload[32:midpoint] == payload[midpoint:cursor], 'LDR/HDR complete leaf records differ'
    assert cursor == len(payload)
    assert counts['surface_exclusion_bypasses'] == 1050 and counts['initial_zero_dispatch'] == 18776
    assert counts['fallback_solid'] == 616
    return counts, checks


def main(directory):
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    image, evidence = verify_installed_ranges(ENGINE, run/'stationary-leaf-reference.local.txt', [
        ('complete stationary non-ray leaf', 0x10147f40, 0x1014802a),
        ('complete stationary brush primitive', 0x10146c30, 0x10146de0),
        ('complete stationary displacement dispatcher', 0x1014d320, 0x1014d535),
        ('complete mesh box overlap including triangle tail', 0x1015a7d0, 0x1015abab),
        ('complete normal selection', 0x1014cd40, 0x1014cdf5),
        ('complete normal fallback wrapper', 0x1014ce00, 0x1014cf9c),
        ('complete nested visit setup', 0x10147c30, 0x10147c97),
        ('complete mixed world hull geometry', 0x10148c30, 0x10148f4a),
    ])
    table = image.b[image.offset(0x104129b0):image.offset(0x104129b0)+1024]
    assert hashlib.sha256(table).hexdigest() == PERMUTATION_SHA256
    fixture = run/'permutation-reference.local.bin'
    fixture.write_bytes(table)
    benchmark = Benchmark(BSP.read_bytes())
    baseline = counts = independent = None
    builds = {}
    native_counts = []
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = directory/f'stationary_hull_leaf_{arch}.exe'
        log = (directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log, log
        binary = exe.read_bytes()
        pe = struct.unpack_from('<I', binary, 60)[0]
        assert binary[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H', binary, pe+4)[0] == machine
        assert struct.unpack_from('<H', binary, pe+24)[0] == (0x10b if arch == 'x86' else 0x20b)
        builds[arch] = dict(executable=str(exe), sha256=sha(exe), machine=hex(machine), warnings=0)
        for repeat in (1, 2):
            record = run/f'{arch}-{repeat}.bin'
            result = subprocess.run([str(exe), str(ENGINE) if arch == 'x86' else '-', str(BSP), str(fixture), str(record)], capture_output=True, text=True, timeout=90)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode == 0, (arch, repeat, result.returncode, result.stderr)
            current = json.loads(result.stdout)
            acquires = current.pop('native_cache_acquires')
            assert acquires == (14600 if arch == 'x86' else 0)
            if arch == 'x86':
                native_counts.append(acquires)
            payload = record.read_bytes()
            if baseline is None:
                counts, independent = inspect_records(payload, benchmark)
                baseline = payload
            assert current == counts and payload == baseline, (arch, repeat, 'complete leaf records differ', current, counts)
    root = Path(__file__).resolve().parent
    sources = {}

    def visit(name):
        if name in sources:
            return
        p = root/name
        sources[name] = sha(p)
        for include in re.findall(r'^\s*#include\s*"([^"\r\n]+)"', p.read_text(), re.M):
            visit(include)

    for name in ['probe_stationary_hull_leaf.cpp', 'build_stationary_hull_leaf.cmd', 'verify_stationary_hull_leaf.py', 'verify_stationary_displacement_hull_policy.py']:
        visit(name)
    previous_path = directory.parent/'20261128-stationary-displacement-policy/verify-06p8dj5x/results.json'
    previous = json.loads(previous_path.read_text())
    assert previous['repeats_per_arch'] == 2 and previous['counts']['cases'] == 131868
    for name, value in previous['source_sha256'].items():
        assert sha(root/name) == value, ('accepted dispatcher source changed', name)
    for build in previous['builds'].values():
        assert sha(Path(build['executable'])) == build['sha256']
    editor = json.loads((root.parent/'publication/stage/docs/build-status.json').read_text())
    assert len(editor['editor_source_dependencies']) == 543
    assert all(sha(root.parent/name) == value for name, value in editor['editor_source_dependencies'].items())
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    receipt = dict(status='VERIFIED PREPARED STATIONARY HULL LEAF', counts=counts, independent_checks=independent,
                   repeats_per_arch=2, builds=builds, source_sha256=sources, record_bytes=len(baseline),
                   record_sha256=hashlib.sha256(baseline).hexdigest(), original_engine_sha256=ENGINE_SHA256,
                   original_bsp_sha256=BSP_SHA256, originals_unchanged=True, permutation_sha256=PERMUTATION_SHA256,
                   production_permutation_bundled=False, editor_dependencies_unchanged=543,
                   native_cache_acquires_per_run=native_counts, verified_spans=[row['name'] for row in evidence],
                   previous_dispatcher=dict(receipt=str(previous_path), record_sha256=previous['record_sha256'], source_and_binary_hashes_unchanged=True),
                   replaced_services=['cache build bucket insertion1015b150/plane append10001610', 'prepared cache acquire1015c760/release10261300'],
                   findings=[
                       'Full original10147f40 executes stationary brush contacts, displacement dispatcher, overlap and nested world fallback over supplied prepared tables. No brush or terrain geometry is substituted.',
                       'Leaf brush entry and displacement entry are hardware observations executing through code. Full brush/cull/overlap order vectors,560work bytes and complete parent visit arrays match before bit packing.',
                       'Brush visit writes precede contents filtering. No swept surface exclusion applies:1050controlled cases accept the flagged0x80brush with mask2080.',
                       'Fraction0stops only after a brush call.18776queries with prior zero fraction still enter terrain because no brush call terminates the leaf. Prior startSolid suppresses terrain even with no brushes.',
                       'Independent policy replays check all brush/terrain visit bits and call order.21600controlled queries additionally predict full brush state and terrain gating with closed-box interval geometry.',
                       'Both benchmark face modes produce identical61920-query blocks. Two fresh warning-free architecture builds, each repeated twice, produce identical records.'],
                   limits=[
                       'Prepared single-leaf queries with preserved non-ray work, nonzero centered hulls and finite fixture metadata. Leaf selection/orchestration is not included.',
                       'An appended test leaf supplies ordered references without changing fallback world topology. Benchmark geometry and controlled world tables are explicit inputs; this does not execute the original whole BSP loader.',
                       'Stationary world leaf enumeration/solid-leaf fallback and public trace setup/finish, engine/server filters, entities/static props, actual spawn and editor integration remain unverified.',
                       'Distinguishing material property IDs and cache/nested storage services are fixtures. Effective physics binding, cache lifecycle/concurrency and production permutation distribution remain separate.',
                       'All543editor dependencies are unchanged; this is not a verified visual/editor improvement or completed SFM compatibility.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'], counts=counts, independent_checks=independent, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
