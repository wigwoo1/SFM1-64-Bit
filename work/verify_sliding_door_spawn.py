"""Compare parentless sliding-door spawn arithmetic and audit the benchmark session."""

from collections import Counter
import hashlib
import json
import math
import os
from pathlib import Path
import random
import re
import struct
import subprocess
import sys
import tempfile

from audit_source_inline_numbering import DUMP
from verify_ifm_draw_packet import check_ranges


ROOT = Path(__file__).resolve().parent.parent
OUT = Path(os.environ.get('SFM64_MAP_DOOR_TEST_DIRECTORY',ROOT/'outputs/map-door-state'))
GAME = Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
SERVER = GAME/'tf/bin/server.dll'
ENGINE = GAME/'bin/engine.dll'
IFM = GAME/'bin/tools/ifm.dll'
BSP = GAME/'tf_movies/maps/mtt_soldier.bsp'
SESSION = GAME/'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'
QUERY = struct.Struct('<14fiI')
REPLY = struct.Struct('<13f4I')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def f32(value):
    return struct.unpack('<f',struct.pack('<f',value))[0]


def verify_installed_ranges(binary, output, spans):
    parts = []
    for _,first,last in spans:
        captured = subprocess.run([DUMP,'/nologo','/disasm:bytes',f'/range:0x{first:X},0x{last-1:X}',binary],
                                  capture_output=True,check=True,timeout=25)
        parts.append(captured.stdout)
    output.write_bytes(b'\n'.join(parts))
    return check_ranges(binary,str(output),spans)


def read_map_doors():
    data = BSP.read_bytes()
    assert data[:4] == b'VBSP'
    start,size = struct.unpack_from('<2I',data,8)
    models,model_size = struct.unpack_from('<2I',data,8+14*16)
    assert model_size % 48 == 0
    entities = []
    doors = []
    for entity in re.findall(rb'\{[^{}]+\}',data[start:start+size]):
        pairs = re.findall(rb'"([^"]+)"\s*"([^"]*)"',entity)
        entities.append(pairs)
        fields = dict(pairs)
        if fields.get(b'classname') != b'func_door':
            continue
        model = fields[b'model'].decode()
        index = int(model[1:])
        assert model.startswith('*') and 0<index<model_size//48 and b'parentname' not in fields
        assert not any(map(float,fields.get(b'angles',b'0 0 0').split()))
        disk = struct.unpack_from('<6f',data,models+index*48)
        # Both installed engine model loaders add a unit on each face.
        loaded = tuple(f32(value+(-1 if axis<3 else 1)) for axis,value in enumerate(disk))
        origin = tuple(map(float,fields[b'origin'].split()))
        movedir = tuple(map(float,fields[b'movedir'].split()))
        wire = QUERY.pack(*origin,*movedir,*loaded,float(fields[b'lip']),float(fields[b'speed']),
                          int(fields.get(b'spawnpos',b'0')),int(fields[b'spawnflags']))
        doors.append((model,fields[b'targetname'].decode(),wire))
    assert [model for model,_,_ in doors] == ['*1','*3','*5','*8','*12']
    trigger_counts = Counter()
    names = {name.encode():model for model,name,_ in doors}
    for pairs in entities:
        for name,value in pairs:
            if not name.startswith(b'On'):
                continue
            target,_,rest = value.partition(b',')
            command,_,_ = rest.partition(b',')
            if target in names and command in (b'Open',b'Close',b'Toggle'):
                trigger_counts[names[target],command.decode()] += 1
    return doors,{f'{model}:{cmd}':n for (model,cmd),n in sorted(trigger_counts.items())}


def audit_session(doors):
    sys.path.insert(0,str(ROOT/'work/cv/65e1f239'))
    import datamodel as dm

    data = dm.load(str(SESSION),compact_arrays=True)
    model_names = {model for model,_,_ in doors}
    target_names = {target for _,target,_ in doors}
    models = {e.id:e for e in data.elements if e.type=='DmeGameModel' and e.get('modelName') in model_names}
    transforms = {model['transform'].id:model for model in models.values()}
    route_targets = models | transforms
    channels = Counter()
    locations = {model:set() for model in model_names}
    for model in models.values():
        transform = model['transform']
        assert tuple(transform['orientation']) == (0.,0.,0.,1.)
        locations[model['modelName']].add(tuple(transform['position']))
    clips = [e for e in data.elements if e.type=='DmeFilmClip']
    for channel in data.elements:
        if channel.type=='DmeChannel' and getattr(channel.get('toElement'),'id',None) in route_targets:
            channels[channel['toAttribute']] += 1
    counts = Counter(model['modelName'] for model in models.values())
    assert counts == {'*1':8,'*3':7,'*5':8,'*8':11,'*12':8}
    assert channels == {'visible':9}
    assert not any(clip['concommands'] for clip in clips)
    assert not any(e.name in target_names for e in data.elements)
    return dict(map_names=sorted({clip['mapname'] for clip in clips if clip['mapname']}), film_clips=len(clips),
                saved_brush_models=dict(sorted(counts.items())), saved_pose_channels=dict(channels),
                distinct_saved_origins={model:len(poses) for model,poses in sorted(locations.items())},
                captured_entity_target_names=0, nonempty_console_command_lists=0,
                scope='Identifies saved DmeGameModel instances by model token. Their saved poses and visible channels do not establish live map-entity identity or collision state.')


def cases(doors):
    rows = [(f'benchmark {model}',wire) for model,_,wire in doors]
    for model,_,wire in doors:
        row = list(QUERY.unpack(wire))
        for speed in (0., -0., 0.125, 300.):
            for lip in (0., 2., 4., -2., 200.):
                for position,flags in [(0,0),(1,0),(2,1),(0,0x821),(1,0x800),(0,0x820)]:
                    rows.append((f'benchmark variants {model}',QUERY.pack(*row[:12],lip,speed,position,flags)))
    rng = random.Random(49768)
    for _ in range(2048):
        low = [rng.uniform(-2048,0) for _ in range(3)]
        high = [rng.uniform(0,2048) for _ in range(3)]
        origin = [rng.uniform(-16000,16000) for _ in range(3)]
        angles = [rng.uniform(-36000,36000) for _ in range(3)]
        rows.append(('general move direction',QUERY.pack(*origin,*angles,*low,*high,
            rng.uniform(-12,64),rng.choice((0.,0.25,200.)),rng.randrange(3),rng.randrange(4096))))
    return rows


def independent_check(query, reply):
    origin,angles,low,high = query[:3],query[3:6],query[6:9],query[9:12]
    lip,speed,position,flags = query[12:]
    closed, opened, initial, direction = reply[:3], reply[3:6], reply[6:9], reply[9:12]
    rad_factor = f32(math.pi/180)
    pitch,yaw = (f32(a*rad_factor) for a in angles[:2])
    direction_check = (math.cos(pitch)*math.cos(yaw),math.cos(pitch)*math.sin(yaw),-math.sin(pitch))
    assert max(abs(a-b) for a,b in zip(direction,direction_check)) < 2.e-7
    spans = [f32(f32(hi-lo)-2) for lo,hi in zip(low,high)]
    contributions = [abs(f32(span*value)) for span,value in zip(spans,direction)]
    distance = f32(f32(f32(contributions[1]+contributions[0])+contributions[2])-lip)
    destination = tuple(f32(p+f32(value*distance)) for p,value in zip(origin,direction))
    starts_open = position==1 or bool(flags&1)
    assert closed==origin and opened==destination and initial==(destination if starts_open else origin)
    assert reply[12:] == ((100. if speed==0. else speed),int(not starts_open),int(bool(flags&2048)),6,7)


def main():
    hashes = {
        SERVER:'47a03f6bce8f519dafe1cfbcb9ba97cf0b0069d6eab57c2a52cffd61c66265d4',
        ENGINE:'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f',
        IFM:'12504c248047e181d31a8500fb6693d1a2d3c805e841fc2227e41aa4e2d2b47e',
        BSP:'60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea',
        SESSION:'5a08cf8eeb086586f05dc856fb5469dd238fde90315ecf17c2dd4ab5f0742aee',
    }
    for path,value in hashes.items():
        assert sha(path)==value,path.name
    work = Path(tempfile.mkdtemp(prefix='spawn-',dir=OUT))
    print(work,flush=True)
    image,server = verify_installed_ranges(SERVER,work/'server-ranges.local.txt',[
        ('Installed door spawn prefix',0x101030e0,0x1010341e),
        ('Spawn return',0x101034f7,0x10103508),
        ('Travel direction',0x103615f0,0x10361685),
        ('Vector scratch slot',0x10018c40,0x10018cc2),
        ('Non-rotating vtable route',0x102c93c0,0x102c93c3),
    ])
    assert image.word(0x10474bbc+0x320)==0x102c93c0
    assert [image.word(a) for a in (0x105d9a0c+4,0x105d9a48+4,0x105d9a84+4,0x1059a61c-8)] == [0x3b8,0x3bc,0x3c8,0x420]
    assert [image.word(a) for a in (0x10456e64,0x10445090,0x10445690,0x10445970)] == [0x3c8efa35,0x40000000,0x7fffffff,0x42c80000]
    engine_image,engine = check_ranges(ENGINE,'engine-disassembly.txt',[
        ('Collision model bounds conversion',0x1014b920,0x1014b998),
        ('Render inline-model bounds conversion',0x100e7fbb,0x100e805f),
    ])
    assert engine_image.word(0x1031bdec)==0x3f800000
    ifm_image,ifm = verify_installed_ranges(IFM,work/'ifm-ranges.local.txt',[
        ('Inline model admission and puppet construction',0x102abd88,0x102abdd5),
        ('Puppet constructor vtables',0x10243220,0x10243241),
        ('Puppet client base-entity accessor',0x1022b000,0x1022b003),
    ])
    ifm_instructions={row['address']:row['instruction'] for span in ifm for row in span['instructions']}
    assert ifm_instructions[0x102abd9e].split()==['jne','102ABDB0']
    assert ifm_instructions[0x102abdcc].split()==['call','10243220']
    assert ifm_instructions[0x10243234].endswith('10ADB9B8h')
    assert ifm_image.word(0x10adb9b8+7*4)==0x1022b000
    assert ifm_instructions[0x1022b000].split()==['xor','eax,eax']
    assert ifm_instructions[0x1022b002]=='ret'
    doors, inputs = read_map_doors()
    audit = audit_session(doors)
    rows = cases(doors)
    query_file=work/'queries.bin'
    query_file.write_bytes(b''.join(wire for _,wire in rows))
    builds = {}
    results = {}
    for arch,machine in [('x86',0x14c),('x64',0x8664)]:
        executable=OUT/f'door_{arch}.exe'
        raw=executable.read_bytes()
        pe,=struct.unpack_from('<I',raw,0x3c)
        assert raw[pe:pe+6] == b'PE\0\0'+struct.pack('<H',machine)
        assert 'warning' not in (OUT/f'door_{arch}-build.log').read_text().lower()
        builds[arch]=dict(path=str(executable),sha256=sha(executable),warnings=0)
        for repeat in range(2):
            destination=work/f'{arch}-{repeat}.bin'
            result=subprocess.run([executable,SERVER if arch=='x86' else '-',query_file,destination],
                                  timeout=30,capture_output=True)
            (work/f'{arch}-{repeat}.log').write_bytes(result.stdout+result.stderr)
            assert result.returncode==0,(arch,result.returncode,result.stderr.decode(errors='replace'))
            data=destination.read_bytes()
            assert len(data)==REPLY.size*len(rows)
            if repeat==0:
                results[arch]=data
            else:
                assert data==results[arch],arch
    counts=Counter()
    benchmark_spawn={}
    for i,(label,raw) in enumerate(rows):
        reference=results['x86'][i*REPLY.size:(i+1)*REPLY.size]
        owned=results['x64'][i*REPLY.size:(i+1)*REPLY.size]
        assert owned==reference,(i,label,REPLY.unpack(reference),REPLY.unpack(owned))
        answer=REPLY.unpack(reference)
        independent_check(QUERY.unpack(raw),answer)
        if i<len(doors):
            model=doors[i][0]
            travel=tuple(b-a for a,b in zip(answer[:3],answer[3:6]))
            assert travel==(0.,0.,111. if model in ('*1','*3') else -126.)
            assert answer[12:15]==(300.,1,0)
            benchmark_spawn[model]=dict(travel=travel,speed=answer[12],initially_open=False)
        counts['open at spawn' if answer[13]==0 else 'closed at spawn']+=1
        if answer[14]:counts['locked at spawn']+=1
    assert all(sha(path)==value for path,value in hashes.items())
    result=dict(status='VERIFIED sliding-door spawn prefix',queries=len(rows),repetitions=2,counts=counts,builds=builds,
                benchmark_models=benchmark_spawn,map_input_links=inputs,session_audit=audit,
                server_bytes_checked=sum(row['bytes'] for row in server),engine_loader_bytes_checked=sum(row['bytes'] for row in engine),
                ifm_bytes_checked=sum(row['bytes'] for row in ifm),
                inline_puppet_route='Admitted inline names call the standard CDmePuppet constructor. Its client base-entity accessor returns null.',
                originals_unchanged=True,scope='Parentless linear door, zero entity rotation, model bounds supplied. Spawn position, travel endpoints, direction, speed and lock flag verified. Engine setters observed at the fixture boundary. Playback mapping, post-spawn physics and trigger scheduling unverified.')
    (work/'results.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(work/'results.json')
    print(json.dumps(result,indent=2))


if __name__=='__main__':
    main()
