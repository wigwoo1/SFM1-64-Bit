"""Replay switchable, unobstructed sliding doors against the installed server."""

from collections import Counter
import json
import math
from pathlib import Path
import random
import struct
import subprocess
import sys
import tempfile

from verify_sliding_door_spawn import (
    BSP, REPLY as SPAWN_REPLY, SERVER, f32, read_map_doors, sha, verify_installed_ranges,
)
from verify_translated_brush_point_trace import (
    ENGINE, MESHES, QUERY as TRACE_QUERY, TRACE, benchmark_doors,
)

HEADER = struct.Struct('<7f3I')
STEP = struct.Struct('<If')
REPLY = struct.Struct('<11fI')
SERVER_SHA = '47a03f6bce8f519dafe1cfbcb9ba97cf0b0069d6eab57c2a52cffd61c66265d4'
BSP_SHA = '60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea'
ENGINE_SHA = 'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
MESHES_SHA = '961a7b203c61fd17c58e0c184e9d244da4748a4be74377762971cca0054351dd'
OPEN, CLOSE, LOCK, UNLOCK = ((n, 0.) for n in range(1, 5))


def run_reference(argv, path):
    result = subprocess.run(argv, capture_output=True, timeout=50)
    path.write_bytes(result.stdout+result.stderr)
    assert result.returncode == 0, (result.returncode, path)


def benchmark_endpoints(build, run):
    doors, inputs = read_map_doors()
    queries = run/'benchmark-spawn.bin'
    queries.write_bytes(b''.join(wire for _, _, wire in doors))
    native = run/'benchmark-spawn-x86.bin'
    owned = run/'benchmark-spawn-x64.bin'
    run_reference([build/'door_x86.exe', SERVER, queries, native], run/'spawn-x86.log')
    run_reference([build/'door_x64.exe', '-', queries, owned], run/'spawn-x64.log')
    assert native.read_bytes() == owned.read_bytes()
    rows = list(SPAWN_REPLY.iter_unpack(native.read_bytes()))
    assert len(rows) == len(doors) == 5
    for (model, name, _), row in zip(doors, rows):
        assert row[:3] == row[6:9]
        assert row[12:] == (300., 1, 0, 6, 7)
        yield model, name, row[:3], row[3:6], row[12], sorted(key for key in inputs if key.startswith(model+':'))


def cases(benchmark):
    for model, _, closed, opened, speed, _ in benchmark:
        label = f'benchmark {model}'
        for dt in map(f32, [0.015, 0.03, 1/24, 1/66, 0.125]):
            piece = [(0, dt)] * (math.ceil(0.5/dt)+2)
            yield label, closed, opened, speed, 0, 0, [
                CLOSE, OPEN, OPEN, (0, 0.), *piece, OPEN, LOCK, CLOSE, *piece,
                OPEN, UNLOCK, OPEN, *piece,
            ]
            yield label, closed, opened, speed, 0, 0, [
                OPEN, (0, dt), CLOSE, (0, dt/2), CLOSE, OPEN, (0, dt), LOCK,
                CLOSE, OPEN, UNLOCK, OPEN, *piece, CLOSE, *(piece*2),
            ]
            yield label, closed, opened, speed, 1, 1, [OPEN, CLOSE, *piece, OPEN, UNLOCK, OPEN, *piece]
        # Both directions at exactly their endpoint get a command before physics.
        yield label, closed, opened, speed, 0, 0, [OPEN, CLOSE, OPEN, (0, 5.)]

    yield 'zero length', (10., 20., -5.), (10., 20., -5.), 300., 0, 0, [OPEN, CLOSE, LOCK, OPEN, (0, .125)]
    rng = random.Random(921707)
    for _ in range(128):
        closed = tuple(f32(rng.uniform(-8000, 8000)) for _ in range(3))
        opened = tuple(f32(p + rng.uniform(-180, 180)) for p in closed)
        speed = f32(rng.uniform(50, 600))
        actions = [rng.choice((OPEN, CLOSE, LOCK, UNLOCK, (0, f32(.015)), (0, f32(.03)), (0, 2.))) for _ in range(40)]
        yield 'oblique translation', closed, opened, speed, rng.randrange(2), rng.randrange(2), actions


def calculate(sequence):
    _, closed, opened, speed, initially_open, locked, actions = sequence
    origin = opened if initially_open else closed
    stopped = (0., 0., 0.)
    velocity = destination = stopped
    phase = 0 if initially_open else 1
    local_clock, deadline = 0., -1.
    for action, duration in actions:
        if action in (3, 4):
            locked = action == 3
        elif (action == 1 and not locked and phase in (1, 3)) or (action == 2 and phase != 1):
            destination = opened if action == 1 else closed
            phase = action+1
            if origin == destination:
                phase -= 2
                deadline = -1.
                velocity = stopped
            else:
                difference = [f32(b-a) for a, b in zip(origin, destination)]
                squares = [f32(d*d) for d in difference]
                travel = f32(f32(math.sqrt(f32(f32(squares[0]+squares[1])+squares[2])))/speed)
                deadline = f32(local_clock+travel)
                reciprocal = f32(1/travel)
                new_velocity = tuple(f32(d*reciprocal) for d in difference)
                if new_velocity != velocity:
                    velocity = new_velocity
        elif action == 0 and deadline >= 0.:
            dt = min(duration, f32(deadline-local_clock))
            if dt > 0.:
                origin = tuple(f32(p+f32(v*dt)) for p, v in zip(origin, velocity))
                local_clock = f32(local_clock+dt)
            if deadline > 0. and local_clock >= deadline:
                origin, velocity, deadline, phase = destination, stopped, -1., phase-2
        yield (*origin, *destination, *velocity, local_clock, deadline, phase)


def verify_collision(run, brush_build, benchmark, labels):
    assert sha(ENGINE) == ENGINE_SHA and sha(MESHES) == MESHES_SHA
    geometry = {number: (mins, maxs) for number, _, mins, maxs, _ in benchmark_doors()}
    outputs = {}
    keys = []
    for role, arch in [('native', 'x86'), ('owned', 'x64')]:
        poses = list(REPLY.iter_unpack((run/f'{arch}-1.bin').read_bytes()))
        rays = bytearray()
        role_keys = []
        for name, _, closed, opened, _, _ in benchmark:
            model = int(name[1:])
            mins, maxs = geometry[model]
            sampled = {pose[:3] for (label, _, _), pose in zip(labels, poses)
                       if label == f'benchmark {name}'}
            assert closed in sampled and opened in sampled
            lower = min(closed[2], opened[2])+mins[2]
            upper = max(closed[2], opened[2])+maxs[2]
            axis = min(range(2), key=lambda i: maxs[i]-mins[i])
            delta = [0., 0., 0.]
            delta[axis] = maxs[axis]-mins[axis]+32.
            stride = max(1, len(sampled)//18)
            positions = sorted({*sorted(sampled)[::stride], closed, opened})
            for position in positions:
                for level in range(1, 24):
                    origin = [closed[i]+(mins[i]+maxs[i])/2 for i in range(3)]
                    origin[axis] = closed[axis]+mins[axis]-16.
                    origin[2] = lower+(upper-lower)*level/24
                    rays.extend(TRACE_QUERY.pack(*origin, *delta, 0x0200400b, 1, 1, model, *position))
                    role_keys.append((model, level))
        if keys:
            assert keys == role_keys
        else:
            keys = role_keys
        query = run/f'collision-{role}-queries.bin'
        query.write_bytes(rays)
        out = run/f'collision-{role}.bin'
        exe = brush_build/f'translated_{role}_{arch}.exe'
        run_reference([exe, BSP, MESHES, query, out, ENGINE], run/f'collision-{role}.log')
        outputs[role] = out.read_bytes()
    assert outputs['native'] == outputs['owned'] and len(outputs['owned']) == len(keys)*TRACE.size
    types = {}
    for key, row in zip(keys, TRACE.iter_unpack(outputs['owned'])):
        types.setdefault(key, set()).add(row[0] < 1)
    changing = Counter(model for (model, _), observed in types.items() if len(observed) == 2)
    assert all(changing[int(name[1:])] > 0 for name, *_ in benchmark), changing
    assert sha(ENGINE) == ENGINE_SHA and sha(MESHES) == MESHES_SHA
    return {'matched_traces': len(keys), 'fixed_rays_that_changed_hit_state_by_door': dict(changing)}


def main(build, brush_build=None):
    assert sha(SERVER) == SERVER_SHA and sha(BSP) == BSP_SHA
    run = Path(tempfile.mkdtemp(prefix='motion-', dir=build))
    image, ranges = verify_installed_ranges(SERVER, run/'pusher-reference.local.txt', [
        ('InputOpen', 0x101026a0, 0x101026da),
        ('InputClose', 0x10101e60, 0x10101e71),
        ('Sliding Close path', 0x10101dd0, 0x10101e5a),
        ('Sliding Open state', 0x10102098, 0x101020bd),
        ('Sliding Open path', 0x101023bb, 0x101023f6),
        ('LinearMove', 0x102304f0, 0x10230637),
        ('Set local movement deadline', 0x100b5c00, 0x100b5c47),
        ('Pusher clamps requested duration', 0x10199040, 0x1019909e),
        ('Linear pusher advances its local clock', 0x10198be0, 0x10198c74),
        ('Linear position update', 0x101969b0, 0x10196a7f),
        ('Pusher dispatches completion', 0x10198ff1, 0x10199038),
        ('LinearMoveDone', 0x102302e0, 0x10230354),
        ('Door reaches open endpoint', 0x10101fb5, 0x10102043),
        ('Door reaches closed endpoint', 0x10101c95, 0x10101d59),
        ('Entity cell width initialization', 0x100c4800, 0x100c480a),
    ])
    for descriptor, name, address in [
            (0x1059aa48, 'Open', 0x101026a0), (0x1059aa84, 'Close', 0x10101e60)]:
        assert image.string(image.word(descriptor+0xc)) == name
        assert image.word(descriptor+0x14) == address
    assert image.word(0x10474bbc+0xc4) == 0x102302e0
    assert image.word(0x10474bbc+0x320) == 0x102c93c0
    benchmark = list(benchmark_endpoints(build, run))
    records = list(cases(benchmark))
    labels, expected, wire = [], bytearray(), bytearray()
    route_counts = Counter()
    for record in records:
        label, closed, opened, speed, initially_open, locked, actions = record
        wire.extend(HEADER.pack(*closed, *opened, speed, initially_open, locked, len(actions)))
        for action, predicted in zip(actions, calculate(record)):
            wire.extend(STEP.pack(*action))
            expected.extend(REPLY.pack(*predicted))
            labels.append((label, action, predicted))
            route_counts[str(predicted[-1])] += 1
    query = run/'sequence.bin'
    query.write_bytes(wire)
    builds = {}
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = build/f'motion_{arch}.exe'
        raw = exe.read_bytes()
        offset, = struct.unpack_from('<I', raw, 0x3c)
        assert raw[offset:offset+4] == b'PE\0\0' and struct.unpack_from('<H', raw, offset+4)[0] == machine
        log = (build/f'motion_{arch}-build.log').read_text()
        assert 'warning ' not in log and 'error ' not in log
        builds[arch] = {'executable': str(exe), 'sha256': sha(exe), 'warnings': 0}
        for repeat in (1, 2):
            reply = run/f'{arch}-{repeat}.bin'
            run_reference([exe, query, reply, SERVER if arch == 'x86' else '-'], run/f'{arch}-{repeat}.log')
            actual = reply.read_bytes()
            if actual != expected:
                assert len(actual) == len(expected)
                index = next(i for i, (actual_byte, expected_byte) in
                             enumerate(zip(actual, expected)) if actual_byte != expected_byte) // REPLY.size
                raise AssertionError((arch, index, labels[index], REPLY.unpack_from(actual, index*REPLY.size)))
    assert sha(SERVER) == SERVER_SHA and sha(BSP) == BSP_SHA
    report = {
        'builds': builds,
        'matched_sequences': len(records),
        'matched_step_rows': len(labels),
        'matched_steps_by_output_state': dict(route_counts),
        'repeats_per_arch': 2,
        'benchmark_brushes_checked': benchmark,
        'originals_unchanged': {str(SERVER): SERVER_SHA, str(BSP): BSP_SHA},
        'reference_ranges': ranges,
        'limitations': 'Parentless toggle doors with zero rotation. Inputs and frame durations are controlled. Audio, think registration, and collision bookkeeping have controlled callbacks. No live trigger overlap, obstruction, or editor integration.',
    }
    if brush_build:
        report['resolved_pose_collision'] = verify_collision(run, brush_build, benchmark, labels)
    (run/'results.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(f'Door motion matched {len(labels)} steps across {len(records)} sequences.')
    print(run/'results.json')


if __name__ == '__main__':
    main(Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else None)
