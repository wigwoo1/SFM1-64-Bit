"""Verify full stationary normal selection and the prepared hull fallback query."""
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


def adjacent(value, up):
    if value == 0.:
        return struct.unpack('<f', struct.pack('<I', 1 if up else 0x80000001))[0]
    bits, = struct.unpack('<I', struct.pack('<f', value))
    return struct.unpack('<f', struct.pack('<I', bits+(1 if up == (value > 0.) else -1)))[0]


def normal_predictions():
    normals = [(-0., 0., 1.), (1., 0., 0.), (0., -1., 0.), (.6, .8, 0.)]
    lists = [[], [0], [1, 0], [3, 2, 1, 0], [0, 1, 0, 2], [2, 2], [3, 0, 3]]
    mins = [(0., 0., 0.), (-2., -3., -4.), (-24., -24., -41.), (-5., 1., -7.), (100., 200., 300.)]
    maxs = [(0., 0., 0.), (2., 3., 4.), (24., 24., 41.), (-2., 9., -3.), (104., 206., 308.)]
    contents = [1, 2, 4, 0x80]
    for q, variant, is_ray, refs, mask in itertools.product(range(32), range(5), (False, True), lists, (0, 1, 2, 0x85, 0xffffffff)):
        i = q % 4
        bounds = [8.*i-4., -2., -1., 8.*i+4., 2., 1.]
        extents = [f32(f32(maxs[variant][a]-mins[variant][a])*.5) for a in range(3)]
        point = [(bounds[0]+bounds[3])*.5, 0., 0.]
        if q//4 == 1:
            point[0] = f32(bounds[3]+extents[0])
        if q//4 == 2:
            point[0] = adjacent(f32(bounds[3]+extents[0]), True)
        if q//4 == 3:
            point[1] = f32(bounds[1]-extents[1])
        if q//4 == 4:
            point[1] = adjacent(f32(bounds[1]-extents[1]), False)
        if q//4 == 5:
            point = [1000., 1000., 1000.]
        if q//4 == 6:
            point = [-4., -2., -1.]
        if q//4 == 7:
            point = [28., 2., 1.]
        normal, selected_contents = (91., -0., 93.), 0xdeadbeef
        selected = -2
        if refs:
            normal, selected_contents = normals[refs[0]], 1
            selected = -1
            for index in refs:
                if not contents[index] & mask:
                    continue
                b = [8.*index-4., -2., -1., 8.*index+4., 2., 1.]
                expansion = [0.]*3 if is_ray else extents
                if all(f32(b[a]-expansion[a]) <= point[a] <= f32(b[a+3]+expansion[a]) for a in range(3)):
                    normal, selected_contents = normals[index], contents[index]
                    selected = index
                    break
        yield struct.pack('<3fI', *normal, selected_contents), selected


def inspect_records(payload, material_flags):
    assert struct.unpack_from('<4I', payload) == (0x53484e31, 11200, 16512, 0x230)
    assert len(payload) == 16+11200*16+16512*0x230
    matched = defaulted = empty = 0
    for case, (prediction, selected) in enumerate(normal_predictions()):
        actual = payload[16+16*case:16+16*(case+1)]
        assert actual == prediction, ('independent normal selection', case)
        matched += selected >= 0
        defaulted += selected == -1
        empty += selected == -2
    begin = 16+11200*16
    body = payload[begin:]
    assert body[:8256*0x230] == body[8256*0x230:], 'LDR/HDR fallback work images differ'
    counts = dict(normal_cases=11200, fallback_cases=0, hits=0, start_solid=0,
                  wrapped_generations=0, zero_direction=0, zero_mask=0, retained_contents=0, passed=True)
    for case, (hdr, mesh, q, hull, direction, mode, mask, wrap) in enumerate(itertools.product(
            range(2), range(43), range(2), range(2), range(4), range(2), (0, 0x2081, 0xffffffff), (False, True))):
        work = body[case*0x230:(case+1)*0x230]
        extents = (24., 24., 41.) if hull else (2., 3., 5.)
        assert work[12:24] == struct.pack('<3f', -91., -92., -93.), 'previous work end not restored'
        assert work[0x18:0x24] == struct.pack('<3f', *(-v for v in extents))
        assert work[0x24:0x30] == work[0x30:0x3c] == struct.pack('<3f', *extents)
        assert struct.unpack_from('<I', work, 0xfc)[0] == mask and work[0x100:0x102] == b'\0\0'
        for offset in (0x104, 0x128, 0x13c, 0x150, 0x164):
            assert work[offset:offset+4] == bytes(4), 'private pointers not normalized'
        assert struct.unpack_from('<3I', work, 0x11c) == (0, 17, 1), 'nested depth/generation restoration'
        assert work[0x54:0x6c] == struct.pack('<6f', 11., 12., 13., 14., 15., 16.)
        assert struct.unpack_from('<I', work, 0x98)[0] == 0x11112222
        assert struct.unpack_from('<H', work, 0x9c)[0] == 0x3344
        assert struct.unpack_from('<2I', work, 0xa0) == (0x55556666, 0x77778888)
        point = struct.unpack_from('<3f', work)
        normal = struct.unpack_from('<3f', work, 0x108)
        if direction == 2:
            assert work[0x108:0x114] == struct.pack('<3f', 0., -0., 0.)
        if direction == 3:
            assert work[0x108:0x114] == struct.pack('<3f', .6, .8, .25)
        delta = [f32(f32(point[a]+f32(normal[a]*99999.8984375))-point[a]) for a in range(3)]
        inverse = [f32(1./v) if v else struct.unpack('<f', struct.pack('<I', 0x7f7fffff))[0] for v in delta]
        assert work[0x3c:0x48] == struct.pack('<3f', *delta), ('independent fallback retained delta', case)
        assert work[0x48:0x54] == struct.pack('<3f', *inverse)
        fraction, = struct.unpack_from('<f', work, 0x80)
        contents, = struct.unpack_from('<I', work, 0x84)
        left, = struct.unpack_from('<f', work, 0x8c)
        name, = struct.unpack_from('<I', work, 0x90)
        prop, flags = struct.unpack_from('<2H', work, 0x94)
        world_surface, = struct.unpack_from('<H', work, 0x9e)
        marker, = struct.unpack_from('<I', work, 0x114)
        assert 0 <= fraction <= 1 and 0 <= left <= 1
        assert work[0x8a] in (0, 1) and work[0x8b] in (0, 1) and marker in (0, 1)
        assert world_surface < 87 or world_surface == 65535
        if name == 0xffffffff:
            assert (prop, flags) == (0x3344, 0)
        elif 0x10000000 <= name < 0x10000000+87:
            material = name-0x10000000
            assert (prop, flags) == (1000+material, material_flags[material])
        else:
            index = name-0x20000000
            assert 0 <= index < 43 and prop in (100+index, 500+index) and flags == 0
        if mask == 0:
            assert fraction == 1 and left == 0 and work[0x8a:0x8c] == b'\0\0'
            assert contents == 0x12345678 and name == 0xffffffff and marker == 0
            assert work[0x6c:0x7c] == struct.pack('<4f', 9., 8., 7., 6.)
            assert work[0x7c:0x80] == bytes((5, 4, 3, 2))
            assert struct.unpack_from('<H', work, 0x88)[0] == 0xbeef
            assert world_surface == (77 if mode else 23)
        if wrap:
            assert work == body[(case-1)*0x230:case*0x230], 'wrap and fresh generation must produce identical canonical work'
        counts['fallback_cases'] += 1
        counts['hits'] += fraction < 1
        counts['start_solid'] += work[0x8b] != 0
        counts['wrapped_generations'] += wrap
        counts['zero_direction'] += direction == 2
        counts['zero_mask'] += mask == 0
        counts['retained_contents'] += contents == 0x12345678
    assert matched and defaulted and empty and matched+defaulted+empty == 11200
    return counts, dict(normal_predictions=11200, selected_normals=matched,
                        first_reference_default_normals=defaulted, empty_lists_preserved=empty,
                        fallback_delta_inverse_predictions=16512, identical_ldr_hdr_work_images=8256,
                        matching_fresh_wrapped_generation_pairs=8256)


def main(directory):
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    image, evidence = verify_installed_ranges(ENGINE, run/'stationary-fallback-reference.local.txt', [
        ('complete normal selection', 0x1014cd40, 0x1014cdf5),
        ('complete point/hull normal bounds predicate', 0x10158e80, 0x10158f4a),
        ('complete inclusive point bounds test', 0x1014e580, 0x1014e5d0),
        ('complete normal fallback wrapper', 0x1014ce00, 0x1014cf9c),
        ('complete nested visit generation setup', 0x10147c30, 0x10147c97),
        ('complete nested depth finish', 0x10145510, 0x1014551e),
        ('complete point/hull geometry router', 0x10149030, 0x10149063),
        ('complete mixed hull BSP geometry', 0x10148c30, 0x10148f4a),
        ('complete non-ray leaf geometry', 0x10148540, 0x10148867),
        ('complete outer displacement hull adapter', 0x1014d100, 0x1014d1ec),
    ])
    scale = struct.unpack_from('<f', image.b, image.offset(0x10350ef4))[0]
    assert scale == 99999.8984375
    assert struct.unpack_from('<f', image.b, image.offset(0x1031cb50))[0] == .5
    offset = image.offset(0x104129b0)
    table = image.b[offset:offset+1024]
    assert hashlib.sha256(table).hexdigest() == PERMUTATION_SHA256
    fixture = run/'permutation-reference.local.bin'
    fixture.write_bytes(table)
    bsp = BSP.read_bytes()
    texinfo, length = struct.unpack_from('<2I', bsp, 8+6*16)
    material_count = struct.unpack_from('<I', bsp, 8+2*16+4)[0]//32
    flags = [0]*material_count
    for index in range(length//72):
        value, material = struct.unpack_from('<2I', bsp, texinfo+72*index+64)
        flags[material] |= value & 65535
    assert len(flags) == 87 and length//72 == 2478
    baseline = counts = independent = None
    builds = {}
    native_counts = []
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = directory/f'stationary_hull_fallback_{arch}.exe'
        log = (directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log, log
        binary = exe.read_bytes()
        pe, = struct.unpack_from('<I', binary, 60)
        assert binary[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H', binary, pe+4)[0] == machine
        assert struct.unpack_from('<H', binary, pe+24)[0] == (0x10b if arch == 'x86' else 0x20b)
        builds[arch] = dict(executable=str(exe), sha256=sha(exe), machine=hex(machine), warnings=0)
        for repeat in (1, 2):
            record = run/f'{arch}-{repeat}.bin'
            result = subprocess.run([str(exe), str(ENGINE) if arch == 'x86' else '-', str(BSP), str(fixture), str(record)],
                                    capture_output=True, text=True, timeout=45)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode == 0, (arch, repeat, result.returncode, result.stderr)
            current = json.loads(result.stdout)
            acquires = current.pop('native_cache_acquires')
            assert acquires == (11384 if arch == 'x86' else 0)
            if arch == 'x86':
                native_counts.append(acquires)
            assert current == dict(normal_cases=11200, fallback_cases=16512, hits=8688, start_solid=1792,
                                  wrapped_generations=8256, zero_direction=4128, zero_mask=5504,
                                  retained_contents=7808, passed=True)
            payload = record.read_bytes()
            if baseline is None:
                counts, independent = inspect_records(payload, flags)
                assert counts == current, (counts, current)
                baseline = payload
            assert payload == baseline and current == counts, (arch, repeat, 'normal/full-work fallback mismatch')
    regressions = []
    for subdir, record_sha in [
            ('mixed-regression', 'f779bfda44cf46a1247fe2a52491993f155aedcedf219de65ecf8633f4fc2832'),
            ('benchmark-regression', '61c4c2d30aa56d0194c9cb928f95326a515215a60d4d642fb243f6cb766a7153'),
            ('swept-regression', '35ca5376f7362f4bfe909a7636ec250fcab31596b1dfb284080ebb5b6e6c4c5d')]:
        candidates = sorted((directory/subdir).glob('verify-*/results.json'), key=lambda p: p.stat().st_mtime)
        assert candidates, ('fresh regression receipt missing', subdir)
        p = candidates[-1]
        regression = json.loads(p.read_text())
        assert regression['record_sha256'] == record_sha and regression['repeats_per_arch'] == 2
        for name, value in regression['source_sha256'].items():
            assert sha(Path(__file__).resolve().parent/name) == value, ('regression source mismatch', name)
        for build in regression['builds'].values():
            assert sha(Path(build['executable'])) == build['sha256']
        regressions.append(dict(receipt=str(p), record_sha256=record_sha, counts=regression['counts']))
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

    for name in ['probe_stationary_hull_fallback.cpp', 'build_stationary_hull_fallback.cmd', 'verify_stationary_hull_fallback.py']:
        visit(name)
    receipt = dict(status='VERIFIED STATIONARY HULL NORMAL SELECTION AND PREPARED FALLBACK', counts=counts,
                   independent_checks=independent, repeats_per_arch=2, builds=builds, source_sha256=sources,
                   record_bytes=len(baseline), record_sha256=hashlib.sha256(baseline).hexdigest(),
                   original_engine_sha256=ENGINE_SHA256, original_bsp_sha256=BSP_SHA256, originals_unchanged=True,
                   permutation_sha256=PERMUTATION_SHA256, production_permutation_bundled=False,
                   native_cache_acquires_per_run=native_counts, regressions=regressions,
                   verified_spans=[row['name'] for row in evidence],
                   replaced_services=['cache build bucket insertion1015b150/plane append10001610',
                                      'prepared cache acquire1015c760/release10261300'],
                   findings=[
                       'Full original1014cd40 ->10158e80 ->1014e580 executes point/hull normal selection with no replaced callee. Output guards and all input work/object bytes are preserved.',
                       'Hull expansion depends on (relativeMax-relativeMin)*.5, not midpoint. Empty lists preserve outputs; nonempty lists default to first normal/contents1; first eligible containing bounds wins.',
                       'Full original1014ce00 executes its reset, temporary ray arithmetic, original nested generation setup10147c30, router10149030, real mixed hull geometry10148c30, original nested finish10145510 and prior-end restoration.',
                       'Only cache storage/acquire/release are fixture services. Geometry, work setup, generation rollover memset and depth finish are not substituted.',
                       'A minimal shared-helper option carries native swept visit flag. Default true preserves existing behavior; nested stationary normal fallback passes false, matching untouched native work+101.',
                       'All560working-buffer bytes match after pointer/surface-name normalization. Parent visit arrays/generation survive; child wrap clears stale brush/displacement visits; nonswept displacement marks stay zero.',
                       'Independent predictions match11200normal outputs and16512delta/inverse calculations. Both8256-query benchmark face blocks and fresh/wrapped generation pairs have identical canonical work bytes.',
                       'Shared-helper synthetic mixed, benchmark geometry and full public swept-wrapper tests were freshly rebuilt/repeated and retain prior accepted record hashes.',
                       'Four architecture records are identical. Native cached geometry/executable sections and installed engine/BSP hashes remain unchanged.'],
                   limits=[
                       'Normal selection uses four explicit synthetic metadata records; fallback uses actual43-mesh benchmark geometry per face mode with distinguishing material-property fixtures.',
                       'Original whole BSP loading, effective material/physics binding and cache allocation/lifetime/synchronization are not executed as production services.',
                       'Prepared nested visit storage is supplied. Rollover and depth transitions execute here, but global pool allocation, arbitrary recursion capacity and concurrency remain separate.',
                       'Fallback comparison covers the preserved non-ray/nonswept path at world root0 with symmetric relative hull bounds, not every engine/router path.',
                       'Private permutation data is explicit test input; production resource distribution remains unresolved for hull sweep caches.',
                       'Complete stationary displacement policy remapping/direct-overlap composition, stationary leaf/world/public wrapper, server entities/static props/filters, actual spawn and editor integration remain unverified.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'], counts=counts, independent_checks=independent, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
