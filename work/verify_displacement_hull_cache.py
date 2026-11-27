"""Verify complete prepared-mesh hull cache geometry with private reference data."""
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
    image, evidence = verify_installed_ranges(ENGINE, run/'hull-cache-reference.local.txt', [
        ('complete mesh hull cache build', 0x1015c630, 0x1015c6a1),
        ('complete per-triangle cache geometry', 0x1015c3b0, 0x1015c563),
        ('complete spare-capacity triangle index sizing', 0x1015b380, 0x1015b3f1),
        ('external-storage lookup cleanup', 0x1015b600, 0x1015b650),
        ('complete plane intern', 0x1015b9f0, 0x1015badb),
    ])
    offset = image.offset(0x104129b0)
    data = image.b[offset:offset+1024]
    assert hashlib.sha256(data).hexdigest() == PERMUTATION_SHA256
    fixture = run/'permutation-reference.local.bin'
    fixture.write_bytes(data)
    baseline = counts = None
    builds = {}
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = directory/f'hull_cache_{arch}.exe'
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
            assert current['passed'] and current['cases'] == 48 and current['triangles'] == 8160
            assert current['planes'] and current['signed_indices'] and current['skipped']
            if counts is None:
                counts = current
            assert counts == current
            payload = records.read_bytes()
            cursor = total = 0
            for _ in range(48):
                power, shape, triangles, planes = struct.unpack_from('<4I', payload, cursor)
                assert 1 <= power <= 4 and shape < 4 and triangles == 2*(1 << power)**2
                cursor += 16+18*triangles+12*planes
                total += planes
            assert cursor == len(payload) and total == counts['planes']
            if baseline is None:
                baseline = payload
            assert payload == baseline, (arch, repeat, 'whole-mesh cache mismatch')
    assert sha(ENGINE) == ENGINE_SHA256
    root = Path(__file__).resolve().parent
    receipt = dict(status='VERIFIED PREPARED-MESH HULL CACHE GEOMETRY', counts=counts,
                   repeats_per_arch=2, builds=builds,
                   source_sha256={name: sha(root/name) for name in ['source_displacement_hull_cache_candidate.h',
                                  'probe_displacement_hull_cache.cpp', 'build_displacement_hull_cache.cmd', 'verify_displacement_hull_cache.py']},
                   record_bytes=len(baseline), record_sha256=hashlib.sha256(baseline).hexdigest(),
                   engine_sha256=ENGINE_SHA256, original_unchanged=True,
                   permutation_sha256=PERMUTATION_SHA256, production_permutation_bundled=False,
                   verified_spans=[row['name'] for row in evidence],
                   findings=[
                       'Original full1015c630 executes sizing with spare capacity, all triangle plane builders, real hash/lookup/intern and external-storage lookup cleanup.',
                       'Only bucket insertion1015b150 and plane-vector append10001610 use fixed-capacity adapters. Native cleanup skips deallocation via external-storage flags.',
                       'All generated18-byte triangle records and pooled plane bytes match the composed owned implementation.',
                       'Each native cache is invoked twice; the unchanged-count fast path preserves cache bytes and makes no new insertions.',
                       'Four grid powers, four authored surface shapes and three hash configurations match across four repeated architecture runs.'],
                   limits=[
                       'Prepared vertices and triangle metadata are supplied. This does not verify original BSP displacement import.',
                       'Native allocation/growth, global cache manager, synchronization, invalidation after changed geometry, and failure paths remain unverified.',
                       'Permutation remains explicit private test data, not a bundled production resource. No table/DLL dependency is added to editor.',
                       'Tree/contact/cache composition, outer world adapter, full collision and editor integration remain separate.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(counts=counts, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
