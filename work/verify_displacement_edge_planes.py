"""Verify native displacement edge-plane geometry before cache lookup/dedup."""
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
    _, evidence = verify_installed_ranges(ENGINE, run/'edge-planes-reference.local.txt', [
        ('complete per-triangle cache geometry caller', 0x1015c3b0, 0x1015c563),
        ('complete YZ edge plane builder', 0x1015bae0, 0x1015bbe1),
        ('complete XZ edge plane builder', 0x1015bbf0, 0x1015bcee),
        ('complete XY edge plane builder', 0x1015bcf0, 0x1015bdee),
    ])
    baseline = counts = None
    builds = {}
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = directory/f'edge_planes_{arch}.exe'
        log = (directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log, log
        data = exe.read_bytes()
        pe = struct.unpack_from('<I', data, 60)[0]
        assert data[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H', data, pe+4)[0] == machine
        builds[arch] = dict(executable=str(exe), sha256=sha(exe), machine=hex(machine), warnings=0)
        for repeat in (1, 2):
            records = run/f'{arch}-{repeat}.bin'
            result = subprocess.run([str(exe), str(ENGINE) if arch == 'x86' else '-', str(records)], capture_output=True, text=True, timeout=45)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode == 0, (arch, result.returncode, result.stderr)
            current = json.loads(result.stdout)
            assert current['passed'] and current['cases'] == 3072 and current['planes'] and current['skipped']
            if counts is None:
                counts = current
            assert current == counts
            payload = records.read_bytes()
            offset = total = skipped = 0
            for _ in range(3072):
                count, = struct.unpack_from('<I', payload, offset)
                indices = struct.unpack_from('<9H', payload, offset+4)
                assert count <= 9
                skipped += indices.count(65535)
                offset += 22+12*count
                total += count
            assert offset == len(payload) and total == counts['planes'] and skipped == counts['skipped']
            if baseline is None:
                baseline = payload
            assert payload == baseline, (arch, repeat, 'geometry/index record mismatch')
    assert sha(ENGINE) == ENGINE_SHA256
    root = Path(__file__).resolve().parent
    receipt = dict(status='VERIFIED EDGE CACHE GEOMETRY; NATIVE LOOKUP NOT IMPLEMENTED', counts=counts,
                   repeats_per_arch=2, builds=builds,
                   source_sha256={name: sha(root/name) for name in ['source_displacement_edge_planes_candidate.h',
                                  'probe_displacement_edge_planes.cpp', 'build_displacement_edge_planes.cmd', 'verify_displacement_edge_planes.py']},
                   record_bytes=len(baseline), record_sha256=hashlib.sha256(baseline).hexdigest(),
                   engine_sha256=ENGINE_SHA256, original_unchanged=True,
                   verified_spans=[row['name'] for row in evidence],
                   findings=[
                       'Full original1015c3b0 and its three edge-plane builders execute; only intern1015b9f0 is intercepted by a disposable-thread hardware breakpoint.',
                       'Owned output matches native ordered normalized plane triples, edge/family destination slots,65535 skips and host-returned signed16-bit IDs.',
                       'An edge plane is skipped if either projected coefficient is zero. Third-point orientation uses the0.03125 threshold before float distance comparison.',
                       'Cases cover six vertex permutations, masked high vertex bits, arbitrary/axis-aligned/collinear/repeated vertices and small third-point offsets.',
                       'Private executable sections and installed engine hash remain unchanged.'],
                   limits=[
                       'Native hash-bucket selection, exact/sign matching, vector allocation and whole-mesh cache lifecycle are not executed or reproduced by this test.',
                       'The interning adapter returns controlled IDs; it does not establish final production cache plane/index identity.',
                       'No contact/cache composition, full displacement/world tracing, BSP import, measured spawn or editor integration is claimed.',
                       'Finite fixture equality does not establish every x87 precision or overflow/degenerate case.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(counts=counts, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
