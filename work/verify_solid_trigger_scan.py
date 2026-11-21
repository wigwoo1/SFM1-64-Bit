"""Compare SolidMoved scan and contact dispatch with the installed engine."""

from collections import Counter
from itertools import product
import json
from pathlib import Path
import random
import re
import struct
import subprocess
import sys
import tempfile

from verify_sliding_door_spawn import BSP, ENGINE, SERVER, f32, sha, verify_installed_ranges

PIN = {
    ENGINE: 'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f',
    SERVER: '47a03f6bce8f519dafe1cfbcb9ba97cf0b0069d6eab57c2a52cffd61c66265d4',
    BSP: '60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea',
}
HEADER = struct.Struct('<18f5I')
ENTRY = struct.Struct('<4I6f')
REPLY = struct.Struct('<12f37I')
MASK = 0x0200400b
LOCAL_HULL = (-2, -3, -1, 3, 2, 4)
ZERO = (0, 0, 0)


def translated(box, origin):
    return tuple(f32(x + origin[i % 3]) for i,x in enumerate(box))


def benchmark_boxes():
    data = BSP.read_bytes()
    offset, size = struct.unpack_from('<2I', data, 8)
    models, length = struct.unpack_from('<2I', data, 8 + 14*16)
    assert data[:4] == b'VBSP' and length % 48 == 0
    boxes = {}
    for entity in re.findall(rb'\{[^{}]+\}', data[offset:offset+size]):
        values = dict(re.findall(rb'"([^"]+)"\s*"([^"]*)"', entity))
        name = values.get(b'model')
        if name not in (b'*2', b'*4', b'*6', b'*9'):
            continue
        assert values[b'classname'] == b'trigger_multiple' and values[b'spawnflags'] == b'1'
        assert b'angles' not in values and b'parentname' not in values
        model = int(name[1:])
        origin = tuple(f32(float(x)) for x in values[b'origin'].split())
        raw = struct.unpack_from('<6f', data, models + model*48)
        boxes[model] = translated(raw, origin)
    assert sorted(boxes) == [2, 4, 6, 9]
    return boxes


def fixture_rows(boxes):
    rng = random.Random(2026112104)
    hits = ((424, 0x80, 0, 0, (0, -2, -3, 2, 2, 3)),)
    far = tuple(100 + i for i in range(6))
    for origin, previous, moved, accurate, solid in product(
        (ZERO, (-4, 0, 0), (8, 0, 0), (8, 4, -8)),
        (ZERO, (-8, 0, 0), (8, -4, 8)), range(2), range(2), (1, 2, 3),
    ):
        candidates = list(hits) + [
            (917, 128, 0, 0, far),
            (432, 128, 1, 0, far),
            (41, 0, 2, MASK, far),
            (21, 0, 3, MASK, far),
            (70, 0, 0, 1, far),
            (81, 4, 0, MASK, far),
            (23, 0, 0, 16, far),
            (12, 0, 0, 0x02000000, far),
        ]
        for blocked in (0, 4, 128):
            yield 'branch-and-filter', origin, previous, LOCAL_HULL, translated((-4,-5,-3,4,5,3),origin), \
                moved, solid, accurate, blocked, candidates
    for moved, count in product(range(2), (0, 1, 8, 9, 16, 17, 32)):
        ids = rng.sample(range(1, 800), count)
        yield 'contact-order', ZERO, ZERO, LOCAL_HULL, LOCAL_HULL, moved, 2, 1, 0, [
            (key, 0, 0, MASK, far) for key in ids]

    for gap in (-1, -0.125, 0, 0.125, 1):
        box = (3+gap, -1, 1, 4+gap, 1, 3)
        yield 'stationary-face', ZERO, ZERO, LOCAL_HULL, LOCAL_HULL, 0, 2, 1, 0, [(5,128,0,0,box)]
    for gap in (-0.125, 0, 0.125):
        box = (11+gap, -1, 1, 12+gap, 1, 3)
        yield 'swept-face', (8,0,0), ZERO, LOCAL_HULL, translated(LOCAL_HULL,(8,0,0)), 1, 2, 1, 0, [(4,128,0,0,box)]
    yield 'swept-crosses-gap', (8,0,0), (-8,0,0), LOCAL_HULL, translated(LOCAL_HULL,(8,0,0)), 1, 2, 1, 0, hits
    yield 'same-pose-misses-gap', (8,0,0), ZERO, LOCAL_HULL, translated(LOCAL_HULL,(8,0,0)), 0, 2, 1, 0, hits

    for extent in (0, 1e-6, 0.00099, 0.00101, 0.01, 1, 4):
        for delta in (0, 1e-23, 1e-19, 0.0002, 3):
            pose = (f32(delta), 0, 0)
            local = (-f32(extent),0,0,f32(extent),0,0)
            yield 'point-and-small-sweep', pose, ZERO, local, translated(local,pose), 1, 2, 1, 0, hits

    # Use the map bounds and placement of all four benchmark triggers in
    # controlled fixtures with the special trigger-bounds flag.
    candidates = [(key, 128, 0, 0, box) for key,box in boxes.items()]
    for model, (xmin,ymin,zmin,xmax,ymax,zmax) in boxes.items():
        center = tuple(f32((a+b)*0.5) for a,b in zip((xmin,ymin,zmin),(xmax,ymax,zmax)))
        for moved in (0, 1):
            for margin in (-0.25, 0, 0.25, 512):
                for axis in range(3):
                    pose = list(center)
                    pose[axis] = f32((xmax,ymax,zmax)[axis] - LOCAL_HULL[axis] + margin)
                    prior = list(center)
                    prior[axis] = f32((xmin,ymin,zmin)[axis] - 128)
                    yield f'benchmark-box-{model}', pose, prior, LOCAL_HULL, translated(LOCAL_HULL,pose), \
                        moved, 2, 1, 0, candidates

    def point():
        return tuple(rng.randrange(-3000,3001)/8 for _ in range(3))

    for _ in range(2200):
        origin = point()
        prior = rng.choice((origin, point()))
        extent = tuple(rng.randrange(1,160)/8 for _ in range(3))
        local = tuple(-a for a in extent) + extent
        surrounding = translated(tuple(-a-1 for a in extent)+tuple(a+1 for a in extent), origin)
        entries = []
        for key in rng.sample(range(1, 999), 16):
            center = rng.choice((origin, prior, point()))
            radius = tuple(rng.randrange(1,160)/8 for _ in range(3))
            bounds = translated(tuple(-v for v in radius)+radius, center)
            entries.append((key, rng.choice((0, 0x80, 8, 136, 4, 132)), rng.choice((0,0,0,0,1,2,3)),
                            rng.choice((0, 1, 2, 4, 0x4000, 0x2000000, 16, 256)), bounds))
        yield 'varied', origin, prior, local, surrounding, rng.randrange(2), rng.choice((1, 2, 3)), \
            rng.randrange(2), rng.choice((0,4,128)), entries


def predict(row, counts):
    name, origin, previous, local, surrounding, moved, solid, accurate, blocked, targets = row
    start = previous if moved else origin
    low, high = (local[:3], local[3:]) if accurate and solid == 2 else (
        [f32(surrounding[i]-origin[i]) for i in range(3)],
        [f32(surrounding[i+3]-origin[i]) for i in range(3)])
    midpoint = [f32(f32(a+b)*0.5) for a,b in zip(low,high)]
    extents = [f32(f32(b-a)*0.5) for a,b in zip(low,high)]
    center = [f32(a+b) for a,b in zip(start,midpoint)]
    delta = [f32(b-a) for a,b in zip(start,origin)]
    is_ray = sum(x*x for x in extents) < 1e-6
    squared = [f32(d*d) for d in delta]
    swept = f32(f32(squared[0]+squared[1])+squared[2]) != 0
    query_kind = 2 if moved else 1
    counts[name] += 1
    counts['box-query' if not moved else 'ray-query'] += 1
    accepted, clip_calls = [], 0
    for key, flags, state, contents, bounds in targets:
        if state or flags & blocked:
            counts['rejected-before-geometry'] += 1
            continue
        if flags & 128:
            counts['bounds-targets'] += 1
            entry, leave = 0., 1.
            for axis in range(3):
                lo, hi = bounds[axis], bounds[axis+3]
                pos, half, step = center[axis], extents[axis], delta[axis]
                if not swept or not step:
                    if lo > f32(pos+half) or hi < f32(pos-half):
                        entry, leave = 1., 0.
                        break
                else:
                    a = (f32(lo-half)-pos)/step
                    b = (f32(hi+half)-pos)/step
                    entry, leave = max(entry,min(a,b)), min(leave,max(a,b))
            hit = entry <= leave
        else:
            clip_calls += 1
            counts['model-trace-targets'] += 1
            hit = bool(contents & MASK)
        if hit:
            accepted.append(key)
    counts['delivered-contacts'] += len(accepted)
    counts['grown-contact-lists'] += len(accepted) > 8
    if name == 'swept-crosses-gap':
        assert accepted == [424]
    if name == 'same-pose-misses-gap':
        assert not accepted
    return REPLY.pack(*center,*delta,*[-x for x in midpoint],*extents,
                      swept,is_ray,query_kind,clip_calls,len(accepted),*accepted,*([0]*(32-len(accepted))))


def reference_ranges(run):
    server, server_ranges = verify_installed_ranges(SERVER, run/'server-reference.local.txt', [
        ('Server acquires VEngineServer interface', 0x10146ea5, 0x10146eb7),
        ('Server calls SolidMoved', 0x100cde79, 0x100cde9a),
    ])
    engine, engine_ranges = verify_installed_ranges(ENGINE, run/'engine-reference.local.txt', [
        ('Installed interface registration', 0x1030c010, 0x1030c025),
        ('Installed interface factory', 0x1012a190, 0x1012a196),
        ('Engine interface SolidMoved', 0x1012a330, 0x1012a34f),
        ('Solid scan construction and collection delivery', 0x101dc410, 0x101dc52e),
        ('Solid scan hull bounds selection', 0x101dba00, 0x101dbac1),
        ('Construct centered sweep', 0x10100450, 0x10100615),
        ('Solid scan target enumeration and geometry dispatch', 0x101dbd50, 0x101dbe84),
        ('Box overlap or box sweep', 0x1014e8a0, 0x1014e9c1),
    ])
    assert server.string(0x10495ce0) == engine.string(0x1034bb28) == 'VEngineServer022'
    assert engine.word(engine.word(0x10402a1c)+0x12c) == 0x1012a330
    assert engine.word(0x1036408c) == 0x101dbd50
    return {'server':server_ranges, 'engine':engine_ranges}


def main(build):
    assert all(sha(path) == value for path,value in PIN.items())
    run = Path(tempfile.mkdtemp(dir=build,prefix='scan-'))
    evidence = reference_ranges(run)
    boxes = benchmark_boxes()
    rows = list(fixture_rows(boxes))
    queries = run/'queries.bin'
    queries.write_bytes(b''.join(HEADER.pack(*o,*p,*l,*s,m,st,a,b,len(ts)) +
        b''.join(ENTRY.pack(i,f,e,c,*v) for i,f,e,c,v in ts) for _,o,p,l,s,m,st,a,b,ts in rows))
    counts = Counter()
    target = b''.join(predict(row,counts) for row in rows)
    builds = {}
    for arch, machine in (('x86',0x14c),('x64',0x8664)):
        exe = build/f'scan_{arch}.exe'
        image = exe.read_bytes()
        pe, = struct.unpack_from('<I',image,0x3c)
        assert image[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H',image,pe+4)[0] == machine
        log = (build/f'scan_{arch}-build.log').read_text()
        assert 'warning ' not in log and 'error ' not in log
        builds[arch] = {'path':str(exe),'sha256':sha(exe),'warnings':0}
        for repeat in (1,2):
            out = run/f'{arch}-{repeat}.bin'
            result = subprocess.run([exe,queries,out,ENGINE if arch=='x86' else '-'],capture_output=True,timeout=45)
            (run/f'{arch}-{repeat}.log').write_bytes(result.stdout+result.stderr)
            assert result.returncode == 0, (arch,result.returncode,result.stderr,str(run))
            actual = out.read_bytes()
            assert len(actual) == len(target)
            if actual != target:
                pos = next(i for i,(a,b) in enumerate(zip(actual,target)) if a!=b)//REPLY.size
                raise AssertionError((arch,pos,rows[pos],REPLY.unpack_from(actual,pos*REPLY.size),
                                      REPLY.unpack_from(target,pos*REPLY.size)))
    assert all(counts.values()) and all(sha(path) == value for path,value in PIN.items())
    report = {'builds':builds,'repeats_per_arch':2,'matched_rows':len(rows),'coverage':dict(counts),
        'benchmark_bounds_fixture':boxes,'reference_ranges':evidence,'originals_unchanged':{str(k):v for k,v in PIN.items()},
        'limitations':'The installed SolidMoved method and enumeration callback execute. The fixture supplies the ordered partition candidates, actor collision data, and ClipRay contents. Benchmark boxes are source geometry for controlled bounds-only tests. The live partition, BSP hull clip geometry, server contact persistence, and SFM playback are outside this test.'}
    (run/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'rows':len(rows),'coverage':dict(counts),'receipt':str(run/'results.json')}))


if __name__ == '__main__':
    main(Path(sys.argv[1]).resolve())
