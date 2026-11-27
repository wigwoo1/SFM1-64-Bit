"""Compare the general convex brush hull kernel with intact original x86 code."""
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
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256, 'Reference identity changed'
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    image, evidence = verify_installed_ranges(ENGINE, run/'convex-hull-reference.local.txt', [
        ('leaf hull brush call and solid-stop gate', 0x101486bd, 0x101486f9),
        ('full optimized/general hull brush dispatch and general kernel', 0x10147920, 0x10147c27),
        ('original plane copy', 0x100e9720, 0x100e975d),
    ])
    assert struct.unpack_from('<f', image.b, image.offset(0x1033fa60))[0] == .03125
    assert struct.unpack_from('<f', image.b, image.offset(0x103423d4))[0] == -99999
    baseline = counts = None
    builds = {}
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = directory/f'convex_hull_{arch}.exe'
        log = (directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log, log
        data = exe.read_bytes()
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
            assert current['passed'] and current['cases'] == 9488
            assert all(current[k] > 0 for k in ['entering', 'start_solid', 'all_solid', 'bevel_hits', 'left_solid_entering', 'new_start_solid'])
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
    receipt = dict(status='VERIFIED GENERAL CONVEX BRUSH HULL KERNEL', counts=counts,
                   benchmark_brushes=192, benchmark_cases=3072, synthetic_cases=6400, empty_brush_cases=16,
                   repeats_per_arch=2, builds=builds,
                   source_sha256={name: sha(root/name) for name in ['source_convex_brush_hull_candidate.h',
                                  'probe_convex_brush_hull.cpp', 'build_convex_brush_hull.cmd', 'verify_convex_brush_hull.py']},
                   semantic_record_bytes=len(baseline), semantic_record_sha256=hashlib.sha256(baseline).hexdigest(),
                   originals_unchanged=True, engine_sha256=ENGINE_SHA256, bsp_sha256=BSP_SHA256,
                   verified_spans=[row['name'] for row in evidence],
                   findings=[
                       'Full original general kernel10147920 executes, including its original plane copy100e9720; only surface lookup10146440 is supplied.',
                       'Hull expansion includes every side, including bevel planes, using support extents added to plane distance.',
                       'Unlike the ray kernel, the hull kernel does not merge an entering hit with prior leftSolid merely because leftSolid exceeds entry.',
                       'Fractions, start/all-solid flags, contents, entering side, retained/copied original plane and tags, surface selection, hit marker and texture hint match x64.',
                       'Four runs have identical semantic record bytes. Original private executable sections and installed reference hashes remain unchanged.'],
                   limits=[
                       'Benchmark brush geometry is exercised through the general branch deliberately, without proving actual optimized/general route selection.',
                       'Optimized boxes, masks/dedup, world traversal composition, displacements, stationary query dispatch, static props/entities and filters are outside this test.',
                       'Surface lookup is an adapter, not actual engine material binding. No live benchmark actor position or full-world collision is established.',
                       'Finite fixture agreement is not a proof over all float/x87 rounding modes; helper remains isolated from editor.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(counts=counts, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
