"""Compare stationary hull brush overlap with the intact installed x86 primitive."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile

from verify_sliding_door_spawn import ENGINE, BSP, sha, verify_installed_ranges
from verify_convex_brush_hull import ENGINE_SHA256, BSP_SHA256

MINIMUM = [(0., 0., 0.), (-1., -3., -5.), (-24., -24., -41.), (0., 0., 0.), (-5., 1., -7.), (-0., -0., 0.)]
MAXIMUM = [(0., 0., 0.), (1., 3., 5.), (24., 24., 41.), (1., 5., 82.), (-2., 9., -3.), (0., 0., 0.)]


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def adjacent(value, up):
    if value == 0.:
        return struct.unpack('<f', struct.pack('<I', 1 if up else 0x80000001))[0]
    bits, = struct.unpack('<I', struct.pack('<f', value))
    bits += 1 if up == (value > 0.) else -1
    return struct.unpack('<f', struct.pack('<I', bits))[0]


def seed(mode):
    row = bytearray(0x40+i for i in range(84))
    struct.pack_into('<4f', row, 0x18, 9., 8., 7., 6.)
    row[0x28:0x2c] = bytes((5, 4, 3, 2))
    struct.pack_into('<fI', row, 0x2c, (1., .25, 0.)[mode % 3], 0x12345678)
    row[0x36] = bool(mode & 8)
    row[0x37] = bool(mode & 4)
    struct.pack_into('<f', row, 0x38, .75 if mode & 2 else 0.)
    return row


def overlap(point, low, high, variant):
    # Independent interval-separation test, with the original float endpoints.
    hull = [(f32(point[a]+MINIMUM[variant][a]), f32(point[a]+MAXIMUM[variant][a])) for a in range(3)]
    return not any(hull[a][1] < low[a] or hull[a][0] > high[a] for a in range(3))


def brush_inventory(data):
    def lump(index):
        return struct.unpack_from('<2I', data, 8+16*index)
    plane_offset, plane_bytes = lump(1)
    planes = [struct.unpack_from('<4fI', data, plane_offset+20*i) for i in range(plane_bytes//20)]
    side_offset, _ = lump(19)
    brush_offset, brush_bytes = lump(18)
    inventory = []
    for index in range(brush_bytes//12):
        first, count, contents = struct.unpack_from('<3I', data, brush_offset+12*index)
        selected = [planes[struct.unpack_from('<H', data, side_offset+8*(first+i))[0]] for i in range(count)]
        optimized = count == 6 and all(p[4] <= 2 for p in selected)
        lows, highs = [None]*3, [None]*3
        bevel = any(struct.unpack_from('<h', data, side_offset+8*(first+i)+6)[0] for i in range(count))
        for p in selected:
            for a in range(3):
                if any(p[k] != 0. for k in range(3) if k != a):
                    continue
                if p[a] == 1.:
                    highs[a] = p[3]
                if p[a] == -1.:
                    lows[a] = -p[3]
        if optimized:
            assert all(v is not None for v in lows+highs)
        for a in range(3):
            if lows[a] is None or highs[a] is None:
                lows[a], highs[a] = -32., 32.
        center = [f32(f32(lows[a]+highs[a])*.5) for a in range(3)]
        inventory.append((contents, optimized, bevel, lows, highs, center))
    assert len(inventory) == 1866
    return inventory


def metadata(inventory):
    for contents, optimized, bevel, low, high, center in inventory:
        for forced in (False, True):
            for variant in range(6):
                for query in range(8):
                    point = center.copy()
                    if query == 1:
                        point[0] = f32(low[0]-MAXIMUM[variant][0])
                    if query == 2:
                        point[0] = adjacent(f32(low[0]-MAXIMUM[variant][0]), False)
                    if query == 3:
                        point[0] = f32(high[0]-MINIMUM[variant][0])
                    if query == 4:
                        point[0] = adjacent(f32(high[0]-MINIMUM[variant][0]), True)
                    if query == 5:
                        point[2] = f32(low[2]-MAXIMUM[variant][2])
                    if query == 6:
                        point[2] = adjacent(f32(high[2]-MINIMUM[variant][2]), True)
                    if query == 7:
                        point = [f32(v+256.) for v in high]
                    box = optimized and not forced
                    prediction = overlap(point, low, high, variant) if box else None
                    for mode in (0, 3, 6, 15):
                        yield contents, box, bevel and not box, variant, mode, 1 <= query <= 6, prediction
    random = 0x372619ab

    def sample():
        nonlocal random
        random = (random*1664525+1013904223) & 0xffffffff
        return (((random >> 8) % 8193)-4096)/64.

    for shape in range(4):
        for query in range(192):
            point = [sample(), sample(), sample()]
            if query % 8 == 0:
                point = [0., -0., 0.]
            if query % 8 == 1:
                point = [16., 0., 0.]
            if query % 8 == 2:
                point = [-16., 0., 0.]
            if query % 8 == 3:
                point = [adjacent(16., True), 0., 0.]
            for variant in range(6):
                box = shape == 0
                prediction = overlap(point, [-16.]*3, [16.]*3, variant) if box else None
                for mode in range(16):
                    yield 1 << shape, box, not box, variant, mode, 1 <= query % 8 <= 3, prediction
    for variant in range(6):
        for mode in range(16):
            yield 1, False, False, variant, mode, False, False


def inspect_records(payload, inventory):
    assert len(payload) == 790368*84
    totals = dict(cases=0, actual_brushes=1866, hits=0, misses=0, optimized=0, general=0,
                  nonzero_hulls=0, asymmetric_hulls=0, boundary_hits=0, bevel_hits=0, passed=True)
    predicted = 0
    seeds = [seed(mode) for mode in range(16)]
    for case, (contents, box, bevel, variant, mode, boundary, prediction) in enumerate(metadata(inventory)):
        row = payload[84*case:84*(case+1)]
        actual_contents, = struct.unpack_from('<I', row, 0x30)
        assert contents != 0x12345678 and actual_contents in (contents, 0x12345678)
        hit = actual_contents == contents
        if prediction is not None:
            assert hit == prediction, ('independent box/empty prediction', case)
            predicted += 1
        expected = seeds[mode].copy()
        if hit:
            struct.pack_into('<fI', expected, 0x2c, 0., contents)
            expected[0x36:0x38] = b'\1\1'
            struct.pack_into('<f', expected, 0x38, 1.)
        assert row == expected, ('stationary primitive modified unrelated trace bytes', case)
        totals['cases'] += 1
        totals['hits'] += hit
        totals['misses'] += not hit
        totals['optimized'] += box
        totals['general'] += not box
        totals['nonzero_hulls'] += 0 < variant < 5
        totals['asymmetric_hulls'] += variant in (3, 4)
        totals['boundary_hits'] += boundary and hit
        totals['bevel_hits'] += bevel and hit
    assert totals['cases'] == 790368 and predicted == totals['optimized']+96
    return totals, predicted


def main(directory):
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    _, evidence = verify_installed_ranges(ENGINE, run/'stationary-hull-reference.local.txt', [
        ('complete stationary brush primitive', 0x10146c30, 0x10146dd7),
        ('complete stationary leaf dispatcher', 0x10147f40, 0x1014802a),
        ('complete stationary world dispatcher', 0x10148030, 0x10148156),
        ('complete stationary displacement dispatcher', 0x1014d320, 0x1014d535),
    ])
    inventory = brush_inventory(BSP.read_bytes())
    baseline = counts = None
    builds = {}
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = directory/f'stationary_brush_hull_{arch}.exe'
        log = (directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log, log
        binary = exe.read_bytes()
        pe, = struct.unpack_from('<I', binary, 60)
        assert binary[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H', binary, pe+4)[0] == machine
        assert struct.unpack_from('<H', binary, pe+24)[0] == (0x10b if arch == 'x86' else 0x20b)
        builds[arch] = dict(executable=str(exe), sha256=sha(exe), machine=hex(machine), warnings=0)
        for repeat in (1, 2):
            target = run/f'{arch}-{repeat}.bin'
            result = subprocess.run([str(exe), str(ENGINE) if arch == 'x86' else '-', str(BSP), str(target)],
                                    capture_output=True, text=True, timeout=45)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode == 0, (arch, repeat, result.returncode, result.stderr)
            current = json.loads(result.stdout)
            assert current == dict(cases=790368, actual_brushes=1866, hits=350156, misses=440212,
                                  optimized=254976, general=535392, nonzero_hulls=526912,
                                  asymmetric_hulls=263456, boundary_hits=251500, bevel_hits=106032, passed=True)
            payload = target.read_bytes()
            if baseline is None:
                measured, predictions = inspect_records(payload, inventory)
                assert measured == current, (measured, current)
                baseline, counts = payload, current
            assert payload == baseline and current == counts, (arch, repeat, 'stationary hull results differ')
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

    for name in ['probe_stationary_brush_hull.cpp', 'build_stationary_brush_hull.cmd', 'verify_stationary_brush_hull.py']:
        visit(name)
    receipt = dict(status='VERIFIED STATIONARY NONZERO-HULL BRUSH PRIMITIVE', counts=counts,
                   independent_box_and_empty_predictions=predictions, repeats_per_arch=2, builds=builds,
                   source_sha256=sources, record_bytes=len(baseline), record_sha256=hashlib.sha256(baseline).hexdigest(),
                   originals_unchanged=True, engine_sha256=ENGINE_SHA256, bsp_sha256=BSP_SHA256,
                   executed_original_spans=['complete stationary brush primitive10146c30..10146dd7'],
                   statically_inspected_spans=[r['name'] for r in evidence], replaced_callees=[],
                   findings=[
                       'Full original10146c30 executes without any adapted callee, code patch, import resolution or DLL initialization. Private executable sections remain unchanged.',
                       'All1866benchmark brushes use both their classified route and general plane route; controlled axial/oblique/bevel/empty brushes extend the corpus.',
                       'Optimized route tests closed interval overlap of center+relative bounds; the general route selects the minimum-dot hull corner and subtracts its offset dot product from plane distance before separation.',
                       'Native float operation order is preserved. Closed-face contact and adjacent outside floats, non-centered relative bounds and signed zeros are covered.',
                       'All84trace bytes and the entire304-byte native work buffer match expected changes. Only fractions/solid flags/contents change on overlap; plane/tags/material/hit metadata and miss state remain unchanged.',
                       'Independent interval predictions validate254976optimized-box records plus96empty-brush records. Every record independently checks allowed writes and aggregate coverage.',
                       'Two runs of each freshly built architecture produce identical records, with zero compiler warnings. Installed engine/BSP hashes remain unchanged.'],
                   limits=[
                       'This is the brush primitive, not stationary world dispatch or full collision. Mask filtering and brush dedup occur in its caller and are not added here.',
                       'Leaf selection/fallback, displacement overlap1015a7d0 and normal/ray fallback paths are statically identified but not executed by this probe.',
                       'Benchmark imported runtime tables are supplied; original whole BSP loading, material binding and engine/server filtering remain separate.',
                       'Finite ordered relative bounds and finite arithmetic are admitted. This does not prove all NaN/infinite inputs or alternate FP modes.',
                       'No editor promotion, stationary actor spawn, static props/entities or full SFM compatibility is established.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'], counts=counts, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
