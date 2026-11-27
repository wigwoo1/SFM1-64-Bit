"""Compare hull displacement tree selection with intact installed engine code."""
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
    assert sha(ENGINE) == ENGINE_SHA256
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    image, evidence = verify_installed_ranges(ENGINE, run/'displacement-hull-reference.local.txt', [
        ('complete outer world hull displacement adapter', 0x1014d100, 0x1014d1ec),
        ('complete displacement hull tree routine', 0x1015c840, 0x1015cc1f),
        ('complete direction normalization', 0x10244060, 0x102440cd),
    ])
    for address in [0x10403944, 0x10403948, 0x1040394c]:
        assert struct.unpack_from('<f', image.b, image.offset(address))[0] == .03125
    baseline = counts = None
    builds = {}
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = directory/f'displacement_hull_tree_{arch}.exe'
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
            assert current['passed'] and current['cases'] == 2304
            assert current['triangle_calls'] and current['empty_cases'] and current['changed_cases']
            if counts is None:
                counts = current
            assert current == counts
            payload = records.read_bytes()
            offset = total = 0
            for case in range(2304):
                number, count, fraction, changed, begin, end = struct.unpack_from('<6I', payload, offset)
                assert number == case and count <= 512 and changed <= 1 and begin == end == bool(count)
                offset += 24+24*count
                total += count
            assert offset == len(payload) and total == counts['triangle_calls']
            if baseline is None:
                baseline = payload
            assert payload == baseline, (arch, repeat, 'semantic record mismatch')
    assert sha(ENGINE) == ENGINE_SHA256
    root = Path(__file__).resolve().parent
    receipt = dict(status='VERIFIED DISPLACEMENT HULL TREE SELECTION; CONTACTS NOT IMPLEMENTED',
                   counts=counts, repeats_per_arch=2, builds=builds,
                   source_sha256={name: sha(root/name) for name in ['source_displacement_hull_tree_candidate.h',
                                  'probe_displacement_hull_tree.cpp', 'build_displacement_hull_tree.cmd', 'verify_displacement_hull_tree.py']},
                   semantic_record_bytes=len(baseline), semantic_record_sha256=hashlib.sha256(baseline).hexdigest(),
                   engine_sha256=ENGINE_SHA256, original_unchanged=True,
                   verified_spans=[row['name'] for row in evidence],
                   findings=[
                       'Original1014d100 uses hull Ray_t fields and calls1015c840, whereas point world adapter1014d010 uses the separate point path.',
                       'Full original1015c840 and direction normalization10244060 execute; cache acquire1015c760, triangle contact1015abb0 and cache release10261300 are intercepted by thread-local hardware breakpoints.',
                       'Child bounds expand by hull extents plus0.03125; breadth-first child order and selected triangle pairs match the owned helper.',
                       'Runtime flag4 disables this routine; flag8 alone does not. Triangle callbacks continue after fraction reaches zero; return value compares original and final fraction.',
                       'Powers1..4, zero/small/player hulls, zero/axial/oblique deltas and multiple initial fractions produce identical triangle IDs, normalized vectors, service order and fraction results across four runs.',
                       'Private executable sections and installed engine identity remain unchanged.'],
                   limits=[
                       'Triangle contacts and cache services are supplied adapters. No actual triangle/hull hit or full displacement collision is established.',
                       'Outer1014d100 is statically checked, not executed by this probe. World leaf displacement culling/dedup, material binding and finish remain separate.',
                       'No original allocator/cache, BSP displacement import, stationary dispatch, benchmark spawn or editor integration is verified.',
                       'Finite fixture equality does not prove every floating-point environment or malformed native input.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(counts=counts, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
