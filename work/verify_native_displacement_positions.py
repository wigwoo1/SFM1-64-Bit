"""Compare a bounded x64 grid generator with original isolated CoreDisp code.

The original installation is only read. Quads here are controlled inputs;
this does not execute the engine's complete BSP-to-CoreDisp construction.
"""
import json
import random
import struct
import subprocess
import sys
import tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT, ENGINE, MAP, DIGEST, sha, require

OUT = ROOT / 'outputs/native-displacement-positions-20261015'
RANGES = [(0x101415d0,0x10141bcf),(0x100b4e20,0x100b4e24),
          (0x10001010,0x10001033),(0x100b4ce0,0x100b4ce3),
          (0x1013c890,0x1013c8b6),(0x1013cbb0,0x1013cbbc),
          (0x1006d8c0,0x1006d8f8),(0x10001120,0x10001144),
          (0x1013c950,0x1013c9dc),(0x1000bd80,0x1000bdb7),
          (0x1000bd40,0x1000bd77),(0x10001150,0x1000116c),
          (0x102de362,0x102de36c)]

def original_fragments():
    require(sha(ENGINE) == DIGEST, 'Engine changed')
    image = Image(ENGINE)
    blobs = [image.b[image.offset(a):image.offset(a)+(z-a)] for a,z in RANGES]
    require(image.word(0x1031bdec) == 0x3f800000, 'Expected float one')
    require(image.word(0x1031be7c) == 0, 'Expected float zero')
    header = '// Checked original CoreDisp grid-position routine and helpers.\n'
    for i,raw in enumerate(blobs):
        header += 'static const unsigned char positionPart%d[]={%s};\n' % (i, ','.join('0x%02x'%v for v in raw))
    header += 'struct PositionFragment{unsigned offset;const unsigned char*bytes;unsigned size;};\n'
    header += 'static const PositionFragment positionFragments[]={' + ','.join('{0x%x,positionPart%d,sizeof(positionPart%d)}'%(a-0x10000000,i,i) for i,(a,z) in enumerate(RANGES)) + '};\n'
    return header,blobs

def benchmark_queries():
    require(sha(MAP)=='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','Map changed')
    data = MAP.read_bytes()
    def lump(index, stride):
        offset,size,_,_=struct.unpack_from('<4I',data,8+16*index)
        require(offset+size<=len(data) and size%stride==0,'Invalid lump')
        return data[offset:offset+size]
    vertices=lump(3,12); edges=lump(12,4); surfedges=lump(13,4)
    faces=lump(58,56) or lump(7,56); infos=lump(26,176); offsets=lump(33,20)
    queries=[]
    for i in range(len(infos)//176):
        d=176*i; power=struct.unpack_from('<i',infos,d+20)[0]
        face=struct.unpack_from('<H',infos,d+36)[0]*56
        first,edge_count,_,disp=struct.unpack_from('<ihhh',faces,face+4)
        require(edge_count==4 and disp==i,'Quad reverse identity')
        corners=[]
        for e in range(4):
            se=struct.unpack_from('<i',surfedges,(first+e)*4)[0]
            vi=struct.unpack_from('<H',edges,abs(se)*4+(2 if se<0 else 0))[0]
            corners.append(struct.unpack_from('<3f',vertices,vi*12))
        start=struct.unpack_from('<3f',infos,d)
        distances=[sum((a-b)**2 for a,b in zip(c,start)) for c in corners]
        nearest=min(range(4),key=distances.__getitem__)
        require(distances[nearest]<1e-4,'Start corner missing')
        corners=corners[nearest:]+corners[:nearest]
        count=((1<<power)+1)**2
        offset=struct.unpack_from('<I',infos,d+12)[0]*20
        raw=offsets[offset:offset+20*count]
        require(len(raw)==20*count,'Offset coverage')
        queries.append(struct.pack('<I12f',power,*(x for c in corners for x in c))+raw)
    return queries

def main():
    header,blobs=original_fragments(); hp=ROOT/'work/native_displacement_position_bytes.h'
    if '--prepare' in sys.argv:
        hp.write_text(header); print('Prepared checked native grid generator'); return
    require(hp.read_text()==header,'Stale native code')
    qs=benchmark_queries(); benchmark_count=len(qs)
    rng=random.Random(202610151)
    for i in range(1024):
        power=1+i%4; count=((1<<power)+1)**2
        corners=[rng.uniform(-8192,8192) for _ in range(12)]
        if i<32: corners=[rng.choice([-1.,-0.,0.,1.]) for _ in range(12)]
        offsets=[]
        for v in range(count):
            direction=[rng.uniform(-1,1) for _ in range(3)]
            distance=rng.uniform(-256,256)
            if i<32: direction=[rng.choice([-1.,-0.,0.,1.]) for _ in range(3)]; distance=rng.choice([-1.,-0.,0.,1.])
            offsets.append(struct.pack('<5f',*direction,distance,rng.uniform(0,255)))
        qs.append(struct.pack('<I12f',power,*corners)+b''.join(offsets))
    OUT.mkdir(exist_ok=True); run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    inp=run/'queries.bin'; inp.write_bytes(b''.join(qs)); outputs=[]; executables=[]
    names=[('native_x86',0x14c)] if '--native-only' in sys.argv else [('native_x86',0x14c),('owned_x64',0x8664)]
    for name,machine in names:
        exe=OUT/('positions_'+name+'.exe'); binary=exe.read_bytes(); pe=struct.unpack_from('<I',binary,60)[0]
        require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture')
        if name=='native_x86': require(all(b in binary for b in blobs),'Original code absent')
        target=run/(name+'.bin')
        p=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=45)
        target.with_suffix('.log').write_text(p.stdout+p.stderr)
        require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode))
        result=target.read_bytes(); pos=0; total=0
        for q in qs:
            power=struct.unpack_from('<I',q)[0]; count=((1<<power)+1)**2
            require(struct.unpack_from('<I',result,pos)[0]==count,'Grid count'); pos+=4+12*count; total+=count
        require(pos==len(result),'Output length')
        outputs.append(result); executables.append(dict(executable=str(exe),sha256=sha(exe)))
    different=[]
    if len(outputs)==2:
        pos=0
        for i,q in enumerate(qs):
            count=((1<<struct.unpack_from('<I',q)[0])+1)**2; length=4+12*count
            if outputs[0][pos:pos+length]!=outputs[1][pos:pos+length]: different.append(i)
            pos+=length
    require(sha(ENGINE)==DIGEST,'Engine changed after execution')
    require(sha(MAP)=='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','Map changed after execution')
    receipt=dict(status='CHARACTERIZED' if len(outputs)==1 else ('PASS' if not different else 'FAIL'),cases=len(qs),benchmark_quads=benchmark_count,vertices=total,different_cases=different[:20],executables=executables,engine_sha256=sha(ENGINE),map_sha256=sha(MAP),native_code_bytes=sum(len(b) for b in blobs),limits=['Grid positions only, zero elevation and zero subdivision offsets','Benchmark quads reconstructed from raw face edges and start corner; complete original loader not executed','Triangle topology, collision tree, runtime bounds and leaf assignment not combined','Not integrated into editor/rendering'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'],cases=len(qs),vertices=total,different_count=len(different),receipt=str(run/'results.json'))))
    require(not different,'Grid positions differ')

if __name__=='__main__':main()
