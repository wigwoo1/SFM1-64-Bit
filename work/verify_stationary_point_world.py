"""Verify complete prepared stationary point leaves and world composition.

Intact original code executes over external private/prepared reference inputs.
This compares complete work, event order and losslessly encoded child visits.
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
from verify_stationary_hull_leaf import Benchmark, initial_trace
from verify_stationary_hull_world import select_leaves, contents_for, initial_brush_marks
from verify_stationary_displacement_hull_policy import f32
from verify_prepared_point_world import read_marks
from verify_stationary_point_policy import compile_probe

TOTAL, BENCHMARK, CONTROLLED, CAPACITY = 162288, 92880, 69120, 288
POLICY = 'source_stationary_displacement_hull_policy_candidate.h'
LEAF = 'source_stationary_hull_leaf_candidate.h'
WORLD = 'source_stationary_hull_world_candidate.h'


def read_runs(payload, offset, count):
    n = struct.unpack_from('<H', payload, offset)[0]; offset += 2
    values = []
    previous = None
    for _ in range(n):
        length, value = struct.unpack_from('<HI', payload, offset); offset += 6
        assert length > 0 and value != previous and len(values)+length <= count
        values.extend([value]*length); previous = value
    assert len(values) == count
    return values, offset


def inspect_records(payload, benchmark, bsp):
    assert struct.unpack_from('<10I', payload) == (0x53505731, BENCHMARK, CONTROLLED, CAPACITY, 560, 1866, 43, 3, 2, 1024)
    leaf_offset, leaf_bytes = struct.unpack_from('<2I', bsp, 8+10*16)
    benchmark_solidity = [struct.unpack_from('<I', bsp, leaf_offset+32*i)[0] for i in range(leaf_bytes//32)]
    counts = dict(cases=0, benchmark_cases=BENCHMARK, controlled_cases=CONTROLLED, capacity_cases=CAPACITY,
                  leaf_queries=0, world_queries=0, selected_leaves=0, visited_leaves=0,
                  solid_leaves_visited=0, solid_leaf_finishes=0, normal_fallback_calls=0,
                  multiple_fallback_queries=0, max_fallbacks_per_query=0, brush_calls=0,
                  terrain_dispatch_calls=0, tiny_extents=0, zero_extents=0, premarked_queries=0,
                  unvisited_non_solid_selected=0, capacity_limited_queries=0,
                  binding_checks=(TOTAL+30)//31, passed=True)
    checks = dict(complete_leaf_selection_predictions=0, actual_visited_prefix_predictions=0,
                  visited_solidity_finish_predictions=0, controlled_brush_and_terrain_gate_predictions=0,
                  zero_mask_full_trace_predictions=0, no_fallback_metadata_preservation=0,
                  complete_child_visit_vectors=0, fallback_ray_arithmetic_and_guard_checks=0,
                  tiny_point_classification=0, stationary_surface_exclusion_bypassed=0,
                  identical_ldr_hdr_queries=BENCHMARK//2)
    offset = 40; halfway = None; phases = [0]*4
    for case in range(TOTAL):
        row = struct.unpack_from('<18I', payload, offset); offset += 72
        point = struct.unpack('<3f', struct.pack('<3I', *row[:3]))
        extents = struct.unpack('<3f', struct.pack('<3I', *row[3:6]))
        mask, mode, visit, index, phase, q, root_mode, disabled, solidity, standalone = row[6:16]
        root, top = struct.unpack('<2i', struct.pack('<2I', *row[16:18]))
        assert phase <= 3 and mode in (0, 1, 2, 4, 9) and visit in (0, 1, 2)
        assert disabled in (0, 1, 2) and standalone in (0, 1)
        assert (phase == 0) == (case < BENCHMARK)
        phases[phase] += 1
        ns, nv, nb, nt, visited_non_solid, fallbacks = struct.unpack_from('<6I', payload, offset); offset += 24
        vectors = []
        for count in (ns, nv, nb, nt):
            vectors.append(struct.unpack_from('<'+'I'*count, payload, offset)); offset += 4*count
        selected, visited, brushes, terrain_leaves = vectors
        ends = [struct.unpack_from('<2f2I', payload, offset+16*i) for i in range(nv)]; offset += 16*nv
        work = payload[offset:offset+560]; offset += 560
        brush_count, disp_count = (1866, 43) if phase == 0 else (3, 2)
        parent = []
        for size in (brush_count, disp_count):
            raw = payload[offset:offset+(size+7)//8]; offset += len(raw)
            parent.append(read_marks(raw, size))
        child_brush, offset = read_runs(payload, offset, brush_count)
        child_disp, offset = read_runs(payload, offset, disp_count)
        brush_marks, disp_marks = parent
        original_brush_marks = initial_brush_marks(brush_count, index, visit)
        original_disp_marks = {(index+(visit == 2)) % disp_count} if visit else set()
        assert original_brush_marks <= brush_marks and original_disp_marks <= disp_marks
        assert len(set(brushes)) == nb and not set(brushes) & original_brush_marks
        assert set(brushes) <= brush_marks and all(i < brush_count for i in brushes)
        assert sum(float(v)*v for v in extents) < 1e-6
        checks['tiny_point_classification'] += any(extents)
        if phase == 0:
            planes, nodes = benchmark.planes, benchmark.nodes
            leaf_contents = benchmark_solidity; brush_lists = benchmark.brushes
            brush_contents = benchmark.contents
        else:
            planes = [(f32(.6), f32(.8), 0., 0., 3)] if phase == 2 else [(1., 0., 0., 0., 0)]
            node_count = 1029 if phase == 3 else 1
            nodes = [(0, i+1 if i+1 < node_count else -1030, -1-i) for i in range(node_count)] if phase == 3 else [(0, -1, -2)]
            leaf_count = 1030 if phase == 3 else 2
            leaf_contents = contents_for(leaf_count, solidity)
            brush_lists = [(0, 1, 2, 1) if i == (0 if phase == 3 else 1) else () for i in range(leaf_count)]
            brush_contents = (1, 0x80, 0)
        assert all(brush_contents[i] & mask for i in brushes)
        if standalone:
            assert top == -1 and selected == visited == (root,) and visited_non_solid == 0
        else:
            expected_top, expected_selected = select_leaves(planes, nodes, root, point, tuple(f32(v+1.) for v in extents))
            assert (top, selected) == (expected_top, expected_selected), ('point leaf selection', case)
            assert visited == selected[:nv] and 0 < nv <= ns <= 1024
            assert all(not state[2] for state in ends[:-1])
            if nv < ns:
                assert ends[-1][2] == 1
            assert visited_non_solid == any(not (leaf_contents[i]&1) for i in visited)
            checks['actual_visited_prefix_predictions'] += 1
        checks['complete_leaf_selection_predictions'] += 1
        assert len(ends) == nv and all(0 <= f <= 1 and 0 <= left <= 1 and a in (0, 1) and s in (0, 1) for f, left, a, s in ends)
        assert fallbacks <= nt <= fallbacks+1
        final = (struct.unpack_from('<f', work, 0x80)[0], struct.unpack_from('<f', work, 0x8c)[0], work[0x8a], work[0x8b])
        if not standalone and not visited_non_solid:
            assert final == (0., 1., 1, 1)
        else:
            assert final == ends[-1]
        checks['visited_solidity_finish_predictions'] += 1
        seed = bytearray(b'\xa7'*560)
        for at, values in ((0, point), (12, (-91., -92., -93.)), (0x18, tuple(-v for v in extents)),
                           (0x24, extents), (0x30, extents)):
            struct.pack_into('<3f', seed, at, *values)
        seed[0x54:0xa8] = initial_trace(mode)
        struct.pack_into('<I', seed, 0xfc, mask); seed[0x100:0x102] = b'\1\0'
        for at, value in ((0x104, 0), (0x114, 0xffffffff if mode else 0), (0x11c, 0), (0x120, 17),
                          (0x124, 0xffffffff if visit == 2 else 0), (0x128, 0), (0x13c, 0),
                          (0x148, brush_count), (0x150, 0), (0x164, 0), (0x170, disp_count)):
            struct.pack_into('<I', seed, at, value)
        expected_work = bytearray(seed)
        allowed = set(range(0x6c, 0x98)) | set(range(0x9e, 0xa0)) | set(range(0x114, 0x118))
        if fallbacks:
            normal = struct.unpack_from('<3f', work, 0x108)
            for axis in range(3):
                step = f32(normal[axis]*99999.8984375)
                delta = f32(f32(point[axis]+step)-point[axis])
                inverse = f32(1./delta) if delta else struct.unpack('<f', bytes.fromhex('ffff7f7f'))[0]
                struct.pack_into('<f', expected_work, 0x3c+4*axis, delta)
                struct.pack_into('<f', expected_work, 0x48+4*axis, inverse)
            struct.pack_into('<3f', expected_work, 0x108, *normal)
            struct.pack_into('<I', expected_work, 0x124, fallbacks)
            assert not any(child_disp) and all(0 <= v <= fallbacks for v in child_brush)
            checks['fallback_ray_arithmetic_and_guard_checks'] += 1
        else:
            stale = 17 if visit == 2 else 0
            assert child_brush == [stale]*brush_count and child_disp == [stale]*disp_count
            for a, b in ((0x6c, 0x80), (0x90, 0xa8), (0x114, 0x118)):
                assert work[a:b] == seed[a:b]
            checks['no_fallback_metadata_preservation'] += 1
        assert all(a == b for i, (a, b) in enumerate(zip(expected_work, work)) if i not in allowed), ('point world guards', case)
        checks['complete_child_visit_vectors'] += 1
        if mask == 0:
            assert nb == 0
            expected = bytearray(seed[0x54:0xa8])
            expected_marker = 0xffffffff if mode else 0
            if fallbacks:
                struct.pack_into('<fI', expected, 0x2c, 1., 0)
                expected[0x36:0x38] = b'\0\0'; struct.pack_into('<fI2H', expected, 0x38, 0., 0xffffffff, 0x3344, 0)
                expected_marker = 0
            if not standalone and not visited_non_solid:
                struct.pack_into('<f', expected, 0x2c, 0.); expected[0x36:0x38] = b'\1\1'; struct.pack_into('<f', expected, 0x38, 1.)
            assert work[0x54:0xa8] == expected and struct.unpack_from('<I', work, 0x114)[0] == expected_marker
            expected_marks = set(original_brush_marks)
            for leaf in visited:
                expected_marks.update(brush_lists[leaf])
            assert brush_marks == expected_marks
            checks['zero_mask_full_trace_predictions'] += 1
        if phase:
            # Independent analytic stationary boxes: all three records share
            # [-4,-1]x[-8,8]x[-8,8], with contents1/80/0 and a flagged material.
            predicted_marks = set(original_brush_marks); predicted_calls = []; predicted_terrain = []
            before = ((1., .25, 0.)[mode % 3], .75 if mode&2 else 0., int(bool(mode&8)), int(bool(mode&4)))
            box_hit = all(min((-1., 8., 8.)[a], f32(point[a]+extents[a])) >= max((-4., -8., -8.)[a], f32(point[a]-extents[a])) for a in range(3))
            for ordinal, leaf in enumerate(visited):
                fraction, left, all_solid, start_solid = before; returned = False
                for brush in brush_lists[leaf]:
                    if brush in predicted_marks:
                        continue
                    predicted_marks.add(brush)
                    if not brush_contents[brush] & mask:
                        continue
                    predicted_calls.append(brush)
                    if brush == 1 and mask&0x2000:
                        checks['stationary_surface_exclusion_bypassed'] += 1
                    if box_hit:
                        fraction, left, all_solid, start_solid = 0., 1., 1, 1
                    if fraction == 0.:
                        returned = True; break
                has_terrain = leaf == 1029 if phase == 3 else leaf == 0 or phase == 2 and leaf == 1
                if not returned and not start_solid and has_terrain:
                    predicted_terrain.append(leaf)
                before = ends[ordinal]
            assert brush_marks == predicted_marks and brushes == tuple(predicted_calls) and terrain_leaves == tuple(predicted_terrain), ('point brush/terrain gate', case)
            checks['controlled_brush_and_terrain_gate_predictions'] += 1
        counts['cases'] += 1; counts['leaf_queries'] += standalone; counts['world_queries'] += not standalone
        counts['selected_leaves'] += ns; counts['visited_leaves'] += nv
        counts['solid_leaves_visited'] += sum(bool(leaf_contents[i]&1) for i in visited)
        counts['solid_leaf_finishes'] += not standalone and not visited_non_solid
        counts['normal_fallback_calls'] += fallbacks; counts['multiple_fallback_queries'] += fallbacks > 1
        counts['max_fallbacks_per_query'] = max(counts['max_fallbacks_per_query'], fallbacks)
        counts['brush_calls'] += nb; counts['terrain_dispatch_calls'] += nt
        counts['tiny_extents'] += any(extents); counts['zero_extents'] += not any(extents)
        counts['premarked_queries'] += visit != 0
        counts['unvisited_non_solid_selected'] += not standalone and not visited_non_solid and any(not leaf_contents[i]&1 for i in selected)
        counts['capacity_limited_queries'] += ns == 1024
        if case+1 == BENCHMARK//2:
            halfway = offset
        if case+1 == BENCHMARK:
            assert payload[40:halfway] == payload[halfway:offset], 'point leaf/world LDR/HDR records differ'
    assert offset == len(payload)
    assert phases == [BENCHMARK, CONTROLLED//2, CONTROLLED//2, CAPACITY]
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

    for name in ('probe_stationary_point_world.cpp', 'build_stationary_point_world.cmd', 'verify_stationary_point_world.py',
                 'verify_stationary_point_policy.py', 'verify_stationary_hull_world.py', 'verify_stationary_hull_leaf.py'):
        visit(name)
    return sources


def regressions(root, directory, run, table):
    entries = (
        ('stationary_hull_leaf', '20261128-stationary-leaf/verify-hlovln0i/results.json', {POLICY, LEAF}),
        ('stationary_hull_world', '20261128-stationary-world/verify-nug5kaze/results.json', {POLICY, LEAF, WORLD}),
        ('stationary_hull_world_trace', '20261128-stationary-world-trace/verify-ltm0hdiu/results.json', {POLICY, LEAF, WORLD}),
    )
    result = []
    for stem, relative, expected_changes in entries:
        previous_path = directory.parent/relative; previous = json.loads(previous_path.read_text())
        changed = {name for name, value in previous['source_sha256'].items() if sha(root/name) != value}
        assert changed == expected_changes, ('unexpected shared source change', stem, changed)
        builds = compile_probe(root, run/f'{stem}-regression', stem, run)
        for arch in ('x86', 'x64'):
            for repeat in (1, 2):
                output = run/f'{stem}-{arch}-{repeat}.bin'
                process = subprocess.run([builds[arch]['executable'], str(ENGINE) if arch == 'x86' else '-', str(BSP), str(table), str(output)],
                                         capture_output=True, text=True, timeout=240)
                (run/f'{stem}-{arch}-{repeat}.log').write_text(process.stdout+process.stderr)
                assert process.returncode == 0, (stem, arch, repeat, process.stderr)
                assert sha(output) == previous['record_sha256'], ('non-ray behavior changed', stem, arch, repeat)
                assert json.loads(process.stdout)['cases'] == previous['counts']['cases']
        result.append(dict(component=stem, accepted_receipt=str(previous_path), builds=builds,
                           cases=previous['counts']['cases'], repeats_per_arch=2, warnings=0,
                           changed_source=sorted(changed), record_sha256=previous['record_sha256'], accepted_record_hash_preserved=True))
    return result


def main(directory):
    directory.mkdir(parents=True, exist_ok=True); root = Path(__file__).resolve().parent
    assert not directory.is_relative_to(Path('F:/').resolve()), 'original installation is read-only'
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    frozen = source_closure(root); run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    builds = compile_probe(root, directory, 'stationary_point_world', run)
    image, evidence = verify_installed_ranges(ENGINE, run/'stationary-point-world-reference.local.txt', [
        ('complete stationary world orchestration', 0x10148030, 0x10148156),
        ('complete stationary leaf selection', 0x10145810, 0x101459a4),
        ('complete stationary leaf', 0x10147f40, 0x1014802a),
        ('complete stationary brush primitive', 0x10146c30, 0x10146dd7),
        ('complete stationary displacement dispatch', 0x1014d320, 0x1014d535),
        ('complete stationary fallback', 0x1014ce00, 0x1014cf9c),
        ('complete nested visit setup', 0x10147c30, 0x10147c97),
        ('complete nested depth finish', 0x10145510, 0x1014551e),
        ('complete point/hull router', 0x10149030, 0x10149063),
        ('complete point BSP geometry', 0x10148950, 0x10148c2c),
    ])
    bsp = BSP.read_bytes(); benchmark = Benchmark(bsp)
    baseline = counts = independent = None; observations = []
    for arch in ('x86', 'x64'):
        for repeat in (1, 2):
            output = run/f'{arch}-{repeat}.bin'
            process = subprocess.run([builds[arch]['executable'], str(ENGINE) if arch == 'x86' else '-', str(BSP), str(output)],
                                     capture_output=True, text=True, timeout=240)
            (run/f'{arch}-{repeat}.log').write_text(process.stdout+process.stderr)
            assert process.returncode == 0, (arch, repeat, process.stderr)
            actual = json.loads(process.stdout)
            if arch == 'x86':
                observed = actual.pop('native_point_observations')
                assert observed == actual['normal_fallback_calls']
                observations.append(observed); assert all(value == observed for value in observations)
            payload = output.read_bytes()
            if baseline is None:
                counts, independent = inspect_records(payload, benchmark, bsp); baseline = payload
            assert actual == counts and payload == baseline, (arch, repeat, 'point leaf/world records differ', actual, counts)
    table = run/'nonray-regression-permutation.local.bin'
    at = image.offset(0x104129b0); raw = image.b[at:at+1024]
    assert hashlib.sha256(raw).hexdigest() == PERMUTATION_SHA256; table.write_bytes(raw)
    regression = regressions(root, directory, run, table)
    components = []
    for relative in ('20261128-point-world/verify-1dj7ia0g/results.json',
                     '20261128-stationary-point-policy/verify-t5qdkvzf/results.json'):
        path = directory.parent/relative; accepted = json.loads(path.read_text())
        assert all(sha(root/name) == value for name, value in accepted['source_sha256'].items()), ('accepted point source changed', relative)
        for build in accepted['builds'].values():
            assert sha(Path(build['executable'])) == build['sha256']
        components.append(dict(receipt=str(path), record_sha256=accepted['record_sha256'], source_and_binary_hashes_unchanged=True))
    assert source_closure(root) == frozen, 'source changed during verification'
    editor = json.loads((root.parent/'publication/stage/docs/build-status.json').read_text())
    assert len(editor['editor_source_dependencies']) == 543
    assert all(sha(root.parent/name) == value for name, value in editor['editor_source_dependencies'].items())
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    receipt = dict(status='VERIFIED PREPARED STATIONARY POINT LEAF AND WORLD', counts=counts,
                   independent_checks=independent, builds=builds, repeats_per_arch=2, source_sha256=frozen,
                   record_bytes=len(baseline), record_sha256=hashlib.sha256(baseline).hexdigest(),
                   original_engine_sha256=ENGINE_SHA256, original_bsp_sha256=BSP_SHA256,
                   originals_unchanged=True, editor_dependencies_unchanged=543, replaced_native_callees=[],
                   native_point_observations_per_run=observations, verified_spans=[row['name'] for row in evidence],
                   nonray_regressions=regression, accepted_point_components=components,
                   findings=[
                       'Original full stationary leaf/world/selector/brush/displacement/fallback routines execute with real prepared point geometry and no substituted native callee or storage service.',
                       'Full560work bytes, full parent/child visit vectors, selected/visited leaf IDs, stationary brush call order, terrain gates and per-leaf fraction/left/solid snapshots match. Child visits use lossless canonical run encoding, not hashes or truncated generations.',
                       'Each fallback retains point/nonswept flags, advances native child generation/depth, enters original10148950 and restores end/depth. Multiple fallbacks share real generation storage and preserve earlier child marks.',
                       'Point mode is explicitly threaded through leaf/world bindings with unchanged non-ray defaults. Fresh old non-ray leaf, world and public trace builds repeat both architectures and retain all accepted complete-record hashes.',
                       'Independent complete selections, actual visited-prefix/solidity finish, controlled brush and terrain gates, zero-mask traces, no-fallback metadata, child lifecycle, ray arithmetic and guard predictions pass.'],
                   limits=[
                       'Prepared finite stationary point-classified leaves/world only; public point trace reset/defaults, coordinate finishing, retained underflow delta and empty-world return remain separate gates.',
                       'The native query receives owned-imported/prepared BSP/material/mesh records, not the complete original loader. Material property IDs are explicit distinguishing fixtures.',
                       'The primary point fixture needs no permutation/cache resource or service substitution. Non-ray regressions retain their documented private permutation input and cache/storage adapters.',
                       'Queue overflow/malformed topology, production global lifetime/concurrency, effective material/physics binding, engine/server entities/static props/filters, real spawn and editor integration remain unverified. All543editor dependencies and package are unchanged.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'], counts=counts, independent_checks=independent, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
