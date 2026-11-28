"""Verify full prepared swept-hull public traces and offset coordinate finishing.

Original instructions/table data are read privately from the installed engine.
No extracted bytes or proprietary inputs are embedded in this source.
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


def default_record(property_id=0x3344):
    record = bytearray(88)
    struct.pack_into('<f', record, 0x2c, 1.)
    struct.pack_into('<I', record, 0x3c, 0xffffffff)
    struct.pack_into('<H', record, 0x40, property_id)
    return record


def finish_predictions():
    """Float-rounded coordinate formulas, independent of the C++ implementation."""
    rays = [
        ((-0., -0., 0.), (-0., 0., -0.), (-0., 2., -3.)),
        ((100000000., -100000000., 17.), (-100000000., 100000000., -17.), (1., -3., .25)),
        ((-2., 3., -4.), (.5, -.25, 1.), (5., -6., 7.)),
        ((.03125, -.03125, 2.), (-.0625, .0625, -2.), (.125, .25, -.5)),
    ]
    for variant, (center, offset, delta) in enumerate(rays):
        base = [f32(offset[a]+center[a]) for a in range(3)]
        for fraction, left in itertools.product((0., .25, 1.), repeat=2):
            result = default_record(0)
            end = [f32(delta[a]+base[a]) if fraction == 1. else f32(f32(delta[a]*fraction)+base[a]) for a in range(3)]
            start = [base[a] if left == 0. else f32(f32(delta[a]*left)+base[a]) for a in range(3)]
            if left == 1.:
                result[0x36:0x38] = b'\1\1'
                fraction = 0.
                end = base
            struct.pack_into('<6f', result, 0, *start, *end)
            struct.pack_into('<f', result, 0x2c, fraction)
            struct.pack_into('<f', result, 0x38, left)
            yield variant, bytes(result)


def inspect_records(payload, flags):
    block_bytes = 16+9632*88
    assert len(payload) == 2*block_bytes+16+36*88
    counts = dict(hits=0, start_solid=0, positions_disabled=0, empty_world=0,
                  nonzero_offset=0, rounded_delta_cases=0, nonzero_world_surface=0)
    bodies = []
    blank = default_record()
    for hdr in range(2):
        begin = hdr*block_bytes
        assert struct.unpack_from('<4I', payload, begin) == (hdr, 58 if hdr else 7, 43, 5312)
        body = payload[begin+16:begin+block_bytes]
        bodies.append(body)
        case = 0
        for _, query, mask, variant, compute, available in itertools.product(
                range(43), range(6), (0, 1, 0x2081, 0xffffffff), range(4), (False, True), (True, False)):
            if not available and query != 0:
                continue
            row = body[case*88:(case+1)*88]
            case += 1
            fraction, = struct.unpack_from('<f', row, 0x2c)
            left, = struct.unpack_from('<f', row, 0x38)
            name, = struct.unpack_from('<I', row, 0x3c)
            prop, surface_flags = struct.unpack_from('<2H', row, 0x40)
            disp_flags, = struct.unpack_from('<H', row, 0x34)
            world_surface, = struct.unpack_from('<H', row, 0x4a)
            marker, = struct.unpack_from('<I', row, 0x54)
            assert 0 <= fraction <= 1 and 0 <= left <= 1
            assert row[0x36] in (0, 1) and row[0x37] in (0, 1) and marker in (0, 1)
            assert row[0x44:0x4a] == bytes(6) and row[0x4c:0x54] == bytes(8)
            assert world_surface < len(flags) or world_surface == 65535
            if name == 0xffffffff:
                assert (prop, surface_flags) == (0x3344, 0)
            elif 0x10000000 <= name < 0x10000000+len(flags):
                material = name-0x10000000
                assert (prop, surface_flags) == (1000+material, flags[material])
            else:
                mesh = name-0x20000000
                assert 0 <= mesh < 43 and surface_flags == 0
                assert prop == (500 if disp_flags & 16 else 100)+mesh
            if not available:
                assert row == blank, ('empty world must retain fully initialized trace', hdr, case)
            if not compute:
                assert row[:24] == bytes(24), ('positions disabled', hdr, case)
            if mask == 0:
                assert row[24:] == blank[24:], ('zero mask changed metadata', hdr, case)
            counts['hits'] += fraction < 1
            counts['start_solid'] += row[0x37] != 0
            counts['positions_disabled'] += not compute
            counts['empty_world'] += not available
            counts['nonzero_offset'] += variant != 0
            # All query-4 fixtures retain a nonzero delta lost by endpoint rounding.
            counts['rounded_delta_cases'] += query == 4
            counts['nonzero_world_surface'] += world_surface != 0
        assert case == 9632
    assert bodies[0] == bodies[1], 'LDR/HDR public trace bodies differ'
    begin = 2*block_bytes
    assert struct.unpack_from('<4I', payload, begin) == (2, 0, 36, 0)
    negative_zero_checks = 0
    for case, (variant, expected) in enumerate(finish_predictions()):
        actual = payload[begin+16+case*88:begin+16+(case+1)*88]
        assert actual == expected, ('independent coordinate finish', case)
        if variant == 0:
            assert struct.unpack_from('<I', actual)[0] == 0x80000000
            negative_zero_checks += 1
    assert negative_zero_checks == 9
    return counts


def main(directory, regression_directory):
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    image, evidence = verify_installed_ranges(ENGINE, run/'world-trace-reference.local.txt', [
        ('complete public world trace wrapper', 0x10149eb0, 0x1014a12c),
        ('complete offset coordinate finisher', 0x10145e20, 0x10145f33),
        ('complete base trace copy', 0x10146de0, 0x10146e63),
        ('memset small-buffer path', 0x102de530, 0x102de5aa),
        ('complete swept hull BSP traversal', 0x10148c30, 0x10148f4a),
        ('complete optimized hull brush kernel', 0x10146850, 0x10146c24),
        ('complete general hull brush dispatch/kernel', 0x10147920, 0x10147c27),
        ('complete non-ray leaf routine', 0x10148540, 0x10148867),
        ('complete outer world hull adapter', 0x1014d100, 0x1014d1ec),
        ('complete displacement finish', 0x1014cfa0, 0x1014cfe9),
        ('complete mesh cache build', 0x1015c630, 0x1015c6a1),
        ('complete hull tree/contact dispatch', 0x1015c840, 0x1015cc1f),
        ('complete hull triangle contact', 0x1015abb0, 0x1015afa3),
    ])
    fixture = run/'permutation-reference.local.bin'
    offset = image.offset(0x104129b0)
    data = image.b[offset:offset+1024]
    assert hashlib.sha256(data).hexdigest() == PERMUTATION_SHA256
    fixture.write_bytes(data)
    bsp = BSP.read_bytes()
    texinfo, length = struct.unpack_from('<2I', bsp, 8+6*16)
    texdata_bytes, = struct.unpack_from('<I', bsp, 8+2*16+4)
    flags = [0]*(texdata_bytes//32)
    for i in range(length//72):
        surface_flags, material = struct.unpack_from('<2I', bsp, texinfo+72*i+64)
        flags[material] |= surface_flags & 65535
    assert len(flags) == 87 and length//72 == 2478
    builds = {}
    baseline = counts = independent = None
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = directory/f'swept_hull_world_trace_{arch}.exe'
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
            assert current == dict(cases=19264, finish_cases=36, face_modes=2, meshes_per_mode=43,
                                  triangles_total=10624, planes_total=22724, leaf_refs_total=2186,
                                  materials=87, texinfo=2478, hits=8820, start_solid=2208,
                                  positions_disabled=9632, empty_world=2752, nonzero_offset=14448,
                                  rounded_delta_cases=2752, passed=True)
            payload = record.read_bytes()
            measured = inspect_records(payload, flags)
            for key in measured:
                if key in current:
                    assert current[key] == measured[key], key
            assert measured['nonzero_world_surface'] > 0
            if baseline is None:
                baseline, counts, independent = payload, current, measured
            assert (payload, current, measured) == (baseline, counts, independent), (arch, repeat, 'full public trace differs')
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

    for name in ['probe_swept_hull_world_trace.cpp', 'build_swept_hull_world_trace.cmd', 'verify_swept_hull_world_trace.py']:
        visit(name)
    regression = None
    if regression_directory:
        candidates = sorted(regression_directory.glob('verify-*/results.json'), key=lambda p: p.stat().st_mtime)
        assert candidates, 'Freshly rebuilt prior benchmark regression receipt missing'
        p = candidates[-1]
        regression = json.loads(p.read_text())
        assert regression['record_sha256'] == '61c4c2d30aa56d0194c9cb928f95326a515215a60d4d642fb243f6cb766a7153'
        assert regression['repeats_per_arch'] == 2 and regression['counts']['cases'] == 16512
        for name, value in regression['source_sha256'].items():
            assert sha(root/name) == value, ('regression source changed', name)
        for arch, build in regression['builds'].items():
            assert sha(Path(build['executable'])) == build['sha256']
        regression = dict(receipt=str(p), record_sha256=regression['record_sha256'], cases_per_run=16512,
                          repeats_per_arch=2, source_hashes_match=True)
    receipt = dict(status='VERIFIED PREPARED SWEPT NON-RAY PUBLIC WORLD TRACE', counts=counts,
                   independent_checks=independent, repeats_per_arch=2, builds=builds, source_sha256=sources,
                   record_bytes=len(baseline), record_sha256=hashlib.sha256(baseline).hexdigest(),
                   engine_sha256=ENGINE_SHA256, bsp_sha256=BSP_SHA256, originals_unchanged=True,
                   permutation_sha256=PERMUTATION_SHA256, production_permutation_bundled=False,
                   verified_spans=[row['name'] for row in evidence], previous_benchmark_regression=regression,
                   replaced_services=['work acquire10149230/release1000e020', 'cache build bucket insertion1015b150/plane append10001610',
                                      'prepared cache acquire1015c760/release10261300'],
                   findings=[
                       'Original10149eb0 performs its trace memset/default initialization, actual mixed hull world traversal, actual offset finisher10145e20 and full84-byte copy. Only declared work/cache services are controlled.',
                       'Hardware breakpoint observations execute through geometry, inspect full trace reset and work-ray fields, and retain selected displacement identity without substituting contacts.',
                       'All84public trace bytes plus the internal displacement marker match. Only native surface-name addresses are normalized to semantic identity.',
                       'Native trace+4a retains the last brush-entry runtime surface index across displacement hits/default-surface resets; owned code carries its semantic source separately.',
                       'Both actual benchmark face modes use43meshes/5312triangles, BSP brush tables and actual2478texinfo/87material identities/flags. Material property IDs remain explicit fixtures.',
                       'Centered/asymmetric hulls, coordinate offsets, retained rounded delta, masks, computePositions on/off and unavailable world are exercised.',
                       'Independent float-rounded predictions match36direct finishing cases, including nine negative-zero start-x checks, cancellation of large center/offset, fraction0/.25/1 and leftSolid0/.25/1.',
                       'All four complete architecture record files are byte-identical. Original executable sections/cache geometry and installed engine/BSP hashes remain unchanged.'],
                   limits=[
                       'Only finite, in-range swept non-ray queries for available worlds. Stationary hull dispatch10148030 and point dispatch10148950 are not admitted by this helper.',
                       'Work-pool allocation/lifetime/generation and cache allocation/lifetime/synchronization are supplied services, not verified original production lifecycle.',
                       'Original whole BSP loader orchestration and effective material/physics property binding are not executed. Owned-imported prepared geometry/property fixtures are supplied.',
                       'Private permutation input is supplied to the isolated x64 test; production resource distribution remains unresolved. No original resource or instruction data is bundled.',
                       'This does not prove entities/static props/player filtering, native spawn behavior, original SFM renderer parity or editor integration.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'], counts=counts, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    parser.add_argument('--regression-directory', type=Path)
    args = parser.parse_args()
    main(args.directory.resolve(), args.regression_directory.resolve() if args.regression_directory else None)
