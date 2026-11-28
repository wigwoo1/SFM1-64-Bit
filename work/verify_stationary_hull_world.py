"""Verify complete prepared stationary hull world dispatch with native x86.

External private engine/BSP/permutation inputs are never embedded in source.
Selection, visited-leaf finish, bounds and zero-mask state have separate checks.
"""
import argparse
from collections import deque
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
from verify_stationary_hull_leaf import Benchmark, MASKS, MODES, initial_trace, visit_bits
from verify_stationary_displacement_hull_policy import f32


def queries(phase):
    if phase == 0:
        for _, index, q, hull, mask, mode, visit in itertools.product(range(2), range(43), range(3), range(3), MASKS, MODES, range(3)):
            yield index, q, hull, 0, mask, mode, visit, 0, None
    else:
        for q, hull, root_mode, mask, mode, visit, disabled, solidity in itertools.product(
                range(2 if phase == 3 else 11), range(2 if phase == 3 else 3), range(3), MASKS, MODES, range(3), range(2), range(4)):
            yield 0, q, hull, root_mode, mask, mode, visit, disabled, solidity


def select_leaves(planes, nodes, root, center, extents):
    """Float-rounded plane interval classification with deferred FIFO fronts."""
    pending = deque()
    cursor = root
    top = last = -1
    leaves = []
    while True:
        if cursor < 0:
            if top == -1:
                top = last
            if len(leaves) < 1024:
                leaves.append(-1-cursor)
            if not pending:
                return top, tuple(leaves)
            cursor = pending.popleft()
            continue
        plane, front, back = nodes[cursor]
        nx, ny, nz, distance, _ = planes[plane]
        dot = f32(f32(f32(center[1]*ny)+f32(center[0]*nx))+f32(center[2]*nz))
        separation = f32(dot-distance)
        radius = f32(abs(extents[1]*ny)+abs(extents[0]*nx)+abs(extents[2]*nz))
        last = cursor
        if separation >= radius:
            cursor = front
        elif separation < -radius:
            cursor = back
        else:
            if top == -1:
                top = cursor
            assert len(pending) < 1023
            pending.append(front)
            cursor = back


def contents_for(count, mode):
    return [0 if mode == 0 else 1 if mode == 1 else int(i+1 != count) if mode == 2 else 0x81 if i == 0 else 0x80 for i in range(count)]


def initial_brush_marks(count, index, mode):
    if mode == 0:
        return set()
    if mode == 1:
        return {index % count}
    return {i for i in range(count) if i % 3 == index % 3}


def inspect_records(payload, benchmark, data):
    assert struct.unpack_from('<10I', payload) == (0x53574831, 46440, 95040, 5760, 560, 1866, 43, 3, 2, 1024)
    leaf_offset, leaf_bytes = struct.unpack_from('<2I', data, 8+10*16)
    actual_contents = [struct.unpack_from('<I', data, leaf_offset+32*i)[0] for i in range(leaf_bytes//32)]
    counts = dict(cases=0, benchmark_cases=46440, controlled_cases=95040, capacity_cases=5760,
                  selected_leaves=0, visited_leaves=0, solid_leaves_visited=0, all_solid=0,
                  solid_leaf_fallbacks=0, normal_fallback_calls=0, multiple_fallback_queries=0,
                  max_fallbacks_per_query=0, zero_extents=0, zero_masks=0, premarked_queries=0,
                  unvisited_non_solid_selected=0, capacity_limited_queries=0, public_binding_checks=4750, passed=True)
    checks = dict(ordered_leaf_selections=0, visited_prefixes=0, visited_solidity_finishes=0,
                  zero_mask_full_trace_predictions=0, unchanged_no_fallback_metadata=0,
                  delta_inverse_predictions=0, monotonic_parent_visits=0,
                  analytic_controlled_selections=0, analytic_capacity_selections=0,
                  identical_ldr_hdr_queries=23220)
    cursor = 40
    midpoint = None
    for phase in range(4):
        bc, dc = (1866, 43) if phase == 0 else (3, 2)
        for index, q, hull, root_mode, mask, mode, visit, disabled, solidity in queries(phase):
            root, top, ns, nv, seen, calls = struct.unpack_from('<2i4I', payload, cursor)
            cursor += 24
            selected = struct.unpack_from('<' + str(ns) + 'I', payload, cursor)
            cursor += 4*ns
            visited = struct.unpack_from('<' + str(nv) + 'I', payload, cursor)
            cursor += 4*nv
            work = payload[cursor:cursor+560]
            cursor += 560
            brush_bits = payload[cursor:cursor+(bc+7)//8]
            cursor += (bc+7)//8
            disp_bits = payload[cursor:cursor+(dc+7)//8]
            cursor += (dc+7)//8
            point = struct.unpack_from('<3f', work)
            extents = ((0., 0., 0.), (2., 3., 5.), (24., 24., 41.))[hull] if phase == 0 else ((0., 0., 0.), (1., 1., 1.), (2., 3., 5.))[hull]
            selection_extents = tuple(f32(v+1.) for v in extents)
            assert work[0x18:0x24] == struct.pack('<3f', *(-v for v in extents))
            assert work[0x24:0x30] == work[0x30:0x3c] == struct.pack('<3f', *extents)
            assert work[12:24] == struct.pack('<3f', -91., -92., -93.)
            assert work[0x54:0x6c] == struct.pack('<6f', 11., 12., 13., 14., 15., 16.)
            assert struct.unpack_from('<I', work, 0xfc)[0] == mask and work[0x100:0x102] == bytes(2)
            assert struct.unpack_from('<2I', work, 0x11c) == (0, 17)
            assert struct.unpack_from('<I', work, 0x124)[0] == (calls if calls else 0xffffffff if visit == 2 else 0)
            assert struct.unpack_from('<I', work, 0x148)[0] == bc and struct.unpack_from('<I', work, 0x170)[0] == dc
            for field in (0x104, 0x128, 0x13c, 0x150, 0x164):
                assert work[field:field+4] == bytes(4)
            assert 0 < nv <= ns <= 1024 and visited == selected[:nv] and seen in (0, 1)
            if phase == 0:
                assert root == 0
                expected_top, expected_selected = select_leaves(benchmark.planes, benchmark.nodes, root, point, selection_extents)
                leaf_contents = actual_contents
                leaf_brushes = benchmark.brushes
            else:
                leaf_count = 1030 if phase == 3 else 2
                assert root == (0 if root_mode == 0 else -1 if root_mode == 1 else -leaf_count)
                x = ((0., 4.) if phase == 3 else (-20., -6., -3., -2., -1., 0., 1., 2., 3., 4., 50.))[q]
                assert point == (x, 0., 0.)
                leaf_contents = contents_for(leaf_count, solidity)
                leaf_brushes = [()] * leaf_count
                leaf_brushes[0 if phase == 3 else 1] = (0, 1, 2)
                if root < 0:
                    expected_top, expected_selected = -1, (-1-root,)
                elif phase == 3:
                    radius = selection_extents[0]
                    expected_top, expected_selected = (1028, (1029,)) if x >= radius else (0, (0,)) if x < -radius else (0, tuple(range(1024)))
                else:
                    nx, ny = (1., 0.) if phase == 1 else (f32(.6), f32(.8))
                    distance = f32(x*nx)
                    radius = f32(selection_extents[0]*nx + selection_extents[1]*ny)
                    expected_top, expected_selected = 0, (0,) if distance >= radius else (1,) if distance < -radius else (1, 0)
                checks['analytic_capacity_selections' if phase == 3 else 'analytic_controlled_selections'] += 1
            assert (top, selected) == (expected_top, expected_selected), (counts['cases'], 'selection', (top, selected), (expected_top, expected_selected))
            expected_seen = any(not leaf_contents[i] & 1 for i in visited)
            assert seen == expected_seen
            fraction = struct.unpack_from('<f', work, 0x80)[0]
            left = struct.unpack_from('<f', work, 0x8c)[0]
            all_solid, start_solid = work[0x8a:0x8c]
            marker = struct.unpack_from('<I', work, 0x114)[0]
            assert fraction in (0., .25, 1.) and left in (0., .75, 1.) and all_solid in (0, 1) and start_solid in (0, 1)
            if not seen:
                assert (fraction, left, all_solid, start_solid) == (0., 1., 1, 1)
            if nv < ns:
                assert all_solid == 1
            brush_initial = initial_brush_marks(bc, index, visit)
            brush_possible = set(brush_initial)
            for leaf in visited:
                brush_possible.update(leaf_brushes[leaf])
            initial_bits, possible_bits = visit_bits(brush_initial, bc), visit_bits(brush_possible, bc)
            assert all(a & i == i and a | p == p for a, i, p in zip(brush_bits, initial_bits, possible_bits))
            disp_initial = {index if visit == 1 else (index+1) % dc} if visit else set()
            initial_disp_bits = visit_bits(disp_initial, dc)
            assert all(a & i == i for a, i in zip(disp_bits, initial_disp_bits))
            assert not disp_bits[-1] & (0xff << (dc % 8)) & 255
            if mode & 4:
                assert calls == 0 and disp_bits == initial_disp_bits
            if mask == 0:
                assert brush_bits == possible_bits and disp_bits == initial_disp_bits
                expected = initial_trace(mode)
                if calls:
                    struct.pack_into('<fI', expected, 0x2c, 1., 0)
                    expected[0x36:0x38] = bytes(2)
                    struct.pack_into('<fI2H', expected, 0x38, 0., 0xffffffff, 0x3344, 0)
                if not seen:
                    struct.pack_into('<f', expected, 0x2c, 0.)
                    expected[0x36:0x38] = b'\1\1'
                    struct.pack_into('<f', expected, 0x38, 1.)
                assert work[0x54:0xa8] == expected and marker == (0 if calls else 0xffffffff if mode else 0)
                checks['zero_mask_full_trace_predictions'] += 1
            if calls:
                normal = struct.unpack_from('<3f', work, 0x108)
                delta = [f32(f32(point[a]+f32(normal[a]*99999.8984375))-point[a]) for a in range(3)]
                inverse = [f32(1./v) if v else struct.unpack('<f', bytes.fromhex('ffff7f7f'))[0] for v in delta]
                assert work[0x3c:0x54] == struct.pack('<6f', *delta, *inverse)
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
                checks['delta_inverse_predictions'] += 1
            else:
                assert work[0x3c:0x54] == b'\xa7'*24 and work[0x108:0x114] == b'\xa7'*12
                seed = initial_trace(mode)
                for begin, end in ((0, 44), (52, 54), (60, 84)):
                    assert work[0x54+begin:0x54+end] == seed[begin:end]
                assert marker == (0xffffffff if mode else 0)
                checks['unchanged_no_fallback_metadata'] += 1
            counts['cases'] += 1
            counts['selected_leaves'] += ns
            counts['visited_leaves'] += nv
            counts['solid_leaves_visited'] += sum(bool(leaf_contents[i] & 1) for i in visited)
            counts['all_solid'] += bool(all_solid)
            counts['solid_leaf_fallbacks'] += not seen
            counts['normal_fallback_calls'] += calls
            counts['multiple_fallback_queries'] += calls > 1
            counts['max_fallbacks_per_query'] = max(counts['max_fallbacks_per_query'], calls)
            counts['zero_extents'] += hull == 0
            counts['zero_masks'] += mask == 0
            counts['premarked_queries'] += visit != 0
            counts['unvisited_non_solid_selected'] += not seen and any(not leaf_contents[i] & 1 for i in selected)
            counts['capacity_limited_queries'] += ns == 1024
            checks['ordered_leaf_selections'] += 1
            checks['visited_prefixes'] += 1
            checks['visited_solidity_finishes'] += 1
            checks['monotonic_parent_visits'] += 1
            if counts['cases'] == 23220:
                midpoint = cursor
        if phase == 0:
            assert payload[40:midpoint] == payload[midpoint:cursor], 'LDR/HDR complete world records differ'
    assert cursor == len(payload)
    assert counts['capacity_limited_queries'] == 960 and counts['unvisited_non_solid_selected'] == 1080
    return counts, checks


def main(directory):
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    image, evidence = verify_installed_ranges(ENGINE, run/'stationary-world-reference.local.txt', [
        ('complete stationary world dispatch', 0x10148030, 0x10148156),
        ('complete stationary leaf selector', 0x10145810, 0x101459a4),
        ('complete stationary non-ray leaf', 0x10147f40, 0x1014802a),
        ('complete stationary brush primitive', 0x10146c30, 0x10146dd7),
        ('complete stationary displacement dispatch', 0x1014d320, 0x1014d535),
        ('complete normal fallback wrapper', 0x1014ce00, 0x1014cf9c),
        ('complete nested visit setup', 0x10147c30, 0x10147c97),
        ('complete nested depth finish', 0x10145510, 0x1014551e),
        ('complete mixed world hull geometry', 0x10148c30, 0x10148f4a),
        ('original stack probe', 0x102df090, 0x102df0bb),
        ('original cookie check', 0x102de362, 0x102de36c),
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
        exe = directory/f'stationary_hull_world_{arch}.exe'
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
            acquires = current.pop('native_cache_acquires')
            assert acquires == (15152 if arch == 'x86' else 0)
            if arch == 'x86':
                native_counts.append(acquires)
            payload = record.read_bytes()
            if baseline is None:
                counts, independent = inspect_records(payload, benchmark, bsp)
                baseline = payload
            assert current == counts and payload == baseline, (arch, repeat, 'full stationary world records differ', current, counts)
    root = Path(__file__).resolve().parent
    sources = {}

    def visit(name):
        if name in sources:
            return
        p = root/name
        sources[name] = sha(p)
        for include in re.findall(r'^\s*#include\s*"([^"\r\n]+)"', p.read_text(), re.M):
            visit(include)

    for name in ['probe_stationary_hull_world.cpp', 'build_stationary_hull_world.cmd', 'verify_stationary_hull_world.py',
                 'verify_stationary_hull_leaf.py', 'verify_stationary_displacement_hull_policy.py']:
        visit(name)
    previous_path = directory.parent/'20261128-stationary-leaf/verify-hlovln0i/results.json'
    previous = json.loads(previous_path.read_text())
    assert previous['repeats_per_arch'] == 2 and previous['counts']['cases'] == 145440
    for name, value in previous['source_sha256'].items():
        assert sha(root/name) == value, ('accepted leaf source changed', name)
    for build in previous['builds'].values():
        assert sha(Path(build['executable'])) == build['sha256']
    editor = json.loads((root.parent/'publication/stage/docs/build-status.json').read_text())
    assert len(editor['editor_source_dependencies']) == 543
    assert all(sha(root.parent/name) == value for name, value in editor['editor_source_dependencies'].items())
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    receipt = dict(status='VERIFIED PREPARED STATIONARY HULL WORLD', counts=counts, independent_checks=independent,
                   repeats_per_arch=2, builds=builds, source_sha256=sources, record_bytes=len(baseline),
                   record_sha256=hashlib.sha256(baseline).hexdigest(), original_engine_sha256=ENGINE_SHA256,
                   original_bsp_sha256=BSP_SHA256, originals_unchanged=True, permutation_sha256=PERMUTATION_SHA256,
                   production_permutation_bundled=False, editor_dependencies_unchanged=543,
                   native_cache_acquires_per_run=native_counts, verified_spans=[row['name'] for row in evidence],
                   previous_leaf=dict(receipt=str(previous_path), record_sha256=previous['record_sha256'], source_and_binary_hashes_unchanged=True),
                   replaced_services=['cache build bucket insertion1015b150/plane append10001610', 'prepared cache acquire1015c760/release10261300'],
                   findings=[
                       'Full original10148030 calls original selection10145810, real stationary leaf/brush/terrain and actual nested world hull fallback. Selection and leaf boundaries execute through hardware observations.',
                       'Selection takes supplied Ray_t center/extents+1 and root, caps stored results at1024 and preserves deferred FIFO fronts/back-first order. Actual selected and visited sequences and topNode match.',
                       'Leaf contents bit1 only tracks whether a processed leaf is non-solid. Solid leaves are still called. Check allSolid after each leaf; final fallback depends on visited leaves, not all selected leaves.',
                       'All560work bytes, complete parent visits, nested fallback count/generation and selection/visit vectors match before records are written. Repeated normal probes advance child generation through up to6calls; parent generation remains17.',
                       'Analytic two-leaf axial/oblique and1030-leaf comb fixtures check root-leaf inputs, asymmetric boundary equality, output-capacity truncation and selected-but-unvisited non-solid leaves.',
                       'Independent selection, visited prefix/solidity finish, zero-mask full trace, metadata and delta/inverse checks pass. Both23220-query benchmark face blocks match; both architectures repeated twice yield identical records.'],
                   limits=[
                       'Prepared finite non-ray work with centered zero/nonzero extents, valid bounded-queue BSP topology and explicit tables. Queue overflow and malformed geometry are outside this native comparison.',
                       'Original whole BSP loader, global work/cache lifecycle, effective material/physics binding and production permutation resource distribution remain unverified; geometry/property/storage fixtures are explicit.',
                       'Native temporary query Ray_t and work center/extents agree here as in the public caller. Divergent arbitrary internal buffers and arbitrary recursive depth are not admitted as production contracts.',
                       'Surface-name addresses normalize through distinct fixture material properties. No original instruction or table data is bundled in owned code.',
                       'Public stationary trace setup/finish and empty-world return, Ray_t nonswept classification, engine/server entity/static-prop/filtering, actual spawn and editor integration remain unverified.',
                       'All543editor dependencies are unchanged; this is not a demonstrated editor/visual improvement or completed native SFM compatibility.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'], counts=counts, independent_checks=independent, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
