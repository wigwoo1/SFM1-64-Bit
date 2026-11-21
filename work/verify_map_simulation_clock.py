"""Compare local server timing and event admission using isolated reference processes."""

from collections import Counter
from itertools import product
import json
from pathlib import Path
import random
import struct
import subprocess
import tempfile

from verify_sliding_door_spawn import ENGINE, IFM, SERVER, f32, sha, verify_installed_ranges

QUERY = struct.Struct('<4i3f4I')
REPLY = struct.Struct('<iff6I')
REFERENCES = {
    ENGINE: 'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f',
    SERVER: '47a03f6bce8f519dafe1cfbcb9ba97cf0b0069d6eab57c2a52cffd61c66265d4',
    IFM: '12504c248047e181d31a8500fb6693d1a2d3c805e841fc2227e41aa4e2d2b47e',
}


def verify_call_chain(run):
    engine, engine_ranges = verify_installed_ranges(ENGINE, run/'engine-reference.local.txt', [
        ('Local server GameFrame publication', 0x1011d280, 0x1011d340),
        ('Plugin wrapper and GameFrame tail call', 0x10122090, 0x101220f8),
        ('Server object time', 0x1012d870, 0x1012d87a),
        ('Engine passes server globals to DLL init', 0x1011d7ca, 0x1011d7e6),
        ('EngineTool server time', 0x101e5740, 0x101e5747),
        ('EngineTool server frame time', 0x101e5750, 0x101e5757),
        ('EngineTool server tick', 0x101e5760, 0x101e5766),
        ('Engine calls server factory', 0x101cad15, 0x101cad2a),
        ('Host frame rate variable registration', 0x1030e200, 0x1030e226),
        ('Host frame rate gate and reciprocal', 0x10180330, 0x10180399),
    ])
    server, server_ranges = verify_installed_ranges(SERVER, run/'server-reference.local.txt', [
        ('GameFrame dispatch', 0x10143030, 0x101431da),
        ('Single-client alternate ticks predicate', 0x100cdd90, 0x100cddad),
        ('Event queue service wrapper', 0x100ea150, 0x100ea15a),
        ('Event queue first-item time and debugger gate', 0x100e9ad0, 0x100e9b25),
        ('Server saves engine globals', 0x10147179, 0x10147182),
    ])
    ifm, ifm_ranges = verify_installed_ranges(IFM, run/'ifm-reference.local.txt', [
        ('Game time readout', 0x102e5479, 0x102e549f),
        ('Editor frame sync cvar calls', 0x1029a030, 0x1029a1c6),
    ])
    assert engine.string(0x1036082c) == server.string(0x10495ad8) == 'ServerGameDLL005'
    assert engine.word(0x104025fc) == 0x10402604
    assert engine.word(engine.word(0x10402604) + 0x14) == 0x10122090
    assert server.word(0x10495404 + 0x10) == 0x10143030
    assert server.word(0x10445090) == 0x40000000
    assert engine.word(0x103666c4 + 0xac) == 0x101e5740
    assert ifm.string(0x10aedaa0) == '[game: %.3f]'
    assert ifm.string(0x10ae52a8) == 'host_syncfps'
    assert ifm.string(0x10ae5298) == engine.string(0x10358b48) == 'host_framerate'
    return {'engine': engine_ranges, 'server': server_ranges, 'ifm': ifm_ranges}


def fixture_rows():
    ticks = (0, 1, 2, 3, 66, 67, 264, 265, 16777219, 1 << 27, (1 << 31) - 1)
    for tick, slots, signon, requested, restoring, alternate in product(
            ticks, (0, 1, 2), (0, 5, 6, 7), (0, 1), (0, 1), (0, 1)):
        interval, host_step = f32(0.015), f32(1/24)
        yield (tick, slots, signon, 0, interval, host_step, f32(tick*interval),
               requested, restoring, alternate, 0)

    randomizer = random.Random(110721)
    for _ in range(1024):
        tick = randomizer.randrange(1 << 31)
        interval = f32(randomizer.choice((1/20, 1/24, 0.015, 1/66, 1/128, 0.01)))
        host_step = f32(randomizer.choice((0, 1/120, 1/30, 1/24, 1/60)))
        yield (tick, randomizer.randrange(1, 5), randomizer.randrange(8), 0,
               interval, host_step, f32(tick*interval), *[randomizer.randrange(2) for _ in range(3)], 0)

    for tick, credit, stepping, requested in product((0, 1, 2, 3, 265, 20000000),
            (-1, 0, 1, 5), (0, 1), (0, 1)):
        interval = f32(0.015)
        now = f32(tick * interval)
        now_word, = struct.unpack('<I', struct.pack('<f', now))
        neighbor_words = [now_word - 1, now_word, now_word + 1] if now else [0x80000001, 0, 1]
        for fire_word in neighbor_words:
            fire, = struct.unpack('<f', struct.pack('<I', fire_word))
            yield (tick, 1, 6, credit, interval, interval, fire, requested, 0, 0, stepping)


def predict(mode, row):
    tick, slots, signon, credits, interval, host_step, deadline, requested, restoring, alternate, stepping = row
    now = f32(tick * interval)
    dt = host_step if requested else 0
    simulating = bool(requested and (slots > 1 or signon == 6))
    physics, physics_advance, clients, service, due = 0, 0, 0, 0, 0
    if mode == 'server' and not restoring:
        if alternate and slots == 1:
            clients = tick % 2
            if not clients:
                dt *= 2
        if not clients:
            physics, physics_advance, service = 1, simulating, 1
    if mode == 'event':
        due = not (stepping and credits <= 0) and deadline <= now
    return (tick, now, dt, simulating, clients, physics, physics_advance, service, due)


def main(build):
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=build))
    for path, known in REFERENCES.items():
        assert sha(path) == known, f'Changed reference: {path.name}'
    spans = verify_call_chain(run)
    rows = list(fixture_rows())
    queries = run/'query.bin'
    queries.write_bytes(b''.join(QUERY.pack(*row) for row in rows))
    builds = {}
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        executable = build/f'map_clock_{arch}.exe'
        raw = executable.read_bytes()
        offset, = struct.unpack_from('<I', raw, 0x3c)
        assert raw[offset:offset+4] == b'PE\0\0'
        assert struct.unpack_from('<H', raw, offset+4)[0] == machine
        log = (build/f'map_clock_{arch}-build.log').read_text()
        assert 'warning ' not in log and 'error ' not in log
        builds[arch] = {'executable': str(executable), 'sha256': sha(executable), 'warnings': 0}
    modes = {}
    for mode, reference in [('engine', ENGINE), ('server', SERVER), ('event', SERVER)]:
        predictions = [predict(mode, row) for row in rows]
        target = b''.join(REPLY.pack(*row) for row in predictions)
        for arch in builds:
            for repeat in (1, 2):
                output = run/f'{mode}-{arch}-{repeat}.bin'
                result = subprocess.run([builds[arch]['executable'], mode,
                    str(reference) if arch == 'x86' else '-', queries, output],
                    capture_output=True, timeout=35)
                (run/f'{mode}-{arch}-{repeat}.log').write_bytes(result.stdout+result.stderr)
                assert result.returncode == 0, (mode, arch, result.returncode)
                actual = output.read_bytes()
                if actual != target:
                    assert len(actual) == len(target)
                    index = next(i for i, (a, b) in enumerate(zip(actual, target)) if a != b) // REPLY.size
                    raise AssertionError((mode, arch, index, rows[index],
                        REPLY.unpack_from(actual, index*REPLY.size), predictions[index]))
        modes[mode] = {
            'matched_rows': len(rows),
            'route_counts': dict(Counter(
                'clients only' if row[4] else
                'simulate and service events' if row[6] else
                'maintenance and service events' if row[7] else
                'dispatch simulation' if row[3] and mode == 'engine' else
                'due' if row[8] else 'other'
                for row in predictions)),
        }
    assert all(sha(path) == known for path, known in REFERENCES.items())
    report = {
        'references_unchanged': {str(path): digest for path, digest in REFERENCES.items()},
        'original_instruction_ranges_checked': spans,
        'builds': builds,
        'repeats_per_arch_and_mode': 2,
        'compared_routes': modes,
        'limitations': 'Controlled inputs and callback boundary observations. No SFM UI or map simulation, no live transport, no target invocation or mutation of queue items.',
    }
    (run/'results.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(f'{sum(v["matched_rows"] for v in modes.values())} matched rows across engine, server, and event routes')
    print(run/'results.json')


if __name__ == '__main__':
    import sys
    main(Path(sys.argv[1]).resolve())
