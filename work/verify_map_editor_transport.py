"""Compare SFM's isolated Play/Pause entry points with the owned editor model."""

import argparse
from collections import Counter
from itertools import product
import json
from pathlib import Path
import random
import struct
import subprocess
import tempfile

from verify_map_simulation_clock import IFM, REFERENCES, sha, verify_installed_ranges

QUERY = struct.Struct('<3i6I')
REPLY = struct.Struct('<if2I' + '2Ii'*4)


def main(build):
    if sha(IFM) != REFERENCES[IFM]:
        raise RuntimeError('Installed IFM differs from the pinned reference')
    run = Path(tempfile.mkdtemp(prefix='compare-', dir=build))
    image, spans = verify_installed_ranges(IFM, run/'transport-reference.local.txt', [
        ('Viewport Play/Pause dispatch', 0x10209c10, 0x10209c69),
        ('Toggleplay command registration', 0x102d1da5, 0x102d1de0),
        ('Command vtable forwarding', 0x10026f70, 0x10026f78),
        ('CIFMTool forwards to document', 0x102bba40, 0x102bba50),
        ('Play/Pause stops the timeline', 0x102b89e0, 0x102b8b06),
        ('Play/Pause starts the timeline', 0x102b4790, 0x102b48e5),
        ('Playhead listener dispatcher', 0x102beef0, 0x102bef4f),
        ('Playhead listener registration', 0x102c9430, 0x102c948a),
    ])
    assert image.string(0x10ad5ab0) == 'OnPlayPause'
    assert image.string(0x10ae8344) == 'toggleplay'
    locator = image.word(0x10aeb0f4-4)
    assert image.string(image.word(locator+12)+8) == '.?AVCIFMTool@@'
    assert image.word(0x10aeb0f4+0x584) == 0x102bba40
    rng = random.Random(89237)
    cases = [(*times, *flags) for times, flags in product(
        product((-1250, -210, -209, 0, 208, 209, 417, 1250, 9581, 9582, 9583, 9999, 10000),
                (-10000, 0), (10000, 11000)), product(range(2), repeat=6))]
    cases.extend((rng.randrange(-100000, 100000), 0, 10000, *[rng.randrange(2) for _ in range(6)])
                 for _ in range(1024))
    queries = run/'queries.bin'
    queries.write_bytes(b''.join(QUERY.pack(*row) for row in cases))
    first = None
    for arch, reference in (('x86', str(IFM)), ('x64', '-')):
        for repeat in (1, 2):
            output = run/f'{arch}-{repeat}.bin'
            completed = subprocess.run([str(build/f'transport_{arch}.exe'), str(queries), str(output), reference],
                                       capture_output=True, timeout=40)
            (run/f'{arch}-{repeat}.log').write_bytes(completed.stdout+completed.stderr)
            if completed.returncode:
                raise RuntimeError((arch, completed.returncode, completed.stderr.decode(errors='replace')))
            data = output.read_bytes()
            assert len(data) == len(cases)*REPLY.size
            if first is None:
                first = data
            elif data != first:
                for idx, (expected, actual) in enumerate(zip(REPLY.iter_unpack(first), REPLY.iter_unpack(data))):
                    if expected != actual:
                        raise RuntimeError((arch, idx, cases[idx], expected, actual))
                raise RuntimeError('transport reply byte mismatch')
    coverage = Counter()
    for query, result in zip(cases, REPLY.iter_unpack(first)):
        assert result[1] == 1 and result[2] != query[3]
        coverage[str(tuple(result[4:4+result[3]*3:3]))] += 1
    for route in ('(1, 3, 4)', '(2, 1, 4)', '(4,)'):
        assert coverage[route]
    assert sha(IFM) == REFERENCES[IFM]
    receipt = dict(matched_rows=len(cases), repeats_per_arch=2, event_sequences=dict(coverage),
                   verified_installed_spans=[span['name'] for span in spans],
                   original_ifm_unchanged=True,
                   scope='Ordinary transport and original playhead-listener dispatch. Supplied range and listener callback, observed seek/frame-sync calls, no recorded view restore')
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(receipt), run/'results.json')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    main(parser.parse_args().build.resolve())
