"""Compare benchmark trigger transitions and reentrant event dispatch."""

import argparse
from collections import Counter
import heapq
import itertools
import json
from pathlib import Path
import random
import re
import struct
import subprocess
import tempfile

from verify_sliding_door_spawn import BSP, SERVER, f32, read_map_doors, sha, verify_installed_ranges

STEP = struct.Struct('<IIIiff')
REPLY = struct.Struct('<5Ii32I')
SERVER_SHA = '47a03f6bce8f519dafe1cfbcb9ba97cf0b0069d6eab57c2a52cffd61c66265d4'
BSP_SHA = '60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea'
START, END, SERVICE, REMOVE, RESPAWN, TEST, DISABLE, OPEN, CLOSE = range(9)
TRIGGERS = ('*2', '*4', '*6', '*9')
DOORS = ('*1', '*3', '*5', '*8')


def bsp_routes():
    data = BSP.read_bytes()
    offset, size = struct.unpack_from('<II', data, 8)
    entities = [{k.decode(): v.decode() for k, v in re.findall(rb'"([^"]+)"\s*"([^"]*)"', entity)
                 if k in {b'model', b'targetname', b'classname', b'spawnflags', b'filtername', b'wait'}
                 or k.startswith(b'On')}
                for entity in re.findall(rb'\{[^{}]+\}', data[offset:offset+size])]
    doors = {model: name for model, name, _ in read_map_doors()[0]}
    result = {}
    for index, (trigger, door) in enumerate(zip(TRIGGERS, DOORS)):
        entity, = [fields for fields in entities if fields.get('model') == trigger
                   and fields.get('classname') == 'trigger_multiple']
        assert entity['spawnflags'] == '1' and entity['wait'] == '0' and not entity.get('filtername')
        own_name = entity.get('targetname')
        routes = {k: v.split(',') for k, v in entity.items() if k.startswith('On')}
        expected = {'OnStartTouch': [doors[door], 'Open', '', '0', '-1']}
        if index < 2:
            expected['OnEndTouchAll'] = [doors[door], 'Close', '', '0', '-1']
        else:
            assert own_name
            expected['OnEndTouch'] = [own_name, 'TouchTest', '', '0', '-1']
            expected['OnNotTouching'] = [doors[door], 'Close', '', '0', '-1']
        assert routes == expected
        result[trigger] = {'door': door, 'routes': routes}
    return result


def op(action, trigger=0, actor=0, argument=0, current=1., delay=0.):
    return action, trigger, actor, argument, f32(current), f32(delay)


def sequences():
    for trigger in range(4):
        yield 'single client departure', [
            op(START, trigger, 5), op(START, trigger), op(SERVICE),
            op(START, trigger), op(SERVICE), op(END, trigger, 4), op(SERVICE),
            op(END, trigger), op(SERVICE, 1, argument=1), op(SERVICE, 1, argument=0),
            op(SERVICE, 1, argument=1), op(SERVICE),
        ]
        yield 'overlap on departure and reentry', [
            op(START, trigger, 0), op(START, trigger, 1), op(SERVICE),
            op(END, trigger, 0), op(SERVICE), op(END, trigger, 1), op(START, trigger, 2),
            op(SERVICE), op(END, trigger, 2), op(SERVICE),
        ]
        yield 'serial reuse and disabled test', [
            op(START, trigger), op(SERVICE), op(REMOVE), op(TEST, trigger), op(SERVICE),
            op(RESPAWN, argument=1), op(END, trigger), op(START, trigger, 1),
            op(START, trigger), op(END, trigger), op(END, trigger, 1), op(SERVICE),
            op(DISABLE, trigger, argument=1), op(TEST, trigger), op(SERVICE),
            op(DISABLE, trigger), op(TEST, trigger), op(SERVICE),
        ]
    yield 'stable ordering and same-pass enqueue', [
        op(TEST, 2, delay=0.5), op(OPEN, 3, delay=0.5), op(CLOSE, 0, delay=0.25),
        op(OPEN, 1), op(TEST, 3, delay=0.5), op(SERVICE, current=1.),
        op(SERVICE, current=1.1875), op(SERVICE, current=1.25),
        op(SERVICE, 1, argument=2, current=1.5), op(SERVICE, 1, argument=4, current=1.5),
    ]
    for trigger in range(4):
        rows = []
        for index in range(98):
            now = f32(1 + index*.015)
            if index in (0, 8):
                rows.append(op(START, trigger, int(index == 8), current=now))
            if index in (13, 48):
                rows.append(op(END, trigger, int(index == 48), current=now))
            rows.append(op(SERVICE, current=now))
        yield 'clocked doors', rows
    rng = random.Random(612147)
    for _ in range(100):
        current = 1.
        rows = []
        for i in range(96):
            current += rng.choice((0., 0., 0.015, 0.03))
            action = rng.choice((START, START, END, END, SERVICE, SERVICE, TEST, OPEN, CLOSE,
                                 REMOVE, RESPAWN, DISABLE))
            trigger = rng.randrange(4)
            actor = rng.randrange(6)
            argument = rng.randrange(2)
            if action == SERVICE:
                trigger, argument = rng.randrange(2), rng.randrange(5)
            rows.append(op(action, trigger, actor, argument, current, rng.choice((0., 0., .02, .1))))
        yield 'mixed transitions', [*rows, op(SERVICE, current=current+1)]


def reference_model(steps, metrics):
    current_actors = {i: (i, 0) for i in range(6)}
    clients = [True]*5+[False]
    actor_serials = [0]*6
    contacts = [set() for _ in TRIGGERS]
    disabled = set()
    events = []
    serial = itertools.count()

    def queue(target, command, when):
        heapq.heappush(events, (when, next(serial), target, command))

    for action, trigger, actor, arg, now, delay in steps:
        dispatched = []
        credits = 0
        if action == SERVICE:
            credits = arg
            while events and events[0][0] <= now and (not trigger or credits > 0):
                _, _, receiver, command = heapq.heappop(events)
                dispatched.append(1 + 4*receiver + command)
                metrics['deliveries'] += 1
                if command == 2:
                    if receiver in disabled:
                        metrics['disabled_touch_tests_suppressed'] += 1
                    elif contacts[receiver]:
                        metrics['touch_tests_with_contact'] += 1
                    elif receiver >= 2:
                        metrics['empty_touch_tests_enqueued_close'] += 1
                        queue(receiver+4, 1, now)
                if trigger:
                    credits -= 1
        elif action == START and actor in current_actors:
            if clients[actor]:
                metrics['duplicate_starts'] += current_actors[actor] in contacts[trigger]
                contacts[trigger].add(current_actors[actor])
                queue(trigger+4, 0, now)
            else:
                metrics['nonclients_rejected'] += 1
        elif action == END and current_actors.get(actor) in contacts[trigger]:
            contacts[trigger].remove(current_actors[actor])
            remaining = contacts[trigger].intersection(current_actors.values())
            metrics['stale_contacts_pruned'] += len(contacts[trigger])-len(remaining)
            contacts[trigger] = remaining
            if trigger >= 2:
                queue(trigger, 2, now)
            elif not remaining:
                queue(trigger+4, 1, now)
        elif action == REMOVE:
            current_actors.pop(actor, None)
        elif action == RESPAWN:
            actor_serials[actor] += 1
            current_actors[actor] = actor, actor_serials[actor]
            clients[actor] = bool(arg)
        elif action in (TEST, OPEN, CLOSE):
            recipient = trigger if action == TEST else trigger+4
            queue(recipient, {TEST: 2, OPEN: 0, CLOSE: 1}[action], f32(now+delay))
        elif action == DISABLE:
            if arg:
                disabled.add(trigger)
            else:
                disabled.discard(trigger)
        assert len(dispatched) <= 32
        yield (*map(len, contacts), len(events), credits, *dispatched, *([0]*(32-len(dispatched))))


def verify_door_delivery(run, door_build, scenarios):
    from verify_sliding_door_motion import (
        HEADER, STEP as MOTION_STEP, REPLY as MOTION_REPLY, benchmark_endpoints, run_reference,
    )
    benchmark = {name: (closed, opened, speed)
                 for name, _, closed, opened, speed, _ in benchmark_endpoints(door_build, run)}
    results = {}
    motion_count = 0
    for arch in ('x86', 'x64'):
        dispatch = iter(REPLY.iter_unpack((run/f'{arch}-1.bin').read_bytes()))
        wire = bytearray()
        sizes = []
        for label, steps in scenarios:
            replies = [next(dispatch) for _ in steps]
            if label != 'clocked doors':
                continue
            trigger = steps[0][1]
            closed, opened, speed = benchmark[DOORS[trigger]]
            moves = []
            last_physics_time = None
            for step, reply in zip(steps, replies):
                if step[0] != SERVICE:
                    continue
                if last_physics_time is not None:
                    moves.append((0, f32(step[4]-last_physics_time)))
                last_physics_time = step[4]
                open_code = 1+4*(trigger+4)
                for command in reply[6:]:
                    if command in (open_code, open_code+1):
                        moves.append((command-open_code+1, 0.))
            assert [action for action, _ in moves if action] == [1, 1, 2]
            sizes.append(len(moves))
            wire.extend(HEADER.pack(*closed, *opened, speed, 0, 0, len(moves)))
            wire.extend(b''.join(MOTION_STEP.pack(*move) for move in moves))
        motion_count = sum(sizes)
        queries = run/f'queued-motion-{arch}.bin'
        reply = run/f'queued-motion-{arch}-result.bin'
        queries.write_bytes(wire)
        executable = door_build/f'motion_{arch}.exe'
        run_reference([executable, queries, reply, SERVER if arch == 'x86' else '-'],
                      run/f'queued-motion-{arch}.log')
        results[arch] = reply.read_bytes()
        rows = iter(MOTION_REPLY.iter_unpack(results[arch]))
        for name, size in zip(DOORS, sizes):
            states = [next(rows) for _ in range(size)]
            assert {row[-1] for row in states} == {0, 1, 2, 3}
            assert states[-1][:3] == benchmark[name][0]
        assert next(rows, None) is None
    assert results['x86'] == results['x64']
    return {'benchmark_doors': len(DOORS), 'matched_motion_steps': motion_count}


def main(build, door_build=None):
    assert sha(SERVER) == SERVER_SHA and sha(BSP) == BSP_SHA
    run = Path(tempfile.mkdtemp(prefix='trigger-', dir=build))
    routes = bsp_routes()
    image, evidence = verify_installed_ranges(SERVER, run/'trigger-reference.local.txt', [
        ('Client-only filter', 0x102563e0, 0x10256584),
        ('StartTouch and duplicate handling', 0x1025c410, 0x1025c4fa),
        ('EndTouch and invalid handle cleanup', 0x102588f0, 0x10258a38),
        ('TouchTest registration', 0x1025bf92, 0x1025bfc3),
        ('TouchTest virtual input and gate', 0x10254cd0, 0x10254cd8),
        ('TouchTest output selection', 0x102561f0, 0x10256229),
        ('Output actions enqueue named input', 0x100eae20, 0x100eaf24),
        ('Absolute event time and stable insertion', 0x100e9860, 0x100e9962),
        ('Queue limits, unlink and restart from head', 0x100e9f7b, 0x100ea005),
    ])
    assert [image.word(0x104e2a2c+offset) for offset in (0x198, 0x1a0, 0x330, 0x340, 0x344, 0x348)] == [
        0x1025c410, 0x102588f0, 0x102561f0, 0x102563e0, 0x10254310, 0x10254330]
    metrics = Counter()
    wire, target, labels = bytearray(), bytearray(), []
    scenarios = list(sequences())
    for label, steps in scenarios:
        wire.extend(struct.pack('<I', len(steps)))
        for step, expected in zip(steps, reference_model(steps, metrics)):
            labels.append((label, step))
            wire.extend(STEP.pack(*step))
            target.extend(REPLY.pack(*expected))
    queries = run/'contacts.bin'
    queries.write_bytes(wire)
    builds = {}
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = build/f'trigger_{arch}.exe'
        raw = exe.read_bytes()
        offset, = struct.unpack_from('<I', raw, 0x3c)
        assert raw[offset:offset+4] == b'PE\0\0' and struct.unpack_from('<H', raw, offset+4)[0] == machine
        log = (build/f'trigger_{arch}-build.log').read_text()
        assert 'warning ' not in log and 'error ' not in log
        builds[arch] = {'executable': str(exe), 'sha256': sha(exe), 'warnings': 0}
        for repeat in (1, 2):
            dest = run/f'{arch}-{repeat}.bin'
            result = subprocess.run([exe, queries, dest, SERVER if arch == 'x86' else '-'],
                                    capture_output=True, timeout=45)
            (run/f'{arch}-{repeat}.log').write_bytes(result.stdout+result.stderr)
            assert result.returncode == 0, (arch, result.returncode, result.stderr)
            actual = dest.read_bytes()
            assert len(actual) == len(target)
            if actual != target:
                index = next(i for i, (a, b) in enumerate(zip(actual, target)) if a != b) // REPLY.size
                raise AssertionError((arch, index, labels[index],
                                      REPLY.unpack_from(actual, index*REPLY.size),
                                      REPLY.unpack_from(target, index*REPLY.size)))
    assert all(metrics.values()) and len(metrics) == 7, metrics
    assert sha(SERVER) == SERVER_SHA and sha(BSP) == BSP_SHA
    report = {
        'builds': builds, 'repeats_per_arch': 2, 'matched_scenarios': len(scenarios),
        'matched_step_rows': len(labels), 'metrics': dict(metrics), 'benchmark_routes': routes,
        'reference_ranges': evidence,
        'originals_unchanged': {str(SERVER): SERVER_SHA, str(BSP): BSP_SHA},
        'limitations': 'The fixture supplies contact notifications and named entity resolution. The original trigger and event queue execute in the x86 probe with controlled entity handles and logging callbacks. No live collision overlap generation or SFM playback.',
    }
    if door_build:
        report['trigger_to_motion_replay'] = verify_door_delivery(run, door_build, scenarios)
    (run/'results.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(f'Trigger I/O matched {len(labels)} rows across {len(scenarios)} scenarios.')
    print(run/'results.json')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    parser.add_argument('--door-build', type=Path)
    args = parser.parse_args()
    main(args.build.resolve(), args.door_build.resolve() if args.door_build else None)
