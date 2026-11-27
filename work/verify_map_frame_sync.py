"""Compare IFM's isolated frame-sync settings dispatcher with the owned x64 route."""

import argparse
from collections import Counter
from itertools import product
import json
from pathlib import Path
import random
import struct
import subprocess
import tempfile

from verify_map_simulation_clock import ENGINE, IFM, REFERENCES, sha, verify_installed_ranges

QUERY = struct.Struct('<8I2f')
REPLY = struct.Struct('<2I' + 'If'*6)


def main(build):
    expected_hash = REFERENCES[IFM]
    if sha(IFM) != expected_hash or sha(ENGINE) != REFERENCES[ENGINE]:
        raise RuntimeError('Installed IFM or engine version does not match this fixture')
    run = Path(tempfile.mkdtemp(prefix='compare-', dir=build))
    engine, engine_spans = verify_installed_ranges(ENGINE, run/'engine-sync-spans.local.txt', [
        ('host_syncfps registration', 0x10307130, 0x10307157),
        ('outer frame sync wait', 0x1007cf92, 0x1007d033),
        ('fixed host frame duration', 0x10180330, 0x101803e4),
        ('fixed duration cvar gate registration', 0x1030b4ba, 0x1030b4df),
    ])
    names = [engine.string(seq) for seq in (0x103318ec, 0x10358b48, 0x10331470)]
    if names != ['host_syncfps', 'host_framerate', 'sv_cheats']:
        raise RuntimeError('Unexpected clock variable names')
    rates = (23.976, 24., 25., 29.97, 30., 59.94, 60., 119.88, 120., 24.499,
             0.5, 24.001, 24.0005, 24/1.001, 120/1.001, 30000/1001)
    rows = [(*flags, rate, initial)
            for flags, rate, initial in product(product(range(2), repeat=8), rates,
                                                (0., 24., 30., 60.))]
    rng = random.Random(17274)
    for index in range(2048):
        rate = rng.uniform(1, 300) if index % 2 else rng.randrange(1, 300)/1.001 + rng.uniform(-.00002, .00002)
        rows.append((0, 1, 1, index % 2, 1, 1, 1, 1, rate, 24.))
    queries = run/'queries.bin'
    queries.write_bytes(b''.join(QUERY.pack(*row) for row in rows))
    baseline = None
    for arch, reference in (('x86', str(IFM)), ('x64', '-')):
        for iteration in (1, 2):
            output = run/f'{arch}-{iteration}.bin'
            subprocess.run([str(build/f'frame_sync_{arch}.exe'), str(queries), str(output), reference],
                           check=True, timeout=60)
            data = output.read_bytes()
            if len(data) != REPLY.size*len(rows):
                raise RuntimeError(f'{arch} incomplete replies')
            if baseline is None:
                baseline = data
            elif baseline != data:
                for idx, (expected, actual) in enumerate(zip(REPLY.iter_unpack(baseline), REPLY.iter_unpack(data))):
                    if expected != actual:
                        raise RuntimeError(f'{arch} row {idx} {rows[idx]}: {expected} vs {actual}')
                raise RuntimeError(f'{arch} byte mismatch')
    coverage = Counter()
    for row, reply in zip(rows, REPLY.iter_unpack(baseline)):
        preference, count, *events = reply
        if preference != row[4]:
            raise RuntimeError('Frame-sync preference was not stored')
        tags = tuple(events[:count*2:2])
        coverage[str(tags)] += 1
    for required in ((), (1, 2, 3), (1, 2, 4), (5, 2), (1, 2, 3, 5, 2)):
        if not coverage[str(required)]:
            raise RuntimeError(f'Missing expected sequence {required}')
    if sha(IFM) != expected_hash or sha(ENGINE) != REFERENCES[ENGINE]:
        raise RuntimeError('Original IFM or engine hash changed during verification')
    result = dict(rows=len(rows), repeats_per_architecture=2, observed_sequences=dict(coverage),
                  originals_unchanged=True, ifm_sha256=expected_hash, engine_sha256=REFERENCES[ENGINE],
                  static_engine_ranges=[e['name'] for e in engine_spans],
                  scope='Installed IFM preference setter, frame-rate conversion, and settings calls with supplied document and cvar callbacks')
    (run/'results.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result), run/'results.json')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    main(parser.parse_args().build.resolve())
