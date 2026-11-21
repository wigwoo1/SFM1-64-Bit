"""Compare the owned translated brush-point trace with the installed engine."""
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import random
import re
import struct
import subprocess
import tempfile

from verify_ifm_draw_packet import check_ranges


ROOT = Path(__file__).resolve().parent.parent
OUT = Path(os.environ.get('SFM64_BRUSH_POINT_TEST_DIRECTORY',
                          ROOT/'outputs/translated-brush-point-trace'))
GAME = Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
ENGINE = GAME/'bin/engine.dll'
CLIENT = GAME/'tf/bin/client.dll'
BSP = GAME/'tf_movies/maps/mtt_soldier.bsp'
MESHES = ROOT/'outputs/swept-world-trace-20261015/verification-9h3j2gj_/prepared.bin'
QUERY = struct.Struct('<6f4I3f')
TRACE = struct.Struct('<2f3I4f2I6f')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def benchmark_doors():
    data = BSP.read_bytes()
    start, extent = struct.unpack_from('<2I', data, 8)
    model_start, model_extent = struct.unpack_from('<2I', data, 8+14*16)
    assert model_extent % 48 == 0
    for entity in re.findall(rb'\{[^{}]+\}', data[start:start+extent]):
        fields = dict(re.findall(rb'"([^"]+)"\s*"([^"]*)"', entity))
        if fields.get(b'classname') != b'func_door':
            continue
        name = fields[b'model'].decode()
        model = int(name[1:])
        origin = tuple(map(float, fields[b'origin'].split()))
        angles = tuple(map(float, fields.get(b'angles', b'0 0 0').split()))
        assert name.startswith('*') and 0 < model < model_extent//48 and not any(angles)
        model_record = struct.unpack_from('<9fi', data, model_start+48*model)
        yield model, origin, model_record[:3], model_record[3:6], model_record[9]


def build_queries(doors):
    rows = []
    rng = random.Random(20261121)
    for model, origin, mins, maxs, _ in doors:
        center = tuple((low+high)*.5 for low,high in zip(mins,maxs,strict=True))
        for pose, shift in enumerate(((0,0,0),(0,0,111.75),(37.25,-13.5,-81.5))):
            translated = tuple(v+d for v,d in zip(origin,shift,strict=True))

            def add(point, delta, mask=0x0200400B, swept=True, offset=translated):
                start = tuple(v+t for v,t in zip(point,translated,strict=True))
                rows.append((model,pose,QUERY.pack(*start,*delta,mask,swept,1,model,*offset)))

            for axis in range(3):
                for sign in (-1,1):
                    for excess in (-.125, 0., .015625, .03125, .0625, 1., 64.):
                        point = list(center)
                        point[axis] = (mins[axis] if sign>0 else maxs[axis])-sign*excess
                        delta = [0.,0.,0.]
                        delta[axis] = sign*(maxs[axis]-mins[axis]+128.)
                        add(point,delta)
                        add(point,delta,mask=0)
                        add(point,(0.,0.,0.),swept=False)
                        add(point,(0.,0.,0.),swept=True)
            # Repeat the same world ray with the brush shifted away. This
            # distinguishes model lookup and origin from world-space tracing.
            short_axis = min(range(3), key=lambda a: maxs[a]-mins[a])
            point = list(center)
            point[short_axis] = mins[short_axis]-8.
            delta = [0.,0.,0.]
            delta[short_axis] = maxs[short_axis]-mins[short_axis]+16.
            add(point,delta)
            moved = list(translated)
            moved[short_axis] += 512.
            add(point,delta,offset=moved)
            for i in range(160):
                point = tuple(rng.uniform(low-128,high+128) for low,high in zip(mins,maxs,strict=True))
                target = tuple(rng.uniform(low,high) for low,high in zip(mins,maxs,strict=True))
                delta = tuple(b-a for a,b in zip(point,target,strict=True))
                add(point,delta,mask=(0x0200400B,1,0x4000)[i%3])
    return rows


def main():
    protected = {
        ENGINE: 'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f',
        CLIENT: '91dd1062227b095f1338312aa7268121894970c3e0a5b0d314e42c77f2aba73b',
        BSP: '60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea',
        MESHES: '961a7b203c61fd17c58e0c184e9d244da4748a4be74377762971cca0054351dd',
    }
    assert all(sha(path)==digest for path,digest in protected.items())
    _, evidence = check_ranges(ENGINE,'engine-disassembly.txt',[
        ('Inline model lookup',0x101464a0,0x101464c9),
        ('Brush entity point dispatch',0x1016d6f0,0x1016d73f),
        ('Brush transform and world-space result',0x1014a130,0x1014a493),
    ])
    client, entity_evidence = check_ranges(CLIENT,'native-client-scale-disassembly.txt',[
        ('Door collision-property accessor',0x10011e60,0x10011e67),
        ('Door default entity rule',0x10015370,0x1001538f),
        ('Collision property model lookup',0x100cdcf0,0x100cdcf8),
        ('Collision property origin lookup',0x100cdce0,0x100cdcea),
        ('Collision property angle selection',0x100ce0f0,0x100ce111),
    ])
    door_table, property_table = 0x104c6b94, 0x104eb984
    for table, name in [(door_table, '.?AVC_BaseDoor@@'), (property_table, '.?AVCCollisionProperty@@')]:
        locator = client.word(table-4)
        assert client.string(client.word(locator+12)+8) == name
    assert client.word(door_table+0x0c) == 0x10011e60
    assert client.word(door_table+0x280) == client.word(0x104c7974+0x280) == 0x10015370
    assert [client.word(property_table+offset) for offset in (0x18,0x20,0x24)] == [
        0x100cdcf0,0x100cdce0,0x100ce0f0]
    doors = list(benchmark_doors())
    assert {model for model,*_ in doors} == {1,3,5,8,12}
    rows = build_queries(doors)
    run = Path(tempfile.mkdtemp(prefix='benchmark-',dir=OUT))
    inputs = run/'rays.bin'
    inputs.write_bytes(b''.join(wire for _,_,wire in rows))
    output, builds = {}, {}
    for role,arch,machine in [('native','x86',0x14c),('owned','x64',0x8664)]:
        executable = OUT/f'translated_{role}_{arch}.exe'
        raw = executable.read_bytes()
        pe, = struct.unpack_from('<I',raw,60)
        assert struct.unpack_from('<H',raw,pe+4)[0] == machine
        dest = run/f'{role}.bin'
        result = subprocess.run([executable,BSP,MESHES,inputs,dest,ENGINE],
                                capture_output=True,timeout=100)
        (run/f'{role}.log').write_bytes(result.stdout+result.stderr)
        assert result.returncode==0, (role,result.stdout,result.stderr)
        output[role] = dest.read_bytes()
        assert len(output[role]) == len(rows)*TRACE.size
        log = OUT/f'translated-{role}-build.log'
        builds[role] = {'executable':str(executable),'sha256':sha(executable),
                        'warnings':len(re.findall(r'warning C\d+',log.read_text(errors='replace')))}
    metrics = Counter()
    per_door = {}
    same_rays = {}
    for i,(model,pose,query) in enumerate(rows):
        at = i*TRACE.size
        x86 = output['native'][at:at+TRACE.size]
        x64 = output['owned'][at:at+TRACE.size]
        if x86!=x64:
            difference = dict(case=i,model=model,pose=pose,input=QUERY.unpack(query),
                              native=TRACE.unpack(x86),owned=TRACE.unpack(x64))
            (run/'difference.json').write_text(json.dumps(difference,indent=2)+'\n')
            raise AssertionError(difference)
        fraction, exit_fraction, started, stuck, contents, *_ = TRACE.unpack(x86)
        tally = per_door.setdefault(model,Counter())
        tally['rays'] += 1
        tally['hits'] += fraction<1
        metrics['hits'] += fraction<1
        metrics['misses'] += fraction==1
        metrics['start_solid'] += bool(started)
        metrics['all_solid'] += bool(stuck)
        metrics['left_solid'] += 0<exit_fraction<1
        values = QUERY.unpack(query)
        metrics['stationary'] += not values[7]
        metrics['zero_delta_with_swept_flag'] += bool(values[7]) and not any(values[3:6])
        metrics['translated_hits'] += bool(pose) and fraction<1
        key, origin = query[:40], query[40:]
        previous = same_rays.setdefault(key, {})
        if previous and origin not in previous and any(hit!=(fraction<1) for hit in previous.values()):
            metrics['same_ray_changed_hit_after_translation'] += 1
        previous[origin] = fraction<1
    assert all(metrics[name]>0 for name in ('hits','misses','start_solid','all_solid','left_solid','translated_hits'))
    assert all(tally['hits']>0 for tally in per_door.values())
    assert metrics['same_ray_changed_hit_after_translation'] >= 3*len(doors)
    for model,invalid in [(0,'world root'),(0xffffffff,'unknown inline model')]:
        wire = bytearray(rows[0][2])
        struct.pack_into('<I',wire,36,model)
        malformed = run/f'invalid-{model}.bin'
        malformed.write_bytes(wire)
        result = subprocess.run([builds['owned']['executable'],BSP,MESHES,malformed,
                                 run/f'invalid-{model}.out',ENGINE],capture_output=True,timeout=30)
        assert result.returncode==1 and b'invalid inline BSP collision model' in result.stderr, invalid
    assert all(sha(path)==digest for path,digest in protected.items())
    receipt = dict(status='PASS',locally_translated_brush_models=len(doors),queries=len(rows),
                   bit_exact=True,metrics=dict(metrics),per_door=per_door,
                   engine_bytes_checked=sum(record['bytes'] for record in evidence),
                   client_bytes_checked=sum(record['bytes'] for record in entity_evidence),
                   client_door_uses_base_entity_filter=True,
                   original_inputs_unchanged=True,executables=builds,
                   scope='Zero-angle point clipping with supplied collideable origins and model indices.')
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(receipt))
    print('Receipt',run/'results.json')


if __name__ == '__main__':
    main()
