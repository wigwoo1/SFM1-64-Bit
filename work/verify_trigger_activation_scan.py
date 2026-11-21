"""Compare the installed client-activation gate and reverse trigger scan."""

from collections import Counter
from itertools import product
import json
from pathlib import Path
import random
import struct
import subprocess
import sys
import tempfile

from verify_sliding_door_spawn import SERVER, ENGINE, f32, sha, verify_installed_ranges
from verify_translated_brush_box_trace import QUERY as BRUSH_QUERY, PINS, TRACE, TRIGGER_MASK, placements
from verify_translated_brush_point_trace import BSP, MESHES

HEADER = struct.Struct('<15f8I')
ENTRY = struct.Struct('<4I15f')
REPLY = struct.Struct('<6f37I')
ZERO = (0.,0.,0.)
LOCAL = (-24.,-24.,-41.,24.,24.,41.)


def translate(box, origin):
    return tuple(f32(x+origin[i%3]) for i,x in enumerate(box))


def header(origin=ZERO, local=LOCAL, bounds=LOCAL, flags=12, accurate=1, disabled=0,
           edict=1, world=0, model=1, solid=1):
    return (*origin,*local,*bounds,flags,accurate,disabled,edict,world,model,solid)


def entry(key, excluded, contents, origin=ZERO, local=LOCAL, solid=2, surrounding=None):
    if surrounding is None:
        surrounding = translate(local, origin)
    return (key,excluded,solid,contents,*origin,*local,*surrounding)


def probe(exe, args, log):
    result = subprocess.run([exe,*args],capture_output=True,timeout=75)
    log.write_bytes(result.stdout+result.stderr)
    assert result.returncode == 0,(result.returncode,str(log),result.stdout,result.stderr)


def benchmark_rows(run, brush_receipt):
    previous = json.loads(brush_receipt.read_bytes())
    pairs = []
    for model,(origin,low,high,_) in placements().items():
        local = (*low,*high)
        middle = tuple(f32((lo+hi)*0.5) for lo,hi in zip(low,high))
        sweep_centers = []
        for axis, sign, gap in product(range(3),(-1,1),(-0.5,0,0.5,64)):
            point = list(middle)
            point[axis] = (low[axis] if sign<0 else high[axis])+sign*(LOCAL[axis+3]+gap)
            sweep_centers.append(tuple(f32(a+b) for a,b in zip(point,origin)))
        pairs.append((model,origin,local,sweep_centers))
    path = run/'brush-queries.bin'
    path.write_bytes(b''.join(BRUSH_QUERY.pack(*center,*ZERO,TRIGGER_MASK,0,1,model,
        *origin,*ZERO,*LOCAL[3:]) for model,origin,_,centers in pairs for center in centers))
    native_replies = {}
    for arch in ('x86','x64'):
        recorded = previous['builds'][arch]
        exe = Path(recorded['path'])
        assert sha(exe) == recorded['sha256']
        out = run/f'brush-{arch}.bin'
        probe(exe,[BSP,MESHES,path,out,ENGINE],run/f'brush-{arch}.log')
        native_replies[arch] = out.read_bytes()
    assert native_replies['x86'] == native_replies['x64']
    results = iter(TRACE.iter_unpack(native_replies['x86']))
    for model,origin,local,centers in pairs:
        candidates = [entry(100+i,0,next(results)[4],pos) for i,pos in enumerate(centers)]
        assert any(c[3]&TRIGGER_MASK for c in candidates) and any(not c[3]&TRIGGER_MASK for c in candidates)
        yield f'benchmark-brush-{model}',header(origin,local,translate(local,origin)),candidates
    assert next(results,None) is None


def fixtures(bench):
    for disabled, edict, world, model, solid, flags, accurate in product(
        range(2),range(2),range(2),range(2),(0,1,2,6),(0,4,8,12,128,140),range(2),
    ):
        yield 'activation-gate',header(flags=flags,accurate=accurate,disabled=disabled,
            edict=edict,world=world,model=model,solid=solid),[]
    rng = random.Random(2026112166)
    for flags, accurate, count in product((12,140),range(2),(1,8,9,17,32)):
        entries = [entry(key,0,1) for key in rng.sample(range(1,999),count)]
        yield 'ordered',header(flags=flags,accurate=accurate),entries
    for flags, accurate, exclusion, contents in product(
        (12,140),range(2),(0,1,2,4,8,16,7,31),(0,1,16,0x4000,0x02000000),
    ):
        entries = [entry(4,exclusion,contents),entry(1,0,1),entry(3,0,16)]
        yield 'filters',header(flags=flags,accurate=accurate),entries
    for _ in range(700):
        origin = tuple(rng.randrange(-400,401)/4 for _ in range(3))
        bounds = translate((-10,-20,-30,30,20,10),origin)
        candidates = []
        for key in rng.sample(range(1,400),16):
            bump = (rng.choice((21,25,54,54.25,120)),rng.randrange(-60,61),rng.randrange(-40,41))
            pos = tuple(f32(a+b) for a,b in zip(origin,bump))
            surrounding = translate((-39,-49,-49,39,49,49),pos)
            candidates.append(entry(key,rng.choice((0,0,0,1,2,4,8,16)),rng.choice((0,8,16,0x4000)),
                pos,LOCAL,rng.choice((1,2,3,6)),surrounding))
        yield 'varied',header(origin,LOCAL,bounds,flags=rng.choice((12,140)),accurate=rng.randrange(2)),candidates
    yield from bench


def expected(mode, rows, coverage):
    report = bytearray()
    for label, record, entries in rows:
        flags, accurate, disabled, edict, world, model, solid = record[-7:]
        if mode == 'server':
            scan = 0
            if not disabled and edict and not world and (solid != 1 or model):
                if flags & 8:
                    scan = 2
                elif solid and not flags & 4:
                    scan = 1
            coverage[f'activation_scan_{scan}'] += 1
            report.extend(REPLY.pack(*([0]*7),scan,*([0]*35)))
            continue
        trigger_box = record[9:15] if flags & 128 else translate(record[3:9],record[:3])
        contacts = []
        clip_calls, fingerprint = 0, 0
        for key, excluded, kind, contents, x,y,z,*boxes in entries:
            if excluded:
                coverage['candidates_excluded'] += 1
                continue
            origin = (x,y,z)
            local = boxes[:6] if accurate and kind == 2 else [
                f32(value-origin[i%3]) for i,value in enumerate(boxes[6:])]
            center_offset = [f32(f32(local[i]+local[3+i])*0.5) for i in range(3)]
            center = [f32(a+b) for a,b in zip(origin,center_offset)]
            extent = [f32(f32(local[3+i]-local[i])*0.5) for i in range(3)]
            if flags & 128:
                hit = all(f32(center[i]+extent[i]) >= trigger_box[i] and
                          f32(center[i]-extent[i]) <= trigger_box[i+3] for i in range(3))
                coverage['trigger_bounds_checks'] += 1
            else:
                clip_calls += 1
                hit = bool(contents & TRIGGER_MASK)
                words = struct.unpack('<12I',struct.pack('<12f',*center,*ZERO,*[-x for x in center_offset],*extent))
                for word in (key,*words,sum(x*x for x in extent)<1e-6,0):
                    fingerprint = ((fingerprint ^ word)*16777619) & 0xffffffff
                coverage['brush_trace_requests'] += 1
            if hit:
                contacts.append(key)
        if label.startswith('benchmark'):
            assert contacts
            coverage[label] += len(contacts)
        coverage['collected_contacts'] += len(contacts)
        coverage['grown_lists'] += len(contacts)>8
        report.extend(REPLY.pack(*trigger_box,1,0,clip_calls,fingerprint,len(contacts),
            *contacts,*([0]*(32-len(contacts)))))
    return bytes(report)


def installed_ranges(run):
    engine, eng = verify_installed_ranges(ENGINE,run/'engine-reference.local.txt',[
        ('Reverse trigger scan interface',0x1012a350,0x1012a367),
        ('TriggerMoved bounds query and contact delivery',0x101dc310,0x101dc38b),
        ('Reverse scan query selects solid list',0x101dbad0,0x101dbb55),
        ('Reverse scan collector and collision routing',0x101dbfb0,0x101dc16d),
        ('Static prop handle classification',0x101c4700,0x101c472c),
        ('Server game entity interface acquisition',0x101cad58,0x101cad6d),
    ])
    server, srv = verify_installed_ranges(SERVER,run/'server-reference.local.txt',[
        ('Trigger post-client activation',0x102542b0,0x102542cc),
        ('Base activation callback',0x1002ac90,0x1002ac91),
        ('Server forwards a trigger scan',0x100cdea0,0x100cdebc),
        ('Server game entity interface registration',0x1041d370,0x1041d385),
        ('Server marks an engine contact pair',0x10143470,0x1014356f),
        ('Static evidence for bilateral touch registration',0x1019bc50,0x1019bcf6),
        ('Static evidence for touch-link timestamp refresh',0x10199f80,0x1019a1e4),
        ('Static evidence for removing all links and resetting the timestamp',0x1019aca0,0x1019ae7d),
        ('Static evidence for expiring stale links after a scan',0x1019baf0,0x1019bc42),
    ])
    assert engine.word(engine.word(0x10402a1c)+0x130) == 0x1012a350
    assert engine.word(0x1036409c) == 0x101dbfb0
    assert engine.word(engine.word(0x1040b598)+0x2c) == 0x101c4700
    assert engine.string(0x103607e0) == server.string(0x10496128) == 'ServerGameEnts001'
    assert server.word(0x10495564+4) == 0x10143470
    assert server.word(0x104e2a2c+0x74) == 0x102542b0
    return {'engine':eng,'server':srv}


def main(build, brush_receipt):
    assert all(sha(path)==digest for path,digest in PINS.items())
    run = Path(tempfile.mkdtemp(dir=build,prefix='activation-'))
    evidence = installed_ranges(run)
    rows = list(fixtures(list(benchmark_rows(run,brush_receipt))))
    queries = run/'queries.bin'
    queries.write_bytes(b''.join(HEADER.pack(*q,len(entries))+
        b''.join(ENTRY.pack(*entry) for entry in entries) for _,q,entries in rows))
    counts, builds = Counter(), {}
    target = {mode:expected(mode,rows,counts) for mode in ('server','engine')}
    for arch,machine in (('x86',0x14c),('x64',0x8664)):
        exe = build/f'activation_{arch}.exe'
        image = exe.read_bytes()
        pe, = struct.unpack_from('<I',image,60)
        assert image[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',image,pe+4)[0]==machine
        build_log = (build/f'activation_{arch}-build.log').read_text()
        assert 'warning ' not in build_log and 'error ' not in build_log
        builds[arch] = {'path':str(exe),'sha256':sha(exe),'warnings':0}
        for mode,reference in (('server',SERVER),('engine',ENGINE)):
            for repeat in (1,2):
                name = f'{mode}-{arch}-{repeat}'
                dest = run/f'{name}.bin'
                probe(exe,[mode,queries,dest,reference if arch=='x86' else '-'],run/f'{name}.log')
                actual = dest.read_bytes()
                assert len(actual)==len(target[mode])
                if actual != target[mode]:
                    index = next(i for i,(a,b) in enumerate(zip(actual,target[mode])) if a!=b)//REPLY.size
                    raise AssertionError((name,index,rows[index],
                        REPLY.unpack_from(actual,index*REPLY.size),REPLY.unpack_from(target[mode],index*REPLY.size)))
    assert all(counts.values()) and all(sha(path)==digest for path,digest in PINS.items())
    report = dict(builds=builds, repeats_per_mode_and_arch=2, scenarios=len(rows),
        matched_rows=len(rows)*len(target), coverage=dict(counts), native_reference_ranges=evidence,
        brush_receipt=str(brush_receipt), originals_unchanged={str(k):v for k,v in PINS.items()},
        limitations='Server gate and engine reverse-scan execution are isolated. The fixture supplies ordered partition entries and model clip results, including fresh independent x86/x64 clips for four benchmark brushes. The final callback reports ordered pairs; persistence and live SFM activation are not executed.')
    (run/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(rows=report['matched_rows'],coverage=dict(counts),receipt=str(run/'results.json'))))


if __name__=='__main__':
    main(Path(sys.argv[1]).resolve(),Path(sys.argv[2]).resolve())
