"""Check the contact-to-door runtime against three installed server boundaries."""

import argparse
from collections import Counter
import json
from pathlib import Path
import random
import re
import struct
import subprocess
import tempfile

import verify_map_trigger_io as io
import verify_sliding_door_motion as motion
import verify_translated_brush_box_trace as box

RUNTIME_REPLY = struct.Struct('<IIIi32I11fI')
TICK = struct.Struct('<ffIiI')
OP = struct.Struct('<III')
SCAN_START, CONTACT, SCAN_END = range(3)
SERVICE_SCANS = 4
ACTORS = (0, 1, 5)


def verify_map_loader(exe, run, endpoints):
    data = io.BSP.read_bytes()
    offset, size = struct.unpack_from('<II', data, 8)
    fields = [re.findall(rb'"([^"]+)"\s*"([^"]*)"', block)
              for block in re.findall(rb'\{[^{}]+\}', data[offset:offset+size])]
    by_model = {dict(entity).get(b'model', b'').decode(): entity for entity in fields}
    raw = subprocess.check_output([exe, '--inspect', io.BSP], text=True, timeout=30)
    rows = iter(raw.splitlines())
    assert next(rows) == 'unsupported 0 0'
    actions = 0
    for model, (closed, opened, speed) in endpoints.items():
        expected = [f'{key.decode()} {value.decode()}' for key, value in by_model[model]
                    if key.startswith(b'On')]
        tag, index, count, *pose = next(rows).split()
        assert (tag, '*'+index, int(count)) == ('door', model, len(expected))
        assert tuple(float.fromhex(value) for value in pose) == (*closed, *opened, speed)
        assert [next(rows) for _ in expected] == expected
        actions += len(expected)
    for index, (model, door) in enumerate(zip(io.TRIGGERS, io.DOORS)):
        assert model in by_model
        tag, trigger, target, mode, *origin = next(rows).split()
        assert (tag, '*'+trigger, '*'+target, int(mode)) == ('trigger', model, door, int(index>=2))
        assert tuple(map(float.fromhex, origin)) == tuple(io.f32(float(value))
                    for value in dict(by_model[model])[b'origin'].split())
    assert next(rows, None) is None

    door = b'{ "classname" "func_door" "model" "*1" "targetname" "Door" "spawnflags" "32" }'
    trigger = (b'{ "classname" "trigger_multiple" "model" "*2" "spawnflags" "1" "wait" "0" '
               b'"OnStartTouch" "door,Open,,0,-1" "OnEndTouchAll" "DOOR,Close,,0,-1" }')
    fixtures = [
        ('case-mixed-target', door+trigger, 'trigger 2 1 0', 0),
        ('escape-separator', door+trigger.replace(b',', b'\x1b'), 'trigger 2 1 0', 0),
        ('duplicate-target', door+trigger+b'{ "classname" "logic_relay" "targetname" "Door" }', 'unsupported 0 1', 0),
        ('delayed-input', door+trigger.replace(b'Open,,0', b'Open,,1'), 'unsupported 0 1', 0),
        ('named-filter', door+trigger.replace(b'"wait"', b'"filtername" "filter" "wait"'), 'unsupported 0 1', 0),
        ('disabled-trigger', door+trigger.replace(b'"wait"', b'"StartDisabled" "1" "wait"'), 'unsupported 0 1', 0),
        ('missing-close', door+trigger.replace(b'"OnEndTouchAll" "DOOR,Close,,0,-1"', b''), 'unsupported 0 1', 0),
        ('non-toggle', door.replace(b'"32"', b'"0"')+trigger, 'unsupported 1 1', 0),
        ('out-of-range-model', door.replace(b'"*1"', b'"*3"')+trigger, 'outside table', 1),
        ('broken-entity', door+trigger[:-2], '', 1),
    ]
    for label, text, marker, code in fixtures:
        header = bytearray(1036)
        struct.pack_into('<4sI', header, 0, b'VBSP', 20)
        struct.pack_into('<4I', header, 8, len(header), len(text)+1, 0, 0)
        struct.pack_into('<4I', header, 8+14*16, len(header)+len(text)+1, 3*48, 0, 0)
        path = run/f'{label}.bsp'
        path.write_bytes(header+text+b'\0'+bytes(3*48))
        result = subprocess.run([exe, '--inspect', path], capture_output=True, text=True, timeout=20)
        assert result.returncode == code and marker in result.stdout+result.stderr, (label, result)
        if marker.startswith('unsupported'):
            assert 'trigger ' not in result.stdout, (label, result)
    return dict(doors=len(endpoints), triggers=len(io.TRIGGERS), preserved_door_outputs=actions,
                controlled_cases=len(fixtures))


def cases():
    rng = random.Random(4158)
    for route in range(4):
        for variant in range(5):
            ticks = []
            # Include simultaneous actors, a non-client, a door reversal, and
            # a final full travel in each direction.
            occupancy = [(0,), (0, 1), (1,), (5,), (), (0,), (), (0,)] + [(0,)] * 40
            occupancy += [()] * 40
            for index, members in enumerate(occupancy):
                if variant and 12 <= index < 36:
                    members = tuple(actor for actor in ACTORS if rng.randrange(2))
                ops = []
                if (index + variant) % 4 == 0:
                    ops.append((SCAN_START, 2, 0))
                    ops.extend((CONTACT, actor, 2) for actor in members)
                    ops.append((SCAN_END, 2, 0))
                else:
                    for actor in ACTORS if variant % 2 == 0 else ACTORS[::-1]:
                        ops.append((SCAN_START, actor, 0))
                        if actor in members:
                            ops.append((CONTACT, actor, 2))
                        ops.append((SCAN_END, actor, 0))
                gated = 0 < index < 38 and index % 5 in (1, 2)
                ticks.append((index / 32, 1 / 32 if index else 0., int(gated), index % 2, ops))
            yield route, ticks


def collision_inputs(build, run, scenarios):
    """Use independent native brush queries to replace fixture contact flags."""
    models = box.placements()
    queries, owned, masks = bytearray(), [], []
    sweeps = []
    for route, ticks in scenarios:
        model = int(io.TRIGGERS[route][1:])
        origin, lo, hi, _ = models[model]
        midpoint = [io.f32(p+(a+b)*.5) for p,a,b in zip(origin,lo,hi)]
        far = [io.f32(origin[0]+hi[0]+128), *midpoint[1:]]
        far_other = [io.f32(origin[0]+lo[0]-128), *midpoint[1:]]
        for tick, (_, _, _, _, ops) in enumerate(ticks):
            wanted = {actor for opcode, actor, _ in ops if opcode == CONTACT}
            inputs = []
            for actor in ACTORS:
                mins, maxs = (-24., -24., 0.), (24., 24., 82. if actor == 0 else 62.)
                center_offset = [(a+b)*.5 for a,b in zip(mins,maxs)]
                radius = [(b-a)*.5 for a,b in zip(mins,maxs)]
                end_center = midpoint if actor in wanted else far
                swept = tick in (5, 7, 17, 28) and actor in (0, 5)
                start_center = far_other if swept else end_center
                if tick == 28 and actor == 5:
                    # Move across the trigger's x range on a parallel path
                    # well outside the brush on the y axis.
                    start_center = [start_center[0], io.f32(origin[1]+hi[1]+128), start_center[2]]
                    end_center = [end_center[0], start_center[1], end_center[2]]
                current = [io.f32(x-y) for x,y in zip(end_center,center_offset)]
                previous = [io.f32(x-y) for x,y in zip(start_center,center_offset)]
                # Recreate Ray_t inputs independently using float32 rounding.
                center = [io.f32(x+y) for x,y in zip(previous,center_offset)]
                delta = [io.f32(x-y) for x,y in zip(current,previous)]
                queries.extend(box.QUERY.pack(*center,*delta,box.TRIGGER_MASK,int(swept),1,
                    model,*origin,*[-v for v in center_offset],*radius))
                inputs.append(OP.pack(3, actor, int(swept)) +
                    struct.pack('<12f', *current, *previous, *mins, *maxs))
                sweeps.append(swept)
            owned.append(b''.join(inputs))
    (run/'brush-queries.bin').write_bytes(queries)
    motion.run_reference([build/'brush_box_x86.exe', io.BSP, box.MESHES, run/'brush-queries.bin',
        run/'brush-hits.bin', box.ENGINE], run/'brush-native.log')
    trace = list(box.TRACE.iter_unpack((run/'brush-hits.bin').read_bytes()))
    assert len(trace) == len(sweeps)
    stats = Counter()
    filtered = iter(trace)
    for swept, row in zip(sweeps, trace):
        stats[('swept' if swept else 'stationary') +
              ('_hits' if row[4]&box.TRIGGER_MASK else '_misses')] += 1
    replaced = []
    for route, ticks in scenarios:
        rows = []
        for now, dt, gate, credits, _ in ticks:
            ops = []
            hit_mask = 0
            for actor in ACTORS:
                ops.append((SCAN_START, actor, 0))
                if next(filtered)[4]&box.TRIGGER_MASK:
                    ops.append((CONTACT, actor, 2))
                    hit_mask |= 1<<actor
            ops.append((SERVICE_SCANS, 0, 0))
            masks.append(hit_mask)
            rows.append((now, dt, gate, credits, ops))
        replaced.append((route,rows))
    assert all(stats[label] for label in ('swept_hits', 'swept_misses', 'stationary_hits', 'stationary_misses'))
    return replaced, iter(owned), iter(masks), dict(stats)


def main(build, collision=False):
    assert io.sha(io.SERVER) == io.SERVER_SHA and io.sha(io.BSP) == io.BSP_SHA
    run = Path(tempfile.mkdtemp(prefix='runtime-', dir=build))
    routes = io.bsp_routes()
    endpoints = {name: (closed, opened, speed) for name, _, closed, opened, speed, _
                 in motion.benchmark_endpoints(build, run)}
    loader = verify_map_loader(build/'runtime_x64.exe', run, endpoints)
    scenarios = list(cases())
    mask_values = None
    trace_metrics = {}
    if collision:
        assert all(io.sha(path)==digest for path,digest in box.PINS.items())
        scenarios, owned_scans, mask_values, trace_metrics = collision_inputs(build, run, scenarios)
    contact_input, owned_input = [], bytearray(struct.pack('<I', len(scenarios)))
    for route, ticks in scenarios:
        contact_input.append('r 0 0\n')
        contact_input.extend(f's {actor} 1\n' for actor in ACTORS)
        contact_input.append('s 2 2\n')
        owned_input.extend(struct.pack('<II', int(io.TRIGGERS[route][1:]), len(ticks)))
        for now, dt, gate, credits, ops in ticks:
            owned_input.extend(TICK.pack(now, dt, gate, credits, len(ACTORS) if collision else len(ops)))
            if collision:
                owned_input.extend(next(owned_scans))
            for opcode, a, b in ops:
                kind = 'u' if opcode==SERVICE_SCANS else 'bcf'[opcode]
                contact_input.append(f'{kind} {a} {b}\n')
                if not collision:
                    owned_input.extend(OP.pack(opcode, a, b))
    (run/'scans.txt').write_text(''.join(contact_input), encoding='ascii')
    native_queue = ['--queue'] if collision else []
    motion.run_reference([build/'touch_x86.exe', run/'scans.txt', run/'contacts.txt', io.SERVER, *native_queue],
                         run/'contacts.log')
    native_rows = iter((run/'contacts.txt').read_text().splitlines())
    io_input, service_points = bytearray(), []
    for route, ticks in scenarios:
        for _ in range(5):
            next(native_rows)
        commands, markers = [], []
        for now, _, gate, credits, ops in ticks:
            for _ in ops:
                for event in next(native_rows).split('|')[0].split():
                    kind, owner, peer = map(int, event.split(':'))
                    if owner == 0x10002 and kind in (1, 3):
                        commands.append(io.op(io.START if kind == 1 else io.END, route, peer & 0xffff, current=now))
            markers.append(len(commands))
            commands.append(io.op(io.SERVICE, gate, argument=credits, current=now))
        io_input.extend(struct.pack('<I', len(commands)))
        io_input.extend(b''.join(io.STEP.pack(*command) for command in commands))
        service_points.append((len(commands), markers))
    assert next(native_rows, None) is None
    (run/'io.bin').write_bytes(io_input)
    motion.run_reference([build/'trigger_x86.exe', run/'io.bin', run/'io-result.bin', io.SERVER], run/'io.log')
    io_rows = iter(io.REPLY.iter_unpack((run/'io-result.bin').read_bytes()))
    physics_input, expected_ticks = bytearray(), []
    metrics = Counter()
    for (route, ticks), (size, markers) in zip(scenarios, service_points):
        states = [next(io_rows) for _ in range(size)]
        closed, opened, speed = endpoints[io.DOORS[route]]
        movements = []
        for marker, (_, dt, _, _, _) in zip(markers, ticks):
            reply = states[marker]
            deliveries = []
            movements.append((0, dt))
            for raw in reply[6:]:
                if not raw:
                    continue
                target, opcode = divmod(raw-1, 4)
                assert target == (route if opcode == 2 else route+4), (route, raw)
                deliveries.append(opcode+1)
                metrics[('open', 'close', 'touch_test')[opcode]] += 1
                if opcode < 2:
                    movements.append((opcode+1, 0.))
            pose_index = metrics['motion_rows'] + len(movements) - 1
            expected_ticks.append((route, reply[route], reply[4:6], deliveries, pose_index))
        physics_input.extend(motion.HEADER.pack(*closed, *opened, speed, 0, 0, len(movements)))
        physics_input.extend(b''.join(motion.STEP.pack(*move) for move in movements))
        metrics['motion_rows'] += len(movements)
    assert next(io_rows, None) is None
    (run/'motion.bin').write_bytes(physics_input)
    motion.run_reference([build/'motion_x86.exe', run/'motion.bin', run/'motion-result.bin', io.SERVER], run/'motion.log')
    physics_rows = list(motion.REPLY.iter_unpack((run/'motion-result.bin').read_bytes()))
    assert len(physics_rows) == metrics['motion_rows']
    (run/'runtime.bin').write_bytes(owned_input)
    expected = bytearray()
    phases = {route: set() for route in routes}
    for route, touching, (pending, credits), events, index in expected_ticks:
        frame = physics_rows[index]
        phases[io.TRIGGERS[route]].add(frame[-1])
        mask = next(mask_values) if mask_values is not None else 0
        expected.extend(RUNTIME_REPLY.pack(touching, pending, mask, credits, *events, *([0]*(32-len(events))), *frame))
    assert all(states == {0, 1, 2, 3} for states in phases.values()), phases
    assert metrics['touch_test'] and metrics['close'] and metrics['open']
    for i in range(2):
        output = run/f'runtime-x64-{i}.bin'
        options = ['--collision'] if collision else []
        motion.run_reference([build/'runtime_x64.exe', run/'runtime.bin', output, io.BSP, *options],
                             run/f'runtime-x64-{i}.log')
        actual = output.read_bytes()
        assert len(actual) == len(expected)
        if actual != expected:
            mismatch = next(j for j, (a, b) in enumerate(zip(actual, expected)) if a != b) // RUNTIME_REPLY.size
            offset = mismatch * RUNTIME_REPLY.size
            raise AssertionError((run, mismatch, RUNTIME_REPLY.unpack_from(actual, offset),
                                  RUNTIME_REPLY.unpack_from(expected, offset)))
    assert io.sha(io.SERVER) == io.SERVER_SHA and io.sha(io.BSP) == io.BSP_SHA
    if collision:
        assert all(io.sha(path)==digest for path,digest in box.PINS.items())
    result = dict(benchmark_routes=routes, loader=loader, scenarios=len(scenarios), matched_ticks=len(expected_ticks),
                  deliveries=dict(metrics), native_contact_fixture_steps=len(contact_input),
                  x64_executable=str(build/'runtime_x64.exe'), x64_sha256=io.sha(build/'runtime_x64.exe'),
                  warnings=(build/'runtime_x64-build.log').read_text().count('warning '),
                  original_server_and_map_unchanged=True,
                  brush_traces=trace_metrics,
                  scope='Supplied actor positions and service boundaries. Native contact expiry queue comparison.'
                    if collision else 'Supplied overlaps and timing. No collision, scheduler, or playback.')
    (run/'results.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print(run/'results.json', len(expected_ticks), dict(metrics))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    parser.add_argument('--collision', action='store_true')
    args = parser.parse_args()
    main(args.build.resolve(), args.collision)
