"""Verify swept hull BSP leaf traversal against intact installed engine code."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile

from verify_sliding_door_spawn import ENGINE, sha, verify_installed_ranges


ENGINE_SHA256 = 'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'

def main(directory):
    assert sha(ENGINE) == ENGINE_SHA256, 'Unrecognized installed engine'
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    image, evidence = verify_installed_ranges(ENGINE, run/'engine-hull-walk.local.txt', [
        ('TraceRay world dispatch prefix', 0x1016ece0, 0x1016ee0f),
        ('world query swept dispatch', 0x1014a079, 0x1014a0c5),
        ('full swept hull BSP recursion', 0x10148c30, 0x10148f4a),
    ])
    assert image.word(0x1035456c+0x14) == 0x1016ece0
    assert struct.unpack_from('<f', image.b, image.offset(0x1033fa60))[0] == .03125
    builds = {}
    baseline = None
    counts = None
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = directory/f'hull_walk_{arch}.exe'
        log = (directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log, log
        data = exe.read_bytes()
        pe = struct.unpack_from('<I', data, 60)[0]
        assert data[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H', data, pe+4)[0] == machine
        builds[arch] = dict(executable=str(exe), sha256=sha(exe), machine=hex(machine), warnings=0)
        for repeat in (1, 2):
            records = run/f'{arch}-{repeat}.bin'
            result = subprocess.run([str(exe), str(ENGINE) if arch == 'x86' else '-', str(records)],
                                    capture_output=True, text=True, timeout=45)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode == 0, (arch, result.returncode, result.stderr)
            current = json.loads(result.stdout)
            assert current['passed'] and current['cases'] == 7680
            assert current['overlap_cases'] and current['pruned_cases']
            if counts is None:
                counts = current
            assert current == counts
            payload = records.read_bytes()
            offset = visits = 0
            for case in range(7680):
                number, count, hit = struct.unpack_from('<3I', payload, offset)
                assert number == case and count <= 8
                offset += 12+12*count
                visits += count
            assert offset == len(payload) and visits == counts['visits']
            if baseline is None:
                baseline = payload
            assert payload == baseline, (arch, repeat, 'semantic record mismatch')
    assert sha(ENGINE) == ENGINE_SHA256
    receipt = dict(status='VERIFIED BOUNDED SWEPT HULL BSP TRAVERSAL', counts=counts,
                   repeats_per_arch=2, builds=builds,
                   source_sha256={name: sha(Path(__file__).resolve().parent/name) for name in
                                  ['source_bsp_hull_walk_candidate.h', 'probe_bsp_hull_walk.cpp',
                                   'build_bsp_hull_walk.cmd', 'verify_bsp_hull_walk.py']},
                   semantic_record_bytes=len(baseline),
                   semantic_record_sha256=hashlib.sha256(baseline).hexdigest(),
                   engine_unchanged=True, engine_sha256=sha(ENGINE),
                   verified_spans=[row['name'] for row in evidence],
                   findings=[
                       'EngineTraceServer vtable+14 identifies TraceRay1016ece0, whose world branch calls10149eb0.',
                       'World query dispatch uses10148030 for stationary queries,10148950 for swept rays and10148c30 for swept non-ray hulls.',
                       'Full original10148c30 recursion runs; only leaf contact10148540 is intercepted by a thread-local hardware breakpoint.',
                       'Leaf indices, visit order, float interval bits and changing hit fraction match native x64 across axial/oblique planes, overlapping leaves, shared subtrees, equal endpoints, varied extents and pruning.',
                       'Original private executable sections remain unchanged; fixture nodes and planes are synthetic, with no DLL initialization/import loading.'],
                   limits=[
                       'Leaf brush/displacement contacts are supplied callbacks; this is not complete hull collision or a measured benchmark spawn.',
                       'Stationary hull dispatch, trace setup/finish, entities, static props, filter bodies and live actor state remain unverified here.',
                       'Double split/extent intermediates match these finite fixtures, not a proof for every x87 rounding mode or every finite float.',
                       'The helper is not integrated into the editor. Existing point-ray APIs and their behavior are unchanged.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(counts=counts, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
