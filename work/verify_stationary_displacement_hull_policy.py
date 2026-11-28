"""Compare complete prepared stationary displacement dispatch with native x86.

Original instructions, map geometry and permutation data remain external private
inputs. The controlled two-leaf fixture exercises real fallback solid geometry.
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


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def order_hash(ids):
    result = 2166136261
    for index in ids:
        result = ((result ^ index) * 16777619) & 0xffffffff
    return result


def queries(controlled):
    if controlled:
        for q, hull, li, mask, mode, visit, config, disabled in itertools.product(
                range(5), range(3), range(4), (0, 0x2081, 0xffffffff), range(2), range(3), range(3), range(2)):
            refs = ((1, 0), (0, 1), (1, 1, 0), (1, 0, 1))[li]
            yield q, hull, refs, mask, mode, visit, config, disabled, 0, 1
    else:
        for _, index, q, hull, li, mask, mode, visit, config in itertools.product(
                range(2), range(43), range(3), range(3), range(3), (0, 0x2081, 0xffffffff), range(2), range(3), range(3)):
            nxt = (index + 1) % 43
            refs = ((index,), (index, index, nxt), (nxt, index, nxt))[li]
            yield q, hull, refs, mask, mode, visit, config, 0, index, nxt


def inspect_records(payload, benchmark_contents, flags):
    assert struct.unpack_from('<8I', payload) == (0x53504432, 125388, 6480, 560, 43, 2, 756, 592)
    assert len(payload) == 32 + 125388 * 756 + 6480 * 592
    assert payload[32:32+62694*756] == payload[32+62694*756:32+125388*756], 'LDR/HDR dispatcher records differ'
    counts = dict(cases=0, benchmark_cases=125388, controlled_cases=6480, fallback_solid=0,
                  direct_overlaps=0, fallbacks=0, start_solid=0, cull_tests=0, mesh_overlap_tests=0,
                  zero_bounds=0, zero_masks=0, premarked_queries=0, distinct_cull_mesh_contents_hits=0,
                  public_binding_checks=4254, passed=True)
    independent = dict(visit_transactions=0, direct_metadata_preserved=0, delta_inverse_predictions=0,
                       controlled_normal_cull_order_predictions=0, known_positive_fallback=0,
                       identical_ldr_hdr_queries=62694)
    offset = 32
    for controlled in (False, True):
        contents = [1, 2] if controlled else benchmark_contents
        size = len(contents)
        for q, hull, refs, mask, mode, visit, config, disabled, index, nxt in queries(controlled):
            work = payload[offset:offset+560]
            visits = struct.unpack_from('<' + str(size) + 'I', payload, offset+560)
            direct, fallback, cull_count, overlap_count, cull_hash, overlap_hash = struct.unpack_from('<6I', payload, offset+560+size*4)
            offset += 584 + size*4
            assert direct == 0xffffffff or direct < size
            assert fallback in (0, 1) and bool(fallback) == (direct == 0xffffffff)
            extent = ((0., 0., 0.), (1., 1., 1.) if controlled else (2., 3., 5.), (2., 3., 5.) if controlled else (24., 24., 41.))[hull]
            point = struct.unpack_from('<3f', work)
            assert work[0x18:0x24] == struct.pack('<3f', *(-v for v in extent))
            assert work[0x24:0x30] == struct.pack('<3f', *extent) == work[0x30:0x3c]
            assert work[12:24] == struct.pack('<3f', -91., -92., -93.)
            assert work[0x54:0x6c] == struct.pack('<6f', 11., 12., 13., 14., 15., 16.)
            assert struct.unpack_from('<I', work, 0xfc)[0] == mask and work[0x100:0x102] == b'\0\0'
            assert struct.unpack_from('<2I', work, 0x11c) == (0, 17)
            assert struct.unpack_from('<I', work, 0x124)[0] == (1 if fallback else 0xffffffff if visit == 2 else 0)
            assert struct.unpack_from('<I', work, 0x148)[0] == (1 if controlled else 1866)
            assert struct.unpack_from('<I', work, 0x170)[0] == size
            for slot in (0x104, 0x128, 0x13c, 0x150, 0x164):
                assert work[slot:slot+4] == bytes(4)
            assert work[0x98:0x9e] == struct.pack('<IH', 0x11112222, 0x3344)
            assert work[0xa0:0xa8] == struct.pack('<2I', 0x55556666, 0x77778888)
            visit_indices = [i // 2 if config == 1 else i for i in range(size)]
            cull_contents = list(contents)
            if config == 2:
                cull_contents[index] = 2
            expected_visits = [0] * size
            if visit:
                expected_visits[visit_indices[index if visit == 1 else nxt]] = 17
            culls = []
            if hull:
                for mesh in refs:
                    slot = visit_indices[mesh]
                    if not (cull_contents[mesh] & mask) or expected_visits[slot] == 17:
                        continue
                    expected_visits[slot] = 17
                    culls.append(mesh)
                    if mesh == direct:
                        break
            assert tuple(expected_visits) == visits
            assert (len(culls), order_hash(culls)) == (cull_count, cull_hash)
            assert overlap_count <= cull_count
            if direct != 0xffffffff:
                assert culls and culls[-1] == direct and overlap_count
            fraction = struct.unpack_from('<f', work, 0x80)[0]
            result_contents = struct.unpack_from('<I', work, 0x84)[0]
            left = struct.unpack_from('<f', work, 0x8c)[0]
            name = struct.unpack_from('<I', work, 0x90)[0]
            prop, surface_flags = struct.unpack_from('<2H', work, 0x94)
            marker = struct.unpack_from('<I', work, 0x114)[0]
            assert fraction in (0., 1.) and left == 0.
            assert work[0x8a] == work[0x8b] == int(fraction == 0.)
            if not fallback:
                assert fraction == 0 and result_contents == contents[direct]
                assert work[0x6c:0x80] == struct.pack('<4f4B', 9., 8., 7., 6., 5, 4, 3, 2)
                assert struct.unpack_from('<H', work, 0x88)[0] == 0xbeef
                assert (name, prop, surface_flags, marker) == (0x10000000+77, 0xabcd, 0xef01, 0xffffffff if mode else 0)
                assert struct.unpack_from('<H', work, 0x9e)[0] == (77 if mode else 23)
                for start, end in ((0x3c, 0x54), (0x108, 0x114)):
                    assert work[start:end] == b'\xa7' * (end-start)
                independent['direct_metadata_preserved'] += 1
            else:
                assert marker in (0, 1)
                if fraction == 0:
                    assert marker == 1
                else:
                    assert result_contents == 0
                if name == 0xffffffff:
                    assert (prop, surface_flags) == (0x3344, 0)
                elif 0x10000000 <= name < 0x10000000+87:
                    material = name-0x10000000
                    assert (prop, surface_flags) == (1000+material, flags[material])
                else:
                    mesh = name-0x20000000
                    assert 0 <= mesh < size and prop in (100+mesh, 500+mesh) and surface_flags == 0
                normal = struct.unpack_from('<3f', work, 0x108)
                delta = [f32(f32(point[a]+f32(normal[a]*99999.8984375))-point[a]) for a in range(3)]
                inverse = [f32(1./v) if v else struct.unpack('<f', bytes.fromhex('ffff7f7f'))[0] for v in delta]
                assert work[0x3c:0x54] == struct.pack('<6f', *delta, *inverse)
                independent['delta_inverse_predictions'] += 1
            if controlled:
                assert point == ((-3., -2., -6., 4., 0.)[q], 0., 0.)
                bounds = ((3., -9., -9., 5., 9., 9.), (49., -9., -9., 51., 9., 9.))
                admitted = [m for m in culls if all(point[a]-extent[a] <= bounds[m][a+3] and bounds[m][a] <= point[a]+extent[a] for a in range(3))]
                assert (len(admitted), order_hash(admitted)) == (overlap_count, overlap_hash)
                if disabled:
                    assert direct != 0
                if fallback:
                    selected = refs[0]
                    for m in refs:
                        if contents[m] & mask and all(bounds[m][a]-extent[a] <= point[a] <= bounds[m][a+3]+extent[a] for a in range(3)):
                            selected = m
                            break
                    assert normal == ((-1., 0., 0.) if selected == 0 else (1., 0., 0.))
                if q == 0 and hull == 1 and refs == (1, 0) and mask == 0x2081 and mode == visit == config == disabled == 0:
                    assert fallback and fraction == 0 and marker == 1 and result_contents == 1
                    independent['known_positive_fallback'] += 1
                independent['controlled_normal_cull_order_predictions'] += 1
            counts['cases'] += 1
            counts['fallback_solid'] += bool(fallback and fraction == 0)
            counts['direct_overlaps'] += not fallback
            counts['fallbacks'] += fallback
            counts['start_solid'] += fraction == 0
            counts['cull_tests'] += cull_count
            counts['mesh_overlap_tests'] += overlap_count
            counts['zero_bounds'] += hull == 0
            counts['zero_masks'] += mask == 0
            counts['premarked_queries'] += visit != 0
            counts['distinct_cull_mesh_contents_hits'] += direct != 0xffffffff and cull_contents[direct] != contents[direct]
            independent['visit_transactions'] += 1
    assert offset == len(payload) and independent['known_positive_fallback'] == 1
    assert counts['fallback_solid'] == 810
    return counts, independent


def main(directory):
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    image, evidence = verify_installed_ranges(ENGINE, run/'stationary-policy-reference.local.txt', [
        ('complete stationary displacement dispatcher', 0x1014d320, 0x1014d535),
        ('complete inclusive bounds overlap', 0x1014e5d0, 0x1014e628),
        ('complete mesh box overlap with triangle tail', 0x1015a7d0, 0x1015abab),
        ('complete triangle box overlap', 0x1014f770, 0x1014fdb2),
        ('complete normal selection', 0x1014cd40, 0x1014cdf5),
        ('complete normal bounds predicate', 0x10158e80, 0x10158f4a),
        ('complete fallback wrapper', 0x1014ce00, 0x1014cf9c),
        ('complete nested visit setup', 0x10147c30, 0x10147c97),
        ('complete nested finish', 0x10145510, 0x1014551e),
        ('complete point hull router', 0x10149030, 0x10149063),
        ('complete mixed world hull geometry', 0x10148c30, 0x10148f4a),
        ('complete non-ray leaf geometry', 0x10148540, 0x10148867),
    ])
    data = image.b[image.offset(0x104129b0):image.offset(0x104129b0)+1024]
    assert hashlib.sha256(data).hexdigest() == PERMUTATION_SHA256
    fixture = run/'permutation-reference.local.bin'
    fixture.write_bytes(data)
    bsp = BSP.read_bytes()
    disp, length = struct.unpack_from('<2I', bsp, 8+26*16)
    assert length == 43*176
    contents = [struct.unpack_from('<I', bsp, disp+176*i+32)[0] for i in range(43)]
    texinfo, length = struct.unpack_from('<2I', bsp, 8+6*16)
    flags = [0] * (struct.unpack_from('<I', bsp, 8+2*16+4)[0]//32)
    for i in range(length//72):
        value, material = struct.unpack_from('<2I', bsp, texinfo+72*i+64)
        flags[material] |= value & 65535
    baseline = counts = independent = None
    builds = {}
    native_counts = []
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = directory/f'stationary_displacement_hull_policy_{arch}.exe'
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
            assert acquires == (38684 if arch == 'x86' else 0)
            if arch == 'x86':
                native_counts.append(acquires)
            payload = record.read_bytes()
            if baseline is None:
                counts, independent = inspect_records(payload, contents, flags)
                baseline = payload
            assert current == counts and payload == baseline, (arch, repeat, 'full dispatcher records differ', current, counts)
    root = Path(__file__).resolve().parent
    sources = {}

    def visit(name):
        if name in sources:
            return
        p = root/name
        sources[name] = sha(p)
        for include in re.findall(r'^\s*#include\s*"([^"\r\n]+)"', p.read_text(), re.M):
            visit(include)

    for name in ['probe_stationary_displacement_hull_policy.cpp', 'build_stationary_displacement_hull_policy.cmd', 'verify_stationary_displacement_hull_policy.py']:
        visit(name)
    components = []
    for relative in ['20261127-stationary-hull-dispatch/verify-okibhnpw/results.json',
                     '20261127-displacement-box-overlap/verify-hyf12ded/results.json',
                     '20261127-stationary-hull-normal/verify-u4yqwfip/results.json']:
        p = directory.parent/relative
        accepted = json.loads(p.read_text())
        assert accepted['repeats_per_arch'] == 2
        for name, value in accepted['source_sha256'].items():
            assert sha(root/name) == value, ('accepted component source changed', name)
        for build in accepted['builds'].values():
            assert sha(Path(build['executable'])) == build['sha256']
        components.append(dict(receipt=str(p), source_hashes_unchanged=True, record_sha256=accepted['record_sha256']))
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    editor = json.loads((root.parent/'publication/stage/docs/build-status.json').read_text())
    assert len(editor['editor_source_dependencies']) == 543
    for name, value in editor['editor_source_dependencies'].items():
        assert sha(root.parent/name) == value, ('editor dependency changed', name)
    receipt = dict(status='VERIFIED PREPARED STATIONARY DISPLACEMENT DISPATCHER', counts=counts,
                   independent_checks=independent, repeats_per_arch=2, builds=builds, source_sha256=sources,
                   record_bytes=len(baseline), record_sha256=hashlib.sha256(baseline).hexdigest(),
                   original_engine_sha256=ENGINE_SHA256, original_bsp_sha256=BSP_SHA256, originals_unchanged=True,
                   permutation_sha256=PERMUTATION_SHA256, production_permutation_bundled=False,
                   native_cache_acquires_per_run=native_counts, accepted_component_sources=components,
                   editor_dependencies_unchanged=543, verified_spans=[row['name'] for row in evidence],
                   replaced_services=['cache build bucket insertion1015b150/plane append10001610', 'prepared cache acquire1015c760/release10261300'],
                   findings=[
                       'Full original1014d320 executes inclusive culling, actual tree/triangle overlap, normal selection and actual mixed-world fallback. Geometry is not substituted.',
                       'All560working-buffer bytes, parent visits and full cull/overlap identity vectors match before records are written. Return paths are observed with hardware breakpoints, executing through code.',
                       'Contents filtering precedes visit testing/writing. Direct hits use mesh contents, preserve plane/material metadata and do not execute fallback.',
                       'Controlled two-leaf geometry produces810real positive-solid fallbacks. Default-first and containing normal selection, duplicate/aliased visits, disabled mesh flags, zero masks/bounds and rollover are exercised.',
                       'Independent parent visit and cull-call predictions cover all queries; controlled normal/AABB/overlap order predictions and one exact positive fallback fixture are checked.',
                       'Both architecture executables repeated twice produce identical records. LDR/HDR benchmark records match; original images and cache geometry remain unchanged.'],
                   limits=[
                       'Prepared non-ray dispatcher with nonempty displacement lists and finite centered hull bounds only; original leaf caller supplies that domain.',
                       'Full stationary leaf/world/public trace dispatch, actual server spawn, entities/static props/filters and editor integration remain unverified.',
                       'Original whole BSP loader and effective material/physics properties are not executed; tables and distinguishing property IDs are supplied fixtures.',
                       'Cache storage/acquire/release and nested visit storage are supplied; production lifecycle, synchronization and permutation resource distribution remain separate.',
                       'No editor dependency or executable changed; this does not establish a visible application improvement or whole-SFM compatibility.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'], counts=counts, independent_checks=independent, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
