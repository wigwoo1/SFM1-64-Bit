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
BUDGET_QUERY = struct.Struct('<3fi2Ii4I')
BUDGET_REPLY = struct.Struct('<if')
DISPATCH_QUERY = struct.Struct('<6if9I')
DISPATCH_REPLY = struct.Struct('<iIIf')
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
        ('Host frame accumulator and tick budget', 0x10182407, 0x1018248e),
        ('Demo player initialization', 0x10306620, 0x1030664b),
        ('Recorded demo primary vtable constructor', 0x10067000, 0x1006700e),
        ('Recorded demo seek-in-progress test', 0x10062540, 0x1006255b),
        ('Recorded demo seek target write', 0x100625df, 0x100625f3),
        ('Recorded demo seek target clear', 0x10063ba5, 0x10063bd0),
        ('Recorded demo paused test', 0x100626d0, 0x10062700),
        ('HLTV demo interface selection', 0x1017ab64, 0x1017ab86),
        ('HLTV pause getter', 0x1017a340, 0x1017a347),
        ('Alternate tick cvar registration', 0x1030c1c0, 0x1030c1ea),
        ('Host inner loop entry', 0x101825ad, 0x101825bd),
        ('Host inner loop server dispatch', 0x10182649, 0x10182655),
        ('Host inner loop end', 0x10182697, 0x101826a0),
        ('Server step permission', 0x1011d230, 0x1011d27a),
        ('Server GameFrame dispatch and clock increment', 0x10120670, 0x1012074b),
        ('Pause noclip lookup', 0x1011d82b, 0x1011d847),
        ('Engine console visibility predicate', 0x101ec160, 0x101ec189),
        ('Engine console factory binding', 0x101ed0fd, 0x101ed111),
        ('Engine modal predicate', 0x101ebfd0, 0x101ebffb),
        ('Bug reporter pause predicate', 0x10136940, 0x1013697d),
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
    assert engine.string(0x1034cd38) == 'sv_alternateticks'
    assert engine.word(0x103fe6b4) == 0x104b5898
    for table, cls, paused in ((0x10327104, '.?AVCDemoPlayer@@', 0x100626d0),
                               (0x10356a84, '.?AVCHLTVServer@@', 0x1017a340)):
        locator = engine.word(table-4)
        assert engine.string(engine.word(locator+12)+8) == cls
        assert engine.word(table+0x1c) == paused
    assert engine.word(0x10327104+0x24) == 0x10062540
    assert engine.word(0x10327104+0x18) == 0x10062640
    assert engine.word(0x10356a84-4) == 0x103f1948
    assert engine.word(0x103f1948+4) == 0x214
    assert engine.string(0x10347b90) == 'sv_noclipduringpause'
    assert engine.string(0x1036a2fc) == 'GameConsole004'
    assert engine.word(0x1036a3f4+0x48) == 0x101ec160
    assert engine.word(0x1036a3f4+0x80) == 0x101ebfd0
    assert engine.word(engine.word(engine.word(0x10402b78))+0xc) == 0x10136940
    assert engine.word(engine.word(engine.word(0x1040c500))+0xc) == 0x10031620
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


def verify_budget(builds, run):
    rng = random.Random(91824)
    rows = list(product((-.015, -.0075, 0., .0075, .014999999, .015, .030, .3),
                        (0., .001, 1/120, 1/60, 1/24, .1), (.01, .015, 1/64, 1/128),
                        (0, 1, 71, 1024), range(2), range(2), (-1, 0, 75),
                        range(2), range(2), range(2), (0,)))
    for _ in range(2048):
        rows.append((rng.uniform(-.05, .1), rng.uniform(0, .15), rng.uniform(.001, .02),
                     rng.randrange(100000), rng.randrange(2), rng.randrange(2), rng.randrange(-2, 100),
                     rng.randrange(2), rng.randrange(2), rng.randrange(2), 0))
    sequence_ranges = []
    for fps, alternate in product((24, 25, 30, 60, 120), range(2)):
        start = len(rows)
        rows.extend((0., 1/fps, .015, 0, 0, 0, -1, 0, 0, alternate, i != 0) for i in range(fps*5))
        sequence_ranges.append((f'{fps} fps, alternate={alternate}', start, len(rows)))
    frozen = len(rows)
    rows.extend((0., 1/24, .015, 0, 1, 0, 10, 24 <= i < 48, 0, 0, i != 0) for i in range(72))
    sequence_ranges.append(('24 fps, one-second demo pause', frozen, len(rows)))

    query_bytes = b''.join(BUDGET_QUERY.pack(*row) for row in rows)
    queries = run/'budget-query.bin'
    queries.write_bytes(query_bytes)
    predictions = []
    prior_tick = 0
    prior_remainder = 0.
    gate_counts = Counter()
    for carry, duration, interval, tick, playing, seeking, cursor, paused, hltv, alternate, continues in BUDGET_QUERY.iter_unpack(query_bytes):
        if continues:
            carry, tick = prior_remainder, prior_tick
        held = paused and (hltv or (playing and not seeking and cursor >= 0))
        gate_counts[f'{"hltv" if hltv else "local demo"} {"held" if held else "added"}'] += 1
        available = carry if held else f32(carry + duration)
        steps = 0
        if available >= interval:
            steps = int(f32(available/interval))
            if alternate:
                endpoint = tick + steps
                steps = endpoint + (endpoint % 2) - tick
            available = f32(available - f32(f32(steps)*interval))
        prior_tick, prior_remainder = tick + steps, available
        predictions.append((steps, available))
    target = b''.join(BUDGET_REPLY.pack(*reply) for reply in predictions)
    for arch in builds:
        for repeat in (1, 2):
            output = run/f'budget-{arch}-{repeat}.bin'
            result = subprocess.run([builds[arch]['executable'], 'budget',
                                     str(ENGINE) if arch == 'x86' else '-', queries, output],
                                    capture_output=True, timeout=35)
            (run/f'budget-{arch}-{repeat}.log').write_bytes(result.stdout+result.stderr)
            if result.returncode:
                raise RuntimeError((arch, result.returncode, result.stderr.decode(errors='replace')))
            actual = output.read_bytes()
            if actual != target:
                assert len(actual) == len(target)
                row = next(i for i, (a, b) in enumerate(zip(actual, target)) if a != b) // BUDGET_REPLY.size
                raise AssertionError((arch, row, rows[row],
                                      BUDGET_REPLY.unpack_from(actual, row*BUDGET_REPLY.size), predictions[row]))
    assert {reply[0] for reply in predictions[sequence_ranges[0][1]:sequence_ranges[0][2]]} == {2, 3}
    assert not any(count for count, _ in predictions[frozen+24:frozen+48])
    assert any(remainder < 0 for _, remainder in predictions)
    return {'matched_rows': len(rows),
            'elapsed_gate_routes': dict(gate_counts),
            'sequences': {name: dict(frames=end-start,
                                     ticks=sum(count for count, _ in predictions[start:end]),
                                     per_frame=dict(Counter(count for count, _ in predictions[start:end])),
                                     final_remainder=predictions[end-1][1])
                          for name, start, end in sequence_ranges}}


def verify_dispatch(builds, run):
    rows = [(99, state, slots, signon, pending, client_slots, f32(.015), *flags)
            for state, slots, signon, pending, client_slots, flags in product(
                (0, 2, 3, 4), (1, 2), (0, 5, 6), (-1, 0, 1, 3), (1, 2),
                product(range(2), repeat=9))]
    queries = run/'dispatch-query.bin'
    queries.write_bytes(b''.join(DISPATCH_QUERY.pack(*row) for row in rows))
    predicted = []
    routes = Counter()
    for row in rows:
        tick, state, slots, signon, pending, client_slots, interval, loading, panel, console, visible, reporter, reporter_visible, reporter_pending, noclip, game_blocks = row
        signed_on = signon == 6
        paused_by_ui = signed_on and ((panel and console and visible) or
                                     (reporter and (reporter_visible or reporter_pending) and client_slots == 1))
        request = pending > 0 and state != 3 and (slots > 1 or not (loading or paused_by_ui))
        call = state >= 2 and (pending > 0 or slots > 1)
        advance = call and pending > 0 and not game_blocks and (request or noclip)
        sim = call and request and (slots > 1 or signed_on)
        predicted.append((tick+advance, call, sim, interval if call and request else 0.))
        routes[f'called={int(call)} advanced={int(advance)} simulated={int(sim)}'] += 1
    target = b''.join(DISPATCH_REPLY.pack(*reply) for reply in predicted)
    for arch, build in builds.items():
        for repeat in (1, 2):
            out = run/f'dispatch-{arch}-{repeat}.bin'
            ref = ENGINE if arch == 'x86' else '-'
            result = subprocess.run([build['executable'], 'dispatch', ref, queries, out],
                                    check=True, capture_output=True, timeout=35)
            (run/f'dispatch-{arch}-{repeat}.log').write_bytes(result.stdout+result.stderr)
            actual = out.read_bytes()
            if actual != target:
                assert len(actual) == len(target)
                idx = next(i for i, (a, b) in enumerate(zip(actual, target)) if a != b) // DISPATCH_REPLY.size
                raise AssertionError((arch, idx, rows[idx], DISPATCH_REPLY.unpack_from(actual, idx*DISPATCH_REPLY.size), predicted[idx]))
    return {'matched_rows': len(rows), 'route_counts': dict(routes)}


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
    modes['budget'] = verify_budget(builds, run)
    modes['dispatch'] = verify_dispatch(builds, run)
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
    print(f'{modes["budget"]["matched_rows"]} host budget rows, '
          f'{modes["dispatch"]["matched_rows"]} server dispatch rows, and '
          f'{sum(modes[k]["matched_rows"] for k in ("engine", "server", "event"))} existing clock and event rows matched')
    print(run/'results.json')


if __name__ == '__main__':
    import sys
    main(Path(sys.argv[1]).resolve())
