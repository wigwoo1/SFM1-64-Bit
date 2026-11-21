"""Check dynamic point-ray enumeration against the installed spatial partition."""

from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import random
import shutil
import struct
import subprocess
import tempfile

from verify_client_collision_partition import QUERY as CLIENT_QUERY, REPLY as CLIENT_REPLY
from verify_client_collision_partition import benchmark_doors
from verify_ifm_draw_packet import check_ranges


ROOT = Path(__file__).resolve().parent.parent
OUT = Path(os.environ.get('SFM64_SPATIAL_POINT_TEST_DIRECTORY', ROOT/'outputs/spatial-point-partition-20261016'))
GAME = Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
ENGINE = GAME/'bin/engine.dll'
TIER0 = GAME/'bin/tier0.dll'
BSP = GAME/'tf_movies/maps/mtt_soldier.bsp'
CLIENT = GAME/'tf/bin/client.dll'
CLIENT_TESTS = Path(os.environ.get('SFM64_CLIENT_COLLISION_TEST_DIRECTORY', ROOT/'outputs/client-collision-partition'))
ENTRY = struct.Struct('<QI6f')
MUTATION = struct.Struct('<4IQ6f')
QUERY = struct.Struct('<I6f3I')
NO_CHANGE = MUTATION.pack(0, 0, 0, 0, 0, *([0]*6))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def box(x, y=64, z=64, half=5):
    return x-half, y-half, z-half, x+half, y+half, z+half


def center(bounds):
    return tuple((a+b)*0.5 for a,b in zip(bounds[:3],bounds[3:]))


def move(handle, bounds):
    return MUTATION.pack(1, handle, 0, 0, 0, *bounds)


def lists(handle, remove, insert):
    return MUTATION.pack(2, handle, remove, insert, 0, *([0]*6))


def destroy(handle):
    return MUTATION.pack(3, handle, 0, 0, 0, *([0]*6))


def create(handle, identity, mask, bounds):
    return MUTATION.pack(4, handle, 0, mask, identity, *bounds)


def query(mask, start, delta=(0,0,0), swept=False, stop=0):
    return QUERY.pack(mask, *start, *delta, 0, stop, swept)


def lifecycle():
    entries = [(1,4,box(64)), (2,4,box(64)), (3,5,box(64)), (4,0,box(64)),
               (5,1,box(64)), (6,4,box(16384)), (7,4,box(16384))]
    steps = []

    def at(mask, x, expected, command=NO_CHANGE):
        steps.append((command, query(mask, (x,64,64)), expected))

    at(4,64,[3,2,1])
    at(4,64,[3,2,1],move(0,box(64)))    # Identical padded bounds preserve order.
    at(4,64,[1,3,2],move(0,box(65)))    # A genuine move reinserts within the old cell.
    at(4,64,[3,2],move(0,box(576)))
    at(4,576,[1])
    at(4,64,[1,3,2],move(0,box(64)))
    at(4,64,[3,2],lists(0,4,1))
    at(1,64,[5,3])                    # Mask rewrite doesn't insert into the new tree.
    at(1,64,[1,5,3],move(0,box(64)))
    at(1,64,[1,5],move(2,box(576)))
    at(4,576,[3])
    at(1,576,[])                      # Second tree still links to the old cell.
    at(4,64,[2],destroy(2))
    at(1,64,[8,1,5],create(2,8,1,box(64)))
    at(4,64,[2],move(3,box(64)))      # A zero list masks all updates.
    at(4,64,[2],lists(3,0,4))
    at(4,64,[4,2],move(3,box(64)))
    at(4,64,[2,4],move(3,box(64,half=128)))  # Same box center, coarser level.
    at(4,64,[4,2],move(3,box(64)))
    at(4,16384,[7,6])
    at(4,16384,[6,7],move(5,box(16384)))  # Raw padded bounds differ at the world edge.
    at(4,64,[4],destroy(1))
    at(4,64,[],destroy(3))
    at(4,64,[9],create(3,9,4,box(64)))
    at(4,64,[10,9],create(1,10,4,box(64)))
    steps.append((NO_CHANGE,query(4,(0,64,64),(192,0,0),swept=True,stop=1),[10]))
    return entries, steps


def benchmark(run, execute):
    rows = []
    for model,origin,mins,maxs,_ in benchmark_doors():
        for shift in (0., 0.5, 256.125, -512.5, 0.):
            translated = (origin[0]+shift, origin[1], origin[2])
            values = mins + maxs
            rows.append(CLIENT_QUERY.pack(0xc000, model, 0, 1, 0, 0, 0,
                                          model+1, 1, 1., *translated, 0.,0.,0., *values, *values, *values))
    requests = run/'door-requests.bin'
    requests.write_bytes(b''.join(rows))
    client_outputs = []
    for arch in ('x86','x64'):
        executable = CLIENT_TESTS/f'partition_{arch}.exe'
        data = run/f'door-requests-{arch}.bin'
        execute(executable, [CLIENT if arch=='x86' else '-', requests, data],f'door-requests-{arch}.log')
        client_outputs.append(data.read_bytes())
    assert client_outputs[0] == client_outputs[1]
    observed = list(CLIENT_REPLY.iter_unpack(client_outputs[0]))
    assert all(row[1:7] == (1,row[2],0x28c,0x84,1,row[2]) for row in observed)
    initial, steps = [], []
    for i in range(0,len(observed),5):
        current = observed[i]
        identity = (1<<36) + current[2]
        bounds = current[14:20]
        # Add overlapping records so ordering can affect the selected hit.
        initial.extend([(identity,0,bounds),(identity+(1<<40),4,bounds)])
    for i, current in enumerate(observed):
        handle = i//5*2
        bounds = current[14:20]
        if i%5==0:
            steps.append((lists(handle,current[3],current[4]),query(4,center(bounds)),None))
        steps.append((move(handle,bounds),query(4,center(bounds)),None))
        steps.append((NO_CHANGE,query(0x80,center(bounds)),None))
        for axis in range(3):
            start = list(center(bounds))
            start[axis] = bounds[axis]-100
            delta = tuple(bounds[axis+3]-start[axis]+100 if j==axis else 0 for j in range(3))
            steps.append((NO_CHANGE,query(4,start,delta,True,1),None))
    return initial, steps


def churn():
    rng = random.Random(79640)
    masks = (0, 4, 0x80, 0x84, 0x65, 0x201, 0xffff)
    positions = (-16000,-4096,-1024,-512,-256,-32,63,255,512,1024,4096,16000)
    def random_box():
        return box(rng.choice(positions)+rng.choice((0,.25,.5,8)), half=rng.choice((5,7,127.96875, 128,513,2200)))
    entries = [(10_000+i,rng.choice(masks),random_box()) for i in range(40)]
    live = {i:entry[2] for i,entry in enumerate(entries)}
    free, steps = [], []
    for index in range(800):
        handle = rng.choice(sorted(live))
        bounds = live[handle]
        if index%11==0:
            cmd = destroy(handle)
            free.append(handle)
            del live[handle]
        elif index%11==1:
            handle = free.pop()
            bounds = random_box()
            live[handle] = bounds
            cmd = create(handle,(1<<35)+index,rng.choice(masks),bounds)
        elif index%4==0:
            cmd = lists(handle,rng.choice(masks),rng.choice(masks))
        else:
            if index%5:
                bounds = random_box()
            live[handle] = bounds
            cmd = move(handle,bounds)
        p = center(bounds)
        steps.append((cmd,query(rng.choice(masks),p),None))
        steps.append((NO_CHANGE,query(rng.choice(masks),(p[0]-310,p[1],p[2]),(620,0,0),True,index%3),None))
    return entries, steps


def query_lists(output, entities, count):
    offset = 4+2*entities
    assert struct.unpack_from('<I',output)[0] == entities
    assert struct.unpack_from('<I',output,offset)[0] == count
    offset += 4
    result = []
    for _ in range(count):
        size, = struct.unpack_from('<I',output,offset)
        offset += 4
        result.append(list(struct.unpack_from(f'<{size}Q',output,offset)))
        offset += 8*size
    assert offset == len(output)
    return result


def main():
    protected = {
        ENGINE:'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f',
        TIER0:'6c05ae3d1f32e9f943b1ad30005f55ae69ddfad2f00f969b11d0c6d9c7356251',
        CLIENT:'91dd1062227b095f1338312aa7268121894970c3e0a5b0d314e42c77f2aba73b',
        BSP:'60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea',
    }
    for path,value in protected.items():
        assert sha(path)==value, path.name
    image, evidence = check_ranges(ENGINE,'engine-disassembly.txt',[
        ('Replace lists',0x101bfc70,0x101bfc9f),
        ('Moved dispatch',0x101bfd90,0x101bfdf9),
        ('Tree move',0x101c20c0,0x101c21a3),
        ('Tree remove',0x101c1ff0,0x101c20b4),
        ('Handle destroy',0x101c4080,0x101c4105),
        ('Tree attach and shared box',0x101c09a0,0x101c0a0d),
    ])
    assert all(image.word(0x1035f534+4*slot)==address for slot,address in
               [(0,0x101bead0),(1,0x101c4110),(2,0x101c4080),(6,0x101bfc70),(7,0x101bfd90)])
    assert image.word(0x1035f508)==0x101c20c0
    run = Path(tempfile.mkdtemp(prefix='dynamic-',dir=OUT))
    print(run,flush=True)
    libraries = run/'libraries'
    libraries.mkdir()
    shutil.copyfile(TIER0,libraries/'tier0.dll')

    def execute(exe,args,log):
        result = subprocess.run([exe,*args],cwd=run,capture_output=True,timeout=60)
        (run/log).write_bytes(result.stdout+result.stderr)
        assert result.returncode==0, (log,result.returncode,result.stderr.decode(errors='replace'))

    receipts = {}
    native = OUT/'spatial_partition_native_x86.exe'
    owned = OUT/'spatial_point_partition_owned_x64.exe'
    for name, (entries,steps) in [('lifecycle',lifecycle()),('benchmark-doors',benchmark(run,execute)),('churn',churn())]:
        data = run/f'{name}.input.bin'
        data.write_bytes(struct.pack('<I',len(entries))+b''.join(ENTRY.pack(i,mask,*b) for i,mask,b in entries)+
                         struct.pack('<I',len(steps))+b''.join(cmd+q for cmd,q,_ in steps))
        results = []
        for arch, executable in [('x86',native),('x64',owned)]:
            destination = run/f'{name}-{arch}.bin'
            args = ([ENGINE,libraries] if arch=='x86' else []) + [data,destination,'dynamic']
            execute(executable,args,f'{name}-{arch}.log')
            raw = destination.read_bytes()
            observed = query_lists(raw,len(entries),len(steps))
            stored = Path(str(destination)+'.stored-boxes').read_bytes()
            for step,(_,_,expected) in enumerate(steps):
                assert expected is None or observed[step]==expected,(name,arch,step,observed[step],expected)
            results.append((raw,stored,observed))
        if results[0] != results[1]:
            differing_queries = [i for i,(a,b) in enumerate(zip(results[0][2],results[1][2])) if a!=b]
            differing_stored = next((i//32 for i,(a,b) in enumerate(zip(results[0][1],results[1][1])) if a!=b),None)
            raise AssertionError((name,'query mismatches',differing_queries[:12],'first stored mismatch',differing_stored))
        counts = Counter(MUTATION.unpack(cmd)[0] for cmd,_,_ in steps)
        receipts[name] = dict(queries=len(steps),callbacks=sum(map(len,results[0][2])),moves=counts[1],
                              list_edits=counts[2],destroys=counts[3],creates=counts[4],
                              fields_and_order_match=True,fixture_sha256=sha(data))
        print(name,receipts[name],flush=True)
    assert all(sha(path)==value for path,value in protected.items())
    builds = {exe.name:dict(path=str(exe),sha256=sha(exe)) for exe in (native,owned)}
    result = dict(status='VERIFIED dynamic point partition',scenarios=receipts,builds=builds,
                  installed_engine_bytes_checked=sum(row['bytes'] for row in evidence),originals_unchanged=True)
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(run/'results.json')


if __name__=='__main__':
    main()
