"""Compare composed prepared-mesh hull sweeps with intact original engine code."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
from verify_sliding_door_spawn import ENGINE, sha, verify_installed_ranges

ENGINE_SHA256 = 'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
PERMUTATION_SHA256 = '35e914e111dddaa8e76930d29c4c2f46156130b7f2d00ff7734274cb947adbe6'


def main(directory):
    assert sha(ENGINE) == ENGINE_SHA256
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    image, evidence = verify_installed_ranges(ENGINE, run/'hull-sweep-reference.local.txt', [
        ('complete mesh cache build', 0x1015c630, 0x1015c6a1),
        ('complete triangle cache geometry', 0x1015c3b0, 0x1015c563),
        ('complete plane intern', 0x1015b9f0, 0x1015badb),
        ('complete hull tree and contact dispatch', 0x1015c840, 0x1015cc1f),
        ('complete direction normalization', 0x10244060, 0x102440cd),
        ('complete hull triangle contact', 0x1015abb0, 0x1015afa3),
        ('complete shared plane clip', 0x10158cb0, 0x10158d70),
        ('complete face plane clip', 0x10158d70, 0x10158e77),
        ('complete projected edge clip YZ', 0x10159520, 0x10159641),
        ('complete projected edge clip XZ', 0x10159650, 0x10159770),
        ('complete projected edge clip XY', 0x10159770, 0x1015988d),
    ])
    offset = image.offset(0x104129b0)
    data = image.b[offset:offset+1024]
    assert hashlib.sha256(data).hexdigest() == PERMUTATION_SHA256
    fixture = run/'permutation-reference.local.bin'
    fixture.write_bytes(data)
    baseline = counts = None
    builds = {}
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = directory/f'hull_sweep_{arch}.exe'
        log = (directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log, log
        binary = exe.read_bytes()
        pe = struct.unpack_from('<I', binary, 60)[0]
        assert binary[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H', binary, pe+4)[0] == machine
        assert struct.unpack_from('<H', binary, pe+24)[0] == (0x10b if arch == 'x86' else 0x20b)
        builds[arch] = dict(executable=str(exe), sha256=sha(exe), machine=hex(machine), warnings=0)
        for repeat in (1, 2):
            records = run/f'{arch}-{repeat}.bin'
            result = subprocess.run([str(exe), str(ENGINE) if arch == 'x86' else '-', str(fixture), str(records)], capture_output=True, text=True, timeout=45)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode == 0, (arch, result.returncode, result.stderr)
            current = json.loads(result.stdout)
            assert current['passed'] and current['cases'] == 41472 and current['meshes'] == 48
            for key in ['changed', 'trace_changes', 'zero_fraction_metadata', 'misses', 'non_axial_hits']:
                assert current[key] > 0, key
            assert current['disabled'] == current['cases']//2
            assert current['trace_changes'] > current['changed']
            if counts is None:
                counts = current
            assert counts == current
            payload = records.read_bytes()
            assert len(payload) == current['cases']*88
            hits = changed = zero_metadata = 0
            for case in range(current['cases']):
                cursor = case*88
                hit, = struct.unpack_from('<I', payload, cursor)
                assert hit in (0, 1)
                trace = payload[cursor+4:cursor+88]
                initial = bytearray([0xa5]*84)
                mode = (case//4)%3
                initial_fraction = [1., .25, 0.][mode]
                struct.pack_into('<f', initial, 0x2c, initial_fraction)
                final_fraction, = struct.unpack_from('<f', trace, 0x2c)
                assert 0 <= final_fraction <= initial_fraction
                assert hit == (initial_fraction > final_fraction)
                assert all(trace[i] == initial[i] for i in range(84) if i not in list(range(0x18, 0x28))+list(range(0x2c, 0x30))+[0x34, 0x35])
                if case%4 in (1, 3):
                    assert not hit and trace == initial
                hits += hit
                changed += trace != initial
                zero_metadata += trace != initial and mode == 2
            assert (hits, changed, zero_metadata) == (counts['changed'], counts['trace_changes'], counts['zero_fraction_metadata'])
            if baseline is None:
                baseline = payload
            assert payload == baseline, (arch, repeat, 'whole sweep trace/result mismatch')
    assert sha(ENGINE) == ENGINE_SHA256
    root = Path(__file__).resolve().parent
    sources = {}

    def visit(name):
        if name in sources:
            return
        source = root/name
        sources[name] = sha(source)
        for include in re.findall(r'^\s*#include\s*"([^"\r\n]+)"', source.read_text(), re.M):
            visit(include)

    for name in ['probe_displacement_hull_sweep.cpp', 'build_displacement_hull_sweep.cmd', 'verify_displacement_hull_sweep.py']:
        visit(name)
    receipt = dict(status='VERIFIED PREPARED-MESH DISPLACEMENT HULL SWEEP COMPOSITION', counts=counts,
                   repeats_per_arch=2, builds=builds, source_sha256=sources,
                   record_bytes=len(baseline), record_sha256=hashlib.sha256(baseline).hexdigest(),
                   engine_sha256=ENGINE_SHA256, original_unchanged=True,
                   permutation_sha256=PERMUTATION_SHA256, production_permutation_bundled=False,
                   verified_spans=[row['name'] for row in evidence],
                   replaced_callees=['bucket insertion1015b150', 'plane append10001610', 'prepared cache acquire1015c760', 'cache release10261300'],
                   findings=[
                       'Original full1015c630 builds each cache; every plane/index byte matches the owned cache builder before sweeps.',
                       'Full original1015c840 tree selection, direction normalization,1015abb0 triangle contacts and all shared/face/edge clips execute. Triangle contacts are not intercepted.',
                       'Four hardware boundaries supply fixed-capacity insertion/append and prepared-cache acquire/release only; no original instruction is patched.',
                       'Complete84-byte trace and closer-hit boolean match the composed x64 helper across both repeated architecture runs.',
                       'Four grid powers, flat/sloped/rough/vertical surfaces, three hash configurations, zero/asymmetric/player extents, zero/axial/oblique motion, four runtime flag combinations and three initial fractions are covered.',
                       'Disabled queries retain the entire trace. Zero-fraction contacts can change metadata without returning a closer hit; the composed helper preserves this.',
                       'Native cached planes/indices remain unchanged after every mesh sweep batch; cache service calls are paired.'],
                   limits=[
                       'Geometry, triangle metadata and tree bounds are prepared input, not original BSP import.',
                       'Native cache allocation/growth/global manager/lifetime/synchronization and invalidation remain unverified.',
                       'Permutation remains explicit identified private fixture data; no Valve table or DLL dependency is added to the editor.',
                       'Outer world trace metadata, leaf culling/dedup, stationary dispatch, complete world collision, benchmark spawn and editor integration remain separate.',
                       'Zero-motion calls exercise the swept entry point directly; they do not verify the original stationary dispatcher.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(counts=counts, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
