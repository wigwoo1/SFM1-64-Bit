"""Compare actual hull/triangle contacts with intact original native routines."""
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
    image, evidence = verify_installed_ranges(ENGINE, run/'hull-contact-reference.local.txt', [
        ('complete hull triangle contact', 0x1015abb0, 0x1015afa3),
        ('complete shared plane clip', 0x10158cb0, 0x10158d70),
        ('complete face plane clip', 0x10158d70, 0x10158e77),
        ('complete projected edge clip YZ', 0x10159520, 0x10159641),
        ('complete projected edge clip XZ', 0x10159650, 0x10159770),
        ('complete projected edge clip XY', 0x10159770, 0x1015988d),
    ])
    assert struct.unpack_from('<f', image.b, image.offset(0x10351c38))[0] == -99999.8984375
    assert struct.unpack_from('<f', image.b, image.offset(0x1031e234))[0] == struct.unpack('<f', struct.pack('<f', .001))[0]
    baseline = counts = None
    builds = {}
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = directory/f'hull_contact_{arch}.exe'
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
            assert current['passed'] and current['cases'] == 17280
            assert current['hits'] and current['non_face_hits'] and current['raw_facing_rejects']
            if counts is None:
                counts = current
            assert counts == current
            payload = records.read_bytes()
            assert len(payload) == current['cases']*0x54
            if baseline is None:
                baseline = payload
            assert payload == baseline, (arch, repeat, 'full trace byte mismatch')
    assert sha(ENGINE) == ENGINE_SHA256
    root = Path(__file__).resolve().parent
    receipt = dict(status='VERIFIED HULL TRIANGLE CONTACT FOR SUPPLIED EDGE CACHES', counts=counts,
                   repeats_per_arch=2, builds=builds,
                   source_sha256={name: sha(root/name) for name in ['source_displacement_hull_triangle_candidate.h',
                                  'probe_displacement_hull_contact.cpp', 'build_displacement_hull_contact.cmd', 'verify_displacement_hull_contact.py']},
                   record_bytes=len(baseline), record_sha256=hashlib.sha256(baseline).hexdigest(),
                   engine_sha256=ENGINE_SHA256, original_unchanged=True, replaced_callees=[],
                   verified_spans=[row['name'] for row in evidence],
                   findings=[
                       'Full original1015abb0 executes its inline axis bounds and all original edge/face/shared clip routines, without replacement callbacks or code patches.',
                       'The facing gate uses raw delta dot triangle normal, not normalized delta; near-parallel cases detect this difference.',
                       'Owned contact preserves native clip order, support expansion, signed cache indices, missing-edge65535 handling, final interval tolerance and closest-hit policy.',
                       'Every byte of the84-byte trace matches across four runs, including retained bytes, fraction/normal/distance and displacement flags.',
                       'Four authored triangle geometries, three cache encodings, three hull sizes, three initial fractions and mixed motion cases include real face and non-face hits.'],
                   corrected_issue='Initial raw-facing gate incorrectly used normalized movement. Native instruction-boundary diagnosis identified it; one focused correction passed the full suite.',
                   limits=[
                       'Vertex data, triangle metadata and valid edge-cache planes/indices are supplied fixtures. Native cache generation, canonicalization and merging are not verified.',
                       'Cache encodings0/1 provide full projected planes; encoding2 deliberately omits one edge per family to exercise native sentinel behavior, not complete triangle coverage.',
                       'No complete displacement tree/contact composition, world adapter, BSP displacement import, stationary behavior, benchmark spawn or editor integration is established.',
                       'Finite fixtures do not establish all degenerate geometry, overflow or floating-point environments.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(counts=counts, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
