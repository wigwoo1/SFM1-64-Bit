"""Verify pure hash/pool mechanics with an explicitly identified private table."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile
from verify_sliding_door_spawn import ENGINE, sha, verify_installed_ranges

ENGINE_SHA256 = 'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
PERMUTATION_SHA256 = '35e914e111dddaa8e76930d29c4c2f46156130b7f2d00ff7734274cb947adbe6'


def main(directory):
    assert sha(ENGINE) == ENGINE_SHA256
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    image, evidence = verify_installed_ranges(ENGINE, run/'plane-pool-reference.local.txt', [
        ('complete Hash12', 0x10261bd0, 0x10261ca3),
        ('complete exact/sign hash bucket lookup', 0x10159890, 0x101599a2),
        ('complete plane intern', 0x1015b9f0, 0x1015badb),
    ])
    offset = image.offset(0x104129b0)
    data = image.b[offset:offset+1024]
    assert len(set(struct.unpack('<256I', data))) == 256
    assert set(struct.unpack('<256I', data)) == set(range(256))
    assert hashlib.sha256(data).hexdigest() == PERMUTATION_SHA256
    fixture = run/'permutation-reference.local.bin'
    fixture.write_bytes(data)
    baseline = counts = None
    builds = {}
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = directory/f'plane_pool_{arch}.exe'
        log = (directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log, log
        binary = exe.read_bytes()
        pe = struct.unpack_from('<I', binary, 60)[0]
        assert binary[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H', binary, pe+4)[0] == machine
        builds[arch] = dict(executable=str(exe), sha256=sha(exe), machine=hex(machine), warnings=0)
        for repeat in (1, 2):
            records = run/f'{arch}-{repeat}.bin'
            result = subprocess.run([str(exe), str(ENGINE) if arch == 'x86' else '-', str(fixture), str(records)], capture_output=True, text=True, timeout=45)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode == 0, (arch, result.returncode, result.stderr)
            current = json.loads(result.stdout)
            assert current['passed'] and current['cases'] == 4096
            assert current['new_planes'] and current['duplicates'] and current['reversed']
            if counts is None:
                counts = current
            assert current == counts
            payload = records.read_bytes()
            cursor = total = 0
            for _ in range(8):
                cursor += 512*16
                size, = struct.unpack_from('<I', payload, cursor)
                assert size <= 512
                total += size
                cursor += 4+size*12
            assert cursor == len(payload) and total == counts['new_planes']
            if baseline is None:
                baseline = payload
            assert payload == baseline, (arch, repeat, 'native/pure x64 pool record mismatch')
    assert sha(ENGINE) == ENGINE_SHA256
    root = Path(__file__).resolve().parent
    receipt = dict(status='VERIFIED HASH/POOL MECHANICS WITH EXPLICIT PERMUTATION DATA', counts=counts,
                   repeats_per_arch=2, builds=builds,
                   source_sha256={name: sha(root/name) for name in ['source_displacement_plane_pool_candidate.h',
                                  'probe_displacement_plane_pool.cpp', 'build_displacement_plane_pool.cmd', 'verify_displacement_plane_pool.py']},
                   record_bytes=len(baseline), record_sha256=hashlib.sha256(baseline).hexdigest(),
                   engine_sha256=ENGINE_SHA256, original_unchanged=True,
                   permutation_sha256=PERMUTATION_SHA256, production_permutation_bundled=False,
                   verified_spans=[row['name'] for row in evidence],
                   findings=[
                       'The incorrect random-generator default is removed. The pure code requires an explicit permutation and owns a validated copy.',
                       'Original10261bd0 hash,10159890 bucket lookup and1015b9f0 intern execute. Only bucket insertion1015b150 and plane-vector append10001610 use fixed-capacity fixture adapters.',
                       'Every query hash, returned ID, plane byte, bucket entry/order and insertion count matches the pure implementation in the native probe.',
                       'Masked/modulo bucket configurations1/3/8/17/64 cover duplicates, reversed planes, collisions, signed zero and one-ULP differences.',
                       'All four runs have identical records. Original private executable sections and installed engine hash remain unchanged.'],
                   limits=[
                       'This verifies mechanics with private identified reference data. No permutation constants are embedded in project source or the editor package.',
                       'Production resource provenance/distribution remains a separate gate; the public SDK table identity was independently checked in table-provenance.local.json.',
                       'Native vector allocation/growth, full cache lifecycle, triangle-cache generation composition, displacement/world tracing and editor integration remain separate.',
                       'Finite tests do not establish malformed/NaN input equivalence, native overflow or allocation-failure behavior.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(counts=counts, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
