"""Compare both hull brush dispatch branches with an intact installed engine."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile

from verify_sliding_door_spawn import ENGINE, BSP, sha, verify_installed_ranges

ENGINE_SHA256 = 'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
BSP_SHA256 = '60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea'


def main(directory):
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    image, evidence = verify_installed_ranges(ENGINE, run/'hull-dispatch-reference.local.txt', [
        ('full optimized hull box kernel', 0x10146850, 0x10146c24),
        ('full hull brush dispatch and general kernel', 0x10147920, 0x10147c27),
        ('original plane copy', 0x100e9720, 0x100e975d),
    ])
    baseline = counts = None
    builds = {}
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = directory/f'dispatch_hull_{arch}.exe'
        log = (directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log, log
        data = exe.read_bytes()
        assert b'requestedExecutionLevel' in data and b'asInvoker' in data, 'Standard-user manifest missing'
        pe = struct.unpack_from('<I', data, 60)[0]
        assert data[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H', data, pe+4)[0] == machine
        builds[arch] = dict(executable=str(exe), sha256=sha(exe), machine=hex(machine), warnings=0)
        for repeat in (1, 2):
            record = run/f'{arch}-{repeat}.bin'
            result = subprocess.run([str(exe), str(ENGINE) if arch == 'x86' else '-', str(BSP), str(record)],
                                    capture_output=True, text=True, timeout=45)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode == 0, (arch, result.returncode, result.stderr)
            current = json.loads(result.stdout)
            assert current['passed'] and current['cases'] == 9504
            assert current['retained_delta_cases'] == 32
            assert current['optimized_cases'] > 0 and current['general_cases'] > 0
            assert current['optimized_cases']+current['general_cases'] == current['cases']
            if counts is None:
                counts = current
            assert counts == current
            payload = record.read_bytes()
            assert len(payload) == current['cases']*60
            if baseline is None:
                baseline = payload
            assert payload == baseline, (arch, repeat, 'semantic record mismatch')
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    root = Path(__file__).resolve().parent
    receipt = dict(status='VERIFIED HULL BRUSH DISPATCH', counts=counts,
                   benchmark_brushes=192, benchmark_cases=3072, synthetic_cases=6400, retained_delta_cases=32,
                   repeats_per_arch=2, builds=builds,
                   source_sha256={name: sha(root/name) for name in ['source_brush_hull_dispatch_candidate.h',
                                  'probe_brush_hull_dispatch.cpp', 'build_brush_hull_dispatch.cmd', 'verify_brush_hull_dispatch.py']},
                   semantic_record_bytes=len(baseline), semantic_record_sha256=hashlib.sha256(baseline).hexdigest(),
                   originals_unchanged=True, engine_sha256=ENGINE_SHA256, bsp_sha256=BSP_SHA256,
                   verified_spans=[row['name'] for row in evidence],
                   findings=[
                       'Full original10147920 dispatch executes, including optimized10146850 and the general convex kernel. Only surface lookup10146440 is supplied.',
                       'The shared owned adapter reuses the existing optimized-box kernel and new general hull kernel; no point-ray behavior changes.',
                       'Checked box records select the optimized route; ordinary side spans select general. Output surface identity retains runtime-surface versus texture-info meaning.',
                       'Fractions, solid flags, contents, entering event, original plane/tag bits, surface selection, hit marker and texture hint match x64.',
                       'Four runs have identical semantic record bytes; private executable sections and installed reference hashes remain unchanged.'],
                   limits=[
                       'Native trace-work start/end/extents/delta/inverse and imported runtime records are supplied fixtures; full original loader/setup is not executed.',
                       'Surface lookup is an adapter. World masks/dedup, displacements, stationary dispatch, static props/entities and filters remain separate.',
                       'No full-world trace, benchmark actor position or editor integration is established. Finite fixtures do not prove every float/x87 mode.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(counts=counts, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
