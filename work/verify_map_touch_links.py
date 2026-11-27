"""Compare ordered contact events and link state against the installed SFM server."""

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import random
import subprocess
import tempfile

from verify_sliding_door_spawn import SERVER, sha

SERVER_SHA = '47a03f6bce8f519dafe1cfbcb9ba97cf0b0069d6eab57c2a52cffd61c66265d4'


def scenarios():
    yield [
        ('s', 0, 1), ('s', 1, 2), ('b', 0, 0), ('c', 0, 1), ('f', 0, 0),
        ('b', 0, 0), ('c', 0, 1), ('f', 0, 0), ('b', 0, 0), ('f', 0, 0),
    ]
    yield [
        ('s', 0, 1), ('s', 1, 2), ('s', 2, 2), ('b', 0, 0),
        ('c', 0, 1), ('c', 0, 2), ('f', 0, 0), ('b', 0, 0), ('f', 0, 0),
    ]
    yield [
        ('s', 0, 1), ('s', 1, 2), ('c', 0, 1), ('h', 0, 0),
        ('b', 1, 0), ('f', 1, 0), ('h', 0, 1), ('c', 0, 1),
        ('b', 0, 0), ('f', 0, 0),
    ]
    yield [
        ('s', 0, 1), ('s', 1, 2), ('e', 0, 0xfffffffe), ('b', 0, 0),
        ('c', 0, 1), ('q', 0, 1), ('f', 0, 0), ('b', 0, 0), ('f', 0, 0),
    ]
    yield [
        ('s', 0, 1), ('s', 1, 2), ('p', 1, 0), ('c', 0, 1),
        ('p', 1, 6), ('q', 0, 1), ('c', 0, 1), ('c', 0, 1),
        ('b', 0, 0), ('f', 0, 0),
    ]
    rng = random.Random(27112026)
    for _ in range(250):
        rows = [('s', a, rng.choice((0, 1, 1, 2, 2, 3, 5, 8, 16))) for a in range(6)]
        active = set(range(6))
        for _ in range(100):
            op = rng.choice('bbbccccccffffspqh')
            a = rng.randrange(6)
            if op == 'h':
                b = int(a not in active)
                if b:
                    active.add(a)
                else:
                    active.remove(a)
            elif op in 'bf':
                if a not in active:
                    continue
                b = 0
            elif op == 's':
                b = rng.randrange(32)
            elif op == 'q':
                b = rng.randrange(2)
            else:
                b = rng.randrange(7 if op == 'p' else 6)
            rows.append((op, a, b))
        yield rows


def queue_scenarios():
    yield [
        ('s', 0, 1), ('s', 1, 1), ('s', 2, 2), ('s', 3, 1),
        ('c', 0, 2), ('c', 1, 2),
        ('b', 1, 0), ('b', 0, 0), ('b', 1, 0), ('j', 3, 0),
        ('u', 0, 0), ('u', 0, 0), ('u', 0, 0),
    ]
    yield [
        ('s', 0, 1), ('s', 1, 1), ('s', 2, 2),
        ('c', 0, 2), ('c', 1, 2), ('b', 0, 0), ('b', 1, 0), ('x', 0, 0),
        ('b', 0, 0), ('x', 1, 0), ('u', 0, 0),
    ]
    yield [('s', 0, 9), ('b', 0, 0), ('u', 0, 0), ('s', 0, 1),
           ('x', 0, 0), ('b', 0, 0), ('u', 0, 0)]
    rng = random.Random(41105)
    for _ in range(200):
        yield [(op, rng.randrange(6), rng.randrange(32 if op=='s' else 6))
               for op in (rng.choice('bbbcccxxxuuuusj') for _ in range(100))]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build_directory', type=Path)
    parser.add_argument('--queue', action='store_true')
    flags = parser.parse_args()
    build = flags.build_directory.resolve()
    if sha(SERVER) != SERVER_SHA:
        raise RuntimeError('The installed server differs from the verified reference')
    run = Path(tempfile.mkdtemp(prefix='untouch-queue-' if flags.queue else 'touch-links-', dir=build))
    cases = list(queue_scenarios() if flags.queue else scenarios())
    steps = [row for case in cases for row in [('r', 0, 0), *case]]
    query = run / 'steps.txt'
    query.write_text(''.join(f'{op} {a} {b}\n' for op, a, b in steps), encoding='ascii')
    reference = None
    builds = {}
    for arch in ('x86', 'x64'):
        executable = build / f'touch_{arch}.exe'
        cmd = [str(executable), str(query)]
        warnings = (build / f'touch_{arch}-build.log').read_text().count('warning ')
        builds[arch] = dict(sha256=sha(executable), warnings=warnings)
        for repeat in range(2):
            path = run / f'{arch}-{repeat}.txt'
            args = [*cmd, str(path)]
            if arch == 'x86':
                args.append(str(SERVER))
            if flags.queue:
                args.append('--queue')
            subprocess.run(args, check=True, timeout=90)
            rows = path.read_text(encoding='ascii').splitlines()
            assert len(rows) == len(steps), arch
            if reference is None:
                reference = rows
            else:
                for i, (actual, expected) in enumerate(zip(rows, reference)):
                    if actual != expected:
                        raise AssertionError(f'{arch} run {repeat}, row {i+1}, {steps[i]}\n'
                                             f'expected {expected}\nactual   {actual}\n{run}')
    def events(i):
        return reference[i].split('|')[0].split()
    if flags.queue:
        assert reference[10].split('|')[-1].split() == ['65537','65536']
        assert events(11) == ['3:65538:65537', '3:65538:65536']
        assert reference[11].split('|')[-1].split() == ['65539']
        assert reference[12].split('|')[-1].split() == []
    else:
        assert events(4) == ['1:65537:65536', '2:65537:65536']
        assert events(7) == ['2:65536:65537', '2:65537:65536']
        assert events(10) == ['3:65537:65536']
        assert events(20) == ['3:65538:65536', '3:65537:65536']
    stats = Counter(event.split(':')[0] for row in reference for event in row.split('|')[0].split())
    assert sha(SERVER) == SERVER_SHA
    (run/'results.json').write_text(json.dumps(dict(scenarios=len(cases), matched_rows=len(steps),
        repeats_per_arch=2, start_events=stats['1'], touch_events=stats['2'], end_events=stats['3'],
        scheduled_notifications=stats['4'], service_passes=sum(op=='u' for op,_,_ in steps),
        native_queue=flags.queue, installed_server_unchanged=True, builds=builds,
        input_sha256=hashlib.sha256(query.read_bytes()).hexdigest()), indent=2)+'\n', encoding='utf-8')
    print(run / 'results.json')


if __name__ == '__main__':
    main()
