"""Compare box hull traces against the installed engine and benchmark brushes."""

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

from verify_sliding_door_spawn import SERVER, f32, verify_installed_ranges
from verify_translated_brush_point_trace import BSP, ENGINE, MESHES, TRACE, sha
from verify_solid_trigger_scan import benchmark_boxes

QUERY = struct.Struct('<6f4I9f')
TRIGGER_MASK = 0x0200400b
PINS = {
    ENGINE: 'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f',
    SERVER: '47a03f6bce8f519dafe1cfbcb9ba97cf0b0069d6eab57c2a52cffd61c66265d4',
    BSP: '60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea',
    MESHES: '961a7b203c61fd17c58e0c184e9d244da4748a4be74377762971cca0054351dd',
}


def placements():
    data = BSP.read_bytes()
    entities, size = struct.unpack_from('<2I', data, 8)
    models, _ = struct.unpack_from('<2I', data, 8+14*16)
    checked = benchmark_boxes()
    result = {}
    for entity in re.findall(rb'\{[^{}]+\}', data[entities:entities+size]):
        values = dict(re.findall(rb'"([^"]+)"\s*"([^"]*)"',entity))
        name = values.get(b'model',b'')
        if name not in {f'*{index}'.encode() for index in checked}:
            continue
        model = int(name[1:])
        origin = tuple(f32(float(value)) for value in values[b'origin'].split())
        record = struct.unpack_from('<9fi', data, models+model*48)
        result[model] = (origin, record[:3], record[3:6], record[9])
    assert result.keys() == checked.keys()
    return result


def fixtures(models):
    rng = random.Random(2026112163)
    shapes = (
        ((-24,-24,0),(24,24,82)),
        ((-24,-24,0),(24,24,62)),
        ((-3,-5,-9),(7,6,15)),
        ((-0.0625,-0.125,-0.5),(0.0625,0.125,0.5)),
    )

    for model, (origin, low, high, _) in models.items():
        center = tuple((a+b)*0.5 for a,b in zip(low,high))
        for pose, translation in enumerate(((0,0,0),(37.25,-13.5,111.75))):
            position = tuple(f32(a+b) for a,b in zip(origin,translation))
            for shape, (mins,maxs) in enumerate(shapes):
                extents = tuple(f32((b-a)*0.5) for a,b in zip(mins,maxs))
                offset = tuple(f32(-(b+a)*0.5) for a,b in zip(mins,maxs))

                def case(label, local_center, delta=(0,0,0), mask=TRIGGER_MASK):
                    start = tuple(f32(a+b) for a,b in zip(local_center,position))
                    delta = tuple(f32(v) for v in delta)
                    return (model, pose, shape, label,
                        QUERY.pack(*start,*delta,mask,delta!=(0,0,0),1,model,*position,*offset,*extents))

                yield case('interior', center)
                yield case('interior-excluded-by-mask', center, mask=0x4000)
                for axis, sign, gap in product(range(3),(-1,1),(-1.,-0.03125,0.,0.015625,0.03125,0.25,16.)):
                    point = list(center)
                    point[axis] = (low[axis] if sign<0 else high[axis]) + sign*(extents[axis]+gap)
                    into = [0.,0.,0.]
                    into[axis] = -sign*(high[axis]-low[axis]+2*extents[axis]+32)
                    yield case('face-stationary', point)
                    yield case('face-inward', point, into)
                    yield case('face-outward', point, [-d for d in into])
                    yield case('face-mask-excludes', point, into, 0x4000)
                    if gap == 0:
                        tangential = (axis+1)%3
                        point[tangential] = low[tangential]-extents[tangential]-16
                        tangent = [0.,0.,0.]
                        tangent[tangential] = high[tangential]-low[tangential]+2*extents[tangential]+32
                        yield case('face-tangent', point, tangent)
                for i in range(100):
                    begin = tuple(rng.uniform(a-64,b+64) for a,b in zip(low,high))
                    end = tuple(rng.uniform(a-64,b+64) for a,b in zip(low,high))
                    yield case('diagonal', begin, [b-a for a,b in zip(begin,end)],
                               (TRIGGER_MASK,1,0x4000,0)[i%4])

        # The same test hull crosses the trigger only when the partition uses
        # the swept path. Its start and end poses are both outside the brush.
        extents = (24,24,41)
        offset = (0,0,-41)
        before, after = list(center), list(center)
        before[0] = low[0]-64
        after[0] = high[0]+64
        start = tuple(f32(a+b) for a,b in zip(before,origin))
        end = tuple(f32(a+b) for a,b in zip(after,origin))
        yield (model, 0, 0, 'through-brush',
            QUERY.pack(*start,*[b-a for a,b in zip(start,end)],TRIGGER_MASK,1,1,model,*origin,*offset,*extents))
        yield (model, 0, 0, 'through-brush-destination',
            QUERY.pack(*end,0,0,0,TRIGGER_MASK,0,1,model,*origin,*offset,*extents))


def main(build):
    assert all(sha(path) == value for path,value in PINS.items())
    run = Path(tempfile.mkdtemp(dir=build,prefix='hulls-'))
    _, engine_ranges = verify_installed_ranges(ENGINE, run/'engine-reference.local.txt', [
        ('Inline collideable clip entry', 0x1016d6f0, 0x1016d73f),
        ('Zero-angle inline hull conversion and world-space return', 0x1014a130, 0x1014a493),
        ('Original world collision dispatch', 0x10149eb0, 0x1014a12f),
        ('Optimized swept box primitive', 0x10146850, 0x10146c23),
        ('Stationary optimized box branch', 0x10146c30, 0x10146ca4),
    ])
    server, server_ranges = verify_installed_ranges(SERVER, run/'server-reference.local.txt', [
        ('Register trigger_multiple', 0x10424620, 0x10424638),
        ('Trigger multiple spawn calls trigger initialization', 0x1025cab0, 0x1025caf0),
        ('Parentless brush solid type and trigger flags', 0x102587a0, 0x102588e5),
        ('Enabled trigger relinks after client activation', 0x102542b0, 0x102542cc),
    ])
    assert server.string(0x104e6af8) == 'trigger_multiple'
    table = 0x104e2a2c
    locator = server.word(table-4)
    assert server.string(server.word(locator+12)+8) == '.?AVCTriggerMultiple@@'
    assert server.word(table+0x60) == 0x1025cab0 and server.word(table+0x74) == 0x102542b0
    models = placements()
    rows = list(fixtures(models))
    queries = run/'queries.bin'
    queries.write_bytes(b''.join(wire for *_,wire in rows))
    outputs, builds = {}, {}
    for arch, machine in (('x86',0x14c),('x64',0x8664)):
        exe = build/f'brush_box_{arch}.exe'
        raw = exe.read_bytes()
        pe, = struct.unpack_from('<I',raw,60)
        assert raw[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0] == machine
        log = (build/f'brush-box-{arch}-build.log').read_text(errors='replace')
        warnings = re.findall(r'warning (C\d+): ([^\r\n]+)',log)
        builds[arch] = {'path':str(exe),'sha256':sha(exe),'warnings':warnings}
        for repeat in (1,2):
            out = run/f'{arch}-{repeat}.bin'
            result = subprocess.run([exe,BSP,MESHES,queries,out,ENGINE],capture_output=True,timeout=90)
            (run/f'{arch}-{repeat}.log').write_bytes(result.stdout+result.stderr)
            assert result.returncode == 0, (arch,result.returncode,result.stdout,result.stderr,str(run))
            actual = out.read_bytes()
            assert len(actual) == len(rows)*TRACE.size
            if arch in outputs:
                assert actual == outputs[arch], ('repeat mismatch',arch)
            outputs[arch] = actual
    counts = Counter()
    model_counts = {model:Counter() for model in models}
    differences = []
    for i, (model,pose,shape,label,wire) in enumerate(rows):
        at = i*TRACE.size
        original = outputs['x86'][at:at+TRACE.size]
        owned = outputs['x64'][at:at+TRACE.size]
        reference = TRACE.unpack(original)
        if original != owned:
            differences.append(dict(index=i,model=model,pose=pose,shape=shape,label=label,
                query=QUERY.unpack(wire),native=reference,owned=TRACE.unpack(owned)))
        mask = QUERY.unpack(wire)[6]
        eligible = bool(reference[4] & mask)
        counts['matching_contents' if eligible else 'no_matching_contents'] += 1
        counts['stationary' if not QUERY.unpack(wire)[7] else 'swept'] += 1
        counts['start_solid'] += reference[2]
        counts['all_solid'] += reference[3]
        model_counts[model]['matches_mask' if eligible else 'misses_mask'] += 1
        if label == 'through-brush':
            assert eligible and 0 < reference[0] < 1 and not reference[2]
        if label == 'through-brush-destination':
            assert not eligible and reference[0] == 1 and not reference[2]
    if differences:
        (run/'differences.local.json').write_text(json.dumps(differences[:60],indent=2),encoding='utf-8')
        raise AssertionError((len(differences),str(run/'differences.local.json')))
    assert all(len(metrics) == 2 for metrics in model_counts.values())
    assert all(sha(path) == value for path,value in PINS.items())
    report = dict(builds=builds, repeats_per_arch=2, matched_rows=len(rows), coverage=dict(counts),
        model_counts={model:dict(metrics) for model,metrics in model_counts.items()},
        model_placements=models, reference_ranges={'engine':engine_ranges,'server':server_ranges},
        originals_unchanged={str(k):v for k,v in PINS.items()},
        limitations='Zero-angle inline models verified as one axial box brush, using controlled hulls and poses. The x86 fixture invokes the original brush-clip branch. No live spatial-index enumeration, server contact persistence, or original SFM playback is exercised.')
    (run/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'rows':len(rows),'coverage':dict(counts),'model_counts':report['model_counts'],
                      'receipt':str(run/'results.json')}))


if __name__ == '__main__':
    main(Path(sys.argv[1]).resolve())
