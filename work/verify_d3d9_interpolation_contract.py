"""Owned calibration, frozen raster hypothesis, then independent GPU holdouts.

This does not change production coordinates or replace the old continuous oracle.
The n.8 nearest-even hypothesis is motivated by the public D3D11 specification;
it is NOT claimed to be a public D3D9 guarantee. Calibration must pass BEFORE any
old discrepant capture is evaluated against it. No fitted quantum/tolerance.
"""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import math
import os
import random
import re
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/d3d9-interpolation-contract-20261101'
VS = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat')
DUMP = VS.parents[2] / 'Tools/MSVC/14.51.36231/bin/Hostx64/x64/dumpbin.exe'
TOLERANCE = 2e-6  # Unchanged from the original analytic gate.
SUBPIXEL_SCALE = 256  # Predeclared hypothesis, not inferred from old errors.


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def need(value, message):
    if not value:
        raise RuntimeError(message)


def put(p, value):
    Path(p).write_text(json.dumps(value, indent=2) + '\n')


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def vertex(x, y, w, attribute):
    return {'clip': [f32((x/16-1)*w), f32((1-y/16)*w), f32(w/2), f32(w)],
            'attribute': list(attribute)}


def encode(rows, path):
    result = bytearray(struct.pack('<I', len(rows)))
    for r in rows:
        for v in r['vertices']:
            result.extend(struct.pack('<8f', *v['clip'], *v['attribute']))
        result.extend(struct.pack('<I', len(r['samples'])))
        for point in r['samples']:
            result.extend(struct.pack('<2I', *point))
    path.write_bytes(result)


def decode(rows, path):
    raw = path.read_bytes()
    at, result = 0, []
    for serial, r in enumerate(rows):
        n = len(r['samples'])
        need(struct.unpack_from('<2I', raw, at) == (serial, n), 'capture framing')
        at += 8
        result.append(struct.unpack_from('<' + 'f'*4*n, raw, at))
        at += n*16
    need(at == len(raw), 'complete capture')
    return result


def expected(vertices, x, y, snapped=True):
    # Exact integer edge/coverage arithmetic after snapping; double reciprocal-W
    # interpolation. The D3D9 integer pixel center is intentional.
    coords = []
    for v in vertices:
        px, py, _, w = v['clip']
        sx, sy = (px/w+1)*16, (1-py/w)*16
        coords.append((round(sx*SUBPIXEL_SCALE), round(sy*SUBPIXEL_SCALE))
                      if snapped else (sx, sy))
    p = (x*SUBPIXEL_SCALE, y*SUBPIXEL_SCALE) if snapped else (x, y)

    def edge(a, b, c):
        return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])

    area = edge(*coords)
    need(area != 0, 'nondegenerate discriminator')
    if area < 0:
        coords = [coords[0], coords[2], coords[1]]
        vertices = [vertices[0], vertices[2], vertices[1]]
        area = -area
    weights = [edge(coords[1], coords[2], p), edge(coords[2], coords[0], p),
               edge(coords[0], coords[1], p)]
    for a, b in zip(coords, coords[1:]+coords[:1]):
        e = edge(a, b, p)
        top_left = b[1] < a[1] or (b[1] == a[1] and b[0] > a[0])
        if e < 0 or (e == 0 and not top_left):
            return [0.]*4
    weights = [l/area/v['clip'][3] for l, v in zip(weights, vertices)]
    total = sum(weights)
    return [sum(l*v['attribute'][k] for l, v in zip(weights, vertices))/total
            for k in range(4)]


def compare(rows, captures, snapped=True):
    result = {'pixels': 0, 'components': 0, 'over_budget': 0, 'max_error': 0.,
              'coverage_mismatches': 0, 'boundary_pixels_excluded': 0}
    for row, values in zip(rows, captures):
        for j, (x, y) in enumerate(row['samples']):
            actual = values[4*j:4*j+4]
            want = expected(row['vertices'], x, y, snapped)
            need(all(math.isfinite(a) for a in actual), 'finite capture')
            result['pixels'] += 1
            result['coverage_mismatches'] += (actual[3] == 0) != (want[3] == 0)
            for a, b in zip(actual, want):
                error = abs(a-b)
                result['components'] += 1
                result['over_budget'] += error > TOLERANCE
                result['max_error'] = max(result['max_error'], error)
    return result


def main():
    before = json.loads((OUT/'before.json').read_text())
    parent = before['parent']
    need(sha(parent['receipt']) == parent['receipt_sha256'], 'parent receipt')
    prior = json.loads(Path(parent['receipt']).read_text())
    sources = dict(before['source_sha256'])
    protected = dict(before['protected_sha256'])
    protected[parent['receipt']] = parent['receipt_sha256']
    for group in (sources, protected, before['complete_editor_dependencies_sha256']):
        for p, h in group.items():
            need(sha(p) == h, 'before identity ' + p)
    new_files = [ROOT/'work/probe_d3d9_interpolation_grid_x64.cpp', Path(__file__).resolve()]
    for p in new_files:
        need(str(p) not in sources, 'new files only; no production transition')
        sources[str(p)] = sha(p)
    run = Path(tempfile.mkdtemp(prefix='r-', dir=OUT))
    print(str(run), flush=True)
    protocol = {'time_utc': datetime.now(timezone.utc).isoformat(),
                'hypothesis': 'screen XY nearest-even n.8; integer D3D9 pixel centers; reciprocal-W interpolation',
                'scope': 'measured adapter/driver only; no D3D9 universal precision guarantee',
                'subpixel_scale': SUBPIXEL_SCALE, 'tolerance': TOLERANCE,
                'calibration': 'independent affine translations along X/Y, 1025 steps each, spacing 1/16384 pixel',
                'holdouts': '64 deterministic unclipped varying-W triangles plus all unchanged original98 diagnostic cases',
                'no_production_changes': True, 'source_sha256': sources}
    put(run/'premeasurement-protocol.json', protocol)
    exe = run/'interpolation-grid.exe'
    batch = run/'build.cmd'
    batch.write_text('\n'.join([
        '@echo off', f'cd /d "{ROOT}"', f'call "{VS}" >nul',
        'if errorlevel 1 exit /b 1',
        f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\probe_d3d9_interpolation_grid_x64.cpp /Fo:"{run/"grid.obj"}" /Fe:"{exe}" d3d9.lib d3dcompiler.lib user32.lib >"{run/"build.log"}" 2>&1',
        'exit /b %errorlevel%', '']))
    clean = {k: v for k, v in os.environ.items() if not k.startswith('SFM64_')}

    def execute(args, label):
        p = subprocess.run(list(map(str, args)), cwd=ROOT, env=clean,
                           capture_output=True, text=True, timeout=300,
                           creationflags=subprocess.CREATE_NO_WINDOW)
        (run/(label+'.log')).write_text(p.stdout+p.stderr)
        need(p.returncode == 0, label+' failed: '+(p.stdout+p.stderr)[-1800:])
        return p.stdout+p.stderr

    execute(['cmd.exe', '/d', '/c', batch], 'build-driver')
    binary = exe.read_bytes()
    pe = struct.unpack_from('<I', binary, 60)[0]
    need(struct.unpack_from('<H', binary, pe+4)[0] == 0x8664 and
         struct.unpack_from('<H', binary, pe+24)[0] == 0x20b, 'AMD64 PE32+')
    warnings = re.findall(r'warning (C\d+): ([^\r\n]+)', (run/'build.log').read_text())
    print('Fresh AMD64 discriminator built, warnings='+str(len(warnings)), flush=True)
    basis = [[1., 0., 0., 1.], [0., 1., 0., 1.], [0., 0., 1., 1.]]
    samples = [[10, 10], [12, 8], [8, 12]]
    calibration = []
    for axis in (0, 1):
        for step in range(-512, 513):
            delta = step/16384
            points = [[4., 4.], [28., 4.], [4., 28.]]
            for point in points:
                point[axis] += delta
            calibration.append({'axis': axis, 'step': step, 'samples': samples,
                                'vertices': [vertex(x, y, 1., a) for (x, y), a in zip(points, basis)]})
    put(run/'calibration-cases.json', calibration)
    query = run/'calibration.bin'
    encode(calibration, query)
    for n in range(2):
        execute([exe, query, run/f'c{n}'], f'c{n}')
        print('Calibration GPU repeat '+str(n)+' complete', flush=True)
    need((run/'c0/pixels.bin').read_bytes() == (run/'c1/pixels.bin').read_bytes(), 'calibration repeatability')
    captured = decode(calibration, run/'c0/pixels.bin')
    calibration_metrics = compare(calibration, captured)
    plateaus = []
    for axis in (0, 1):
        values = captured[axis*1025:(axis+1)*1025]
        transitions = [j-512 for j in range(1, 1025) if values[j] != values[j-1]]
        plateaus.append({'axis': axis, 'distinct_output_plateaus': len(transitions)+1,
                         'transition_steps': transitions})
    frozen = {'time_utc': datetime.now(timezone.utc).isoformat(),
              'premeasurement_protocol_sha256': sha(run/'premeasurement-protocol.json'),
              'calibration': calibration_metrics, 'plateaus': plateaus,
              'scale': SUBPIXEL_SCALE, 'rounding': 'nearest-even', 'tolerance': TOLERANCE}
    put(run/'calibration-result.json', frozen)
    need(calibration_metrics['over_budget'] == 0 and calibration_metrics['coverage_mismatches'] == 0,
         'predeclared raster hypothesis failed independent calibration; preserve and stop')
    need(all(p['distinct_output_plateaus'] == 17 for p in plateaus), 'unexpected independent plateau count; stop')
    print('Independent calibration accepted; raster model frozen before holdouts', flush=True)
    rng = random.Random(0x53464D64)
    holdouts = []
    for serial in range(64):
        points = [[3+rng.randrange(1024)/1024, 4+rng.randrange(1024)/1024],
                  [27+rng.randrange(1024)/1024, 5+rng.randrange(1024)/1024],
                  [6+rng.randrange(1024)/1024, 27+rng.randrange(1024)/1024]]
        weights = [1.25, 2.5, 4.75] if serial % 2 else [1., 1., 1.]
        vertices = [vertex(x, y, w, a) for (x, y), w, a in zip(points, weights, basis)]
        if serial % 3 == 0:
            vertices.reverse()  # Both windings; no culling in the discriminator.
        holdouts.append({'vertices': vertices, 'samples': [[x, y] for y in range(32) for x in range(32)]})
    put(run/'holdout-cases.json', holdouts)
    query = run/'holdouts.bin'
    encode(holdouts, query)
    for n in range(2):
        execute([exe, query, run/f'h{n}'], f'h{n}')
    need((run/'h0/pixels.bin').read_bytes() == (run/'h1/pixels.bin').read_bytes(), 'holdout repeatability')
    captured = decode(holdouts, run/'h0/pixels.bin')
    holdout_metrics = compare(holdouts, captured)
    continuous_holdouts = compare(holdouts, captured, False)
    put(run/'holdout-result.json', {'snapped': holdout_metrics, 'continuous': continuous_holdouts,
                                  'frozen_calibration_sha256': sha(run/'calibration-result.json')})
    need(holdout_metrics['over_budget'] == 0 and holdout_metrics['coverage_mismatches'] == 0,
         'frozen model fails new holdouts; preserve and stop')
    # Only now inspect retained discrepancy cases. Recompile the unchanged source
    # and execute its full operational/native-reference checks twice as well.
    previous = json.loads(Path(prior['verification']['receipt']).read_text())
    diag = json.loads(Path(previous['diagnostic_receipt']['receipt']).read_text())
    old_run = Path(diag['executable']['path']).parent
    old_rows = json.loads((old_run/'cases.json').read_text())
    diagnostic_exe = run/'shading.exe'
    batch = run/'build-shading.cmd'
    batch.write_text('\n'.join([
        '@echo off', f'cd /d "{ROOT}"', f'call "{VS}" >nul', 'if errorlevel 1 exit /b 1',
        f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\probe_d3d9_shading_consumption_x64.cpp /Fo:"{run/"shading.obj"}" /Fe:"{diagnostic_exe}" d3d9.lib d3dcompiler.lib user32.lib >"{run/"build-shading.log"}" 2>&1',
        'exit /b %errorlevel%', '']))
    execute(['cmd.exe', '/d', '/c', batch], 'build-shading-driver')
    logs = []
    for n in range(2):
        logs.append(execute([diagnostic_exe, old_run/'q.bin', old_run/'owned.vtf', old_run/'owned.bsp', run/f'd{n}'], f'd{n}'))
    raw = (run/'d0/pixels.bin').read_bytes()
    need(raw == (run/'d1/pixels.bin').read_bytes() == (old_run/'d0/pixels.bin').read_bytes(), 'all unchanged98 diagnostic pixels retained')
    need(all(raw == (run/f'd{n}/reference.bin').read_bytes() for n in range(2)), 'native reference exact twice')
    old_snapped, old_continuous = [], []
    at = 0
    for serial, r in enumerate(old_rows):
        need(struct.unpack_from('<2I', raw, at) == (serial, r['mode']), 'old capture framing')
        at += 8
        values = struct.unpack_from('<4096f', raw, at)
        at += 16384
        # Two disjoint triangles. Build a separate CPU clip calculation rounded
        # at each shader operation; this fixture uses only exact dyadic products.
        triangles = []
        for model in (0, 2):
            vertices = []
            for v in r['models'][model]:
                p = v['position']+[1.]
                clip = [f32(sum(f32(r['camera'][4*j+k]*p[k]) for k in range(4))) for j in range(4)]
                clip[0] = f32(clip[0]-f32(clip[3]/32))
                clip[1] = f32(-clip[1]+f32(clip[3]/32))
                attribute = v['normal']+[1.] if r['mode'] == 1 else v['tangent']
                vertices.append({'clip': clip, 'attribute': attribute})
            triangles.append(vertices)
        for snapped, metrics in ((True, old_snapped), (False, old_continuous)):
            row = {'pixels': 0, 'components': 0, 'over_budget': 0, 'max_error': 0., 'coverage_mismatches': 0}
            for y in range(32):
                for x in range(32):
                    outputs = [expected(t, x, y, snapped) for t in triangles]
                    hits = [a for a in outputs if a[3] != 0]
                    need(len(hits) <= 1, 'disjoint original triangles')
                    want = hits[0] if hits else [0.]*4
                    actual = values[4*(y*32+x):4*(y*32+x)+4]
                    row['pixels'] += 1
                    row['coverage_mismatches'] += (actual[3] == 0) != (want[3] == 0)
                    for a, b in zip(actual, want):
                        error = abs(a-b)
                        row['components'] += 1
                        row['over_budget'] += error > TOLERANCE
                        row['max_error'] = max(row['max_error'], error)
            metrics.append(row)
    need(at == len(raw), 'all original capture bytes')

    def total(rows):
        return {k: (max(r[k] for r in rows) if k == 'max_error' else sum(r[k] for r in rows)) for k in rows[0]}

    old_metrics, ideal = total(old_snapped), total(old_continuous)
    put(run/'original98-result.json', {'snapped': old_metrics, 'continuous_original_gate_still_fails': ideal,
                                     'per_case_snapped': old_snapped, 'per_case_continuous': old_continuous,
                                     'frozen_calibration_sha256': sha(run/'calibration-result.json')})
    need(old_metrics['over_budget'] == 0 and old_metrics['coverage_mismatches'] == 0, 'frozen model fails original holdouts')
    need(ideal['over_budget'] == 4510 and abs(ideal['max_error']-diag['metrics']['max_error']) < 1e-12,
         'original continuous discrepancy retained exactly')
    for group in (sources, protected, before['complete_editor_dependencies_sha256']):
        for p, h in group.items():
            need(sha(p) == h, 'final preserved identity '+p)
    need(sha(run/'calibration-result.json') == json.loads((run/'holdout-result.json').read_text())['frozen_calibration_sha256'], 'frozen model unchanged')
    result = {'status': 'VERIFIED measured interpolation contract; continuous ideal gate remains failed, not waived',
              'time_utc': datetime.now(timezone.utc).isoformat(), 'previous_goal_turn': 'PROGRESS',
              'parent': parent, 'source_sha256': sources, 'protected_sha256': protected,
              'calibration_cases': len(calibration), 'calibration': calibration_metrics,
              'plateaus': plateaus, 'new_holdout_cases': len(holdouts), 'new_holdouts': holdout_metrics,
              'continuous_new_holdouts': continuous_holdouts, 'original_cases': len(old_rows),
              'original98_snapped': old_metrics, 'original98_continuous': ideal,
              'repetitions': 2, 'tolerance': TOLERANCE, 'boundary_pixels_excluded': 0,
              'grid_executable': {'path': str(exe), 'sha256': sha(exe)},
              'shading_executable': {'path': str(diagnostic_exe), 'sha256': sha(diagnostic_exe)},
              'grid_build_warnings': warnings,
              'shading_build_warnings': re.findall(r'warning (C\d+): ([^\r\n]+)', (run/'build-shading.log').read_text()),
              'shading_logs': logs, 'editor_rebuilt': False, 'new_editor_frames': 0,
              'complete_editor_preserved': prior['complete_editor_preserved'],
              'complete_editor_dependencies_sha256': before['complete_editor_dependencies_sha256'],
              'canonical_preserved_sha256': prior['canonical_preserved_sha256'],
              'limits': ['Measured NVIDIA HAL/driver only, unclipped triangles/float32 target.',
                         'D3D11 n.8 rule is not a universal D3D9 guarantee; independent measurement establishes this local model.',
                         'Continuous ideal2e-6 gate still fails4510 components; a separate explicit native raster contract now tested.',
                         'No production shader/coordinate/default/editor change, no Source-lit image equivalence.',
                         'Default730 and CPU/editor checks are preserved prior evidence, not rerun in this test-only task.'],
              'full_goal': 'active/incomplete; independent reconstruction'}
    receipt = run/'results.json'
    put(receipt, result)
    put(OUT/'latest-verification.json', {'receipt': str(receipt), 'receipt_sha256': sha(receipt)})
    print(json.dumps({k: result[k] for k in ('status','calibration','new_holdouts','original98_snapped','original98_continuous','grid_executable','shading_executable')}, indent=2), flush=True)


if __name__ == '__main__':
    main()
