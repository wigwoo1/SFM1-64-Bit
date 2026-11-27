"""Compare isolated noclip command state against the installed server."""

import argparse
from collections import Counter
from itertools import product
import json
from pathlib import Path
import struct
import subprocess
import tempfile

from verify_map_trigger_actor import REFERENCES, SERVER, sha

QUERY = struct.Struct('<5Ii4I')
REPLY = struct.Struct('<8I')


def main(build):
    reference_sha = sha(SERVER)
    if reference_sha != REFERENCES[SERVER]:
        raise RuntimeError("Installed server version does not match the fixture")
    run = Path(tempfile.mkdtemp(prefix='compare-', dir=build))
    cases = [(*actor, mode, *command, flags, solid, edict, entity_flags)
             for actor, mode, command, flags, solid, edict, entity_flags in product(
                 product(range(2), repeat=3), (0, 2, 3, 8), ((1, 0), (2, 0), (2, -2), (2, 3)),
                 (0, 4, 8, 12, 0x8000), (0, 1, 2, 3, 6), range(2), (0, 4, 0x20, 0x24))]
    queries = run/'queries.bin'
    queries.write_bytes(b''.join(QUERY.pack(*row) for row in cases))
    baseline = None
    for arch, reference in (('x86', str(SERVER)), ('x64', '-')):
        exe = build/f'noclip_{arch}.exe'
        for iteration in (1, 2):
            output = run/f'{arch}-{iteration}.bin'
            subprocess.run([str(exe), str(queries), str(output), reference], check=True, timeout=60)
            data = output.read_bytes()
            if len(data) != REPLY.size*len(cases):
                raise RuntimeError(f'{arch} incomplete rows')
            if baseline is None:
                baseline = data
            elif data != baseline:
                for row, (expected, actual) in enumerate(zip(REPLY.iter_unpack(baseline), REPLY.iter_unpack(data))):
                    if expected != actual:
                        raise RuntimeError(f'{arch} row {row} {cases[row]}: {expected} vs {actual}')
    coverage = Counter()
    for query, reply in zip(cases, REPLY.iter_unpack(baseline)):
        initial_mode, solid_flags, solid_type = query[3], query[6], query[7]
        if reply[2:4] != (solid_flags, solid_type):
            raise RuntimeError('The command changed collision solidity')
        mode = reply[0]
        coverage['unchanged' if mode == initial_mode else 'entered' if mode == 8 else 'left'] += 1
    if sha(SERVER) != reference_sha:
        raise RuntimeError('Installed server hash changed during verification')
    result = dict(matched_rows=len(cases), architecture_runs=2, coverage=dict(coverage),
                  server_sha256=reference_sha, original_server_unchanged=True,
                  scope='Original command and movement-mode setter with isolated engine callbacks and unstuck disabled')
    (run/'results.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result), run/'results.json')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    main(parser.parse_args().build.resolve())
