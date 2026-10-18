"""Original overflow wrapper/angular response/directional ambient accumulation."""
import itertools
import json
import math
from pathlib import Path
import random
import struct
import subprocess
import sys
import tempfile
from verify_ifm_draw_packet import check_ranges
from verify_native_leaf_ambient_contract import ENGINE,DIGEST
from verify_session_illumination_dag import IFM,IFM_SHA,SESSION,SESSION_SHA
from verify_dag_world_position import sha,require
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/world-light-overflow-20261015'
MAP=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\maps\mtt_soldier.bsp')
MAP_SHA='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea'
RANGES=[('overflow_wrapper',0x10195510,0x10195576),('angular_response',0x100d1420,0x100d1637),('directional_accumulation',0x10193390,0x10193657),('version0_worldlight_disk_to_runtime',0x100e1f9c,0x100e20bf),('worldlight_lump_choice',0x100e8c4f,0x100e8c92)]
BASIS=[(1,0,0),(-1,0,0),(0,1,0),(0,-1,0),(0,0,1),(0,0,-1)]
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def dot(a,b):return f32(f32(f32(a[1]*b[1])+f32(a[0]*b[0]))+f32(a[2]*b[2]))
def composite_overflow_reference(light,point,ambient):
    """Independent expected response for the composite's eight fixed lights."""
    incoming=(float(light%3-1)*.25,float(light%2)*.5,1.);t=(light-1)%6;d=dot(incoming,incoming)
    if t==0:angular=f32(.5*d)
    elif t in (1,4):angular=d
    elif t==2:angular=f32(math.pow(.5,.5 if light%2 else 2.)*d)
    elif t==3:angular=.5
    else:angular=1.
    attenuation=f32(f32(light*.125)+f32(abs(point[0])*.0009765625));scale=f32(angular*attenuation);result=ambient[:]
    for i,basis in enumerate(BASIS):
        value=dot(basis,incoming)
        if value>0:
            amount=f32(value*scale)
            for c,intensity in enumerate([float(light),light*.5,light*.25]):result[i*3+c]=f32(f32(intensity*amount)+result[i*3+c])
    return result
def main():
    require(sha(ENGINE)==DIGEST and sha(IFM)==IFM_SHA and sha(SESSION)==SESSION_SHA and sha(MAP)==MAP_SHA,'Pinned original changed');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));smoke='--smoke' in sys.argv
    image,evidence=check_ranges(ENGINE,'engine-disassembly.txt',RANGES);require([image.word(0x100d1638+4*i) for i in range(6)]==[0x100d1438,0x100d14b6,0x100d14f4,0x100d15e8,0x100d14b6,0x100d1631],'Angular dispatch table');require(struct.unpack_from('<d',image.b,image.offset(0x1031dcd0))[0]==.01,'Surface threshold')
    executables=[]
    for name,machine in [('overflow_native_x86',0x14c),('overflow_owned_x64',0x8664),('overflow_safety_x64',0x8664)]:
        path=OUT/(name+'.exe');raw=path.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'Architecture/manifest');executables.append(dict(path=str(path),sha256=sha(path),machine=hex(machine)))
    rng=random.Random(2026101553);rows=[];expected=[];meta=[]
    def append(type=1,special=0,count=6,intensity=(2,4,6),normal=(0,0,-.5),inner=.75,outer=.25,exponent=1,point=(10,20,30),incoming=(0,0,1),attenuation=1,initial=None,basis=None,label='analytical'):
        intensity=tuple(map(f32,intensity));normal=tuple(map(f32,normal));inner,outer,exponent=map(f32,(inner,outer,exponent));point=tuple(map(f32,point));incoming=tuple(map(f32,incoming));attenuation=f32(attenuation);initial=list(map(f32,initial if initial is not None else [rng.uniform(-2,2) for _ in range(18)]));basis=[tuple(map(f32,v)) for v in (basis if basis is not None else BASIS)]
        rows.append(struct.pack('<iII52f',type,special,count,*intensity,*normal,inner,outer,exponent,*point,*incoming,attenuation,*initial,*[x for v in basis for x in v]))
        lambert=dot(incoming,incoming);cosine=-dot(normal,incoming);power=False
        if type<0 or type>5:angular=0.
        elif type==5:angular=1.
        elif type==3:angular=0. if cosine<0 else cosine
        elif lambert<0:angular=0.
        elif type==0:
            surface=-((normal[1]*incoming[1]+normal[0]*incoming[0])+normal[2]*incoming[2]);angular=f32(surface*lambert) if surface>.01 else 0.
        elif type in (1,4):angular=lambert
        elif outer>=cosine:angular=0.
        elif cosine>=inner:angular=lambert
        elif exponent in (0.,1.):angular=f32(f32(f32(cosine-outer)/f32(inner-outer))*lambert)
        else:angular=f32(math.pow((cosine-outer)/(inner-outer),exponent)*lambert);power=True
        scale=f32(angular*attenuation);result=initial[:];basis_calls=0;positive=0
        if scale!=0:
            basis_calls=1
            for i in range(count-1,-1,-1):
                d=dot(basis[i],incoming)
                if d>0:
                    positive+=1;amount=f32(d*scale)
                    for channel in range(3):result[i*3+channel]=f32(f32(intensity[channel]*amount)+result[i*3+channel])
        expected.append(struct.pack('<19f4I',angular,*result,1,basis_calls,basis_calls,3 if special else 0));meta.append(dict(type=type,special=special,count=count,power=power,angular=angular,scale=scale,positive=positive,label=label))
    for t in range(-1,8):append(type=t)
    for exponent in [0,.125,.5,1,2,3,4,8]:append(type=2,exponent=exponent,label='spot middle cone')
    for z in [-.75,-.25,0,.01,.75]:append(type=2,normal=(0,0,z),label='cone boundary')
    append(type=0,normal=(0,0,-.01),label='surface threshold below')
    append(type=0,normal=(0,0,-math.nextafter(.01,1)),label='surface threshold float rounding')
    append(attenuation=0,label='zero scale skips basis')
    append(attenuation=-1,label='signed attenuation')
    append(incoming=(0,0,0),type=5,label='zero direction with nonzero scale')
    append(count=0,label='empty basis still queried')
    append(special=1,label='visibility flags three')
    append(incoming=(1,2,3),basis=[(.25,-.5,1)]*6,label='generic supplied directions')
    map_evidence=None
    if not smoke:
        for t,special,count,attenuation,exponent in itertools.product(range(6),range(2),range(7),[-1,0,.25,1],[0,.5,1,2,4]):append(type=t,special=special,count=count,attenuation=attenuation,exponent=exponent,label='exhaustive type/gate/count')
        for i in range(12288):
            incoming=[rng.uniform(-1,1) for _ in range(3)];normal=[rng.uniform(-1,1) for _ in range(3)];outer=rng.uniform(-.8,.4);inner=rng.uniform(outer,1)
            append(type=rng.choice([-1,0,1,2,3,4,5,6]),special=i%2,count=rng.randrange(7),incoming=incoming,normal=normal,inner=inner,outer=outer,exponent=rng.choice([0,.125,.5,1,2,3,4,8]),attenuation=rng.uniform(-2,4),intensity=[rng.uniform(-4,16) for _ in range(3)],basis=[[rng.uniform(-1,1) for _ in range(3)] for _ in range(6)],label='finite controlled angular/cube')
        bsp=MAP.read_bytes();require(bsp[:4]==b'VBSP' and struct.unpack_from('<I',bsp,4)[0]==20,'Pinned map version');lumps=[]
        for lump in [15,54]:
            offset,length,version,fourcc=struct.unpack_from('<4I',bsp,8+lump*16);require(offset<=len(bsp) and length<=len(bsp)-offset and length%88==0 and fourcc==0 and version==0,'World-light version0 extent/compression');types={};n=length//88
            for i in range(n):
                # Original100E2030..20BE copies88-byte disk records to100-byte
                # runtime records, inserting12 zero bytes at runtime+24h.
                # Disk type28h/cone30h therefore become runtime34h/3Ch.
                o=offset+i*88;intensity=struct.unpack_from('<3f',bsp,o+12);normal=struct.unpack_from('<3f',bsp,o+24);t=struct.unpack_from('<i',bsp,o+40)[0];inner,outer,exponent=struct.unpack_from('<3f',bsp,o+48);require(0<=t<=5,'Invalid benchmark disk light type');types[str(t)]=types.get(str(t),0)+1
                # Actual angular/intensity metadata; incoming visibility results
                # remain controlled, not claims of live world-light visibility.
                for incoming in [(0,0,1),(1,0,0),(0,-1,0),tuple(-x for x in normal)]:append(type=t,intensity=intensity,normal=normal,inner=inner,outer=outer,exponent=exponent,incoming=incoming,label='actual BSP world-light metadata')
            lumps.append(dict(lump=lump,version=version,count=n,types=types))
        map_evidence=dict(path=str(MAP),sha256=MAP_SHA,lumps=lumps)
    query=run/'queries.bin';query.write_bytes(b''.join(rows));outputs=[];logs=[]
    def execute(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],capture_output=True,text=True,timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' exit '+str(p.returncode)+' '+p.stderr);return p.stdout.strip()
    for name in ['overflow_native_x86','overflow_owned_x64']:
        target=run/(name+'.bin');logs.append(execute(name,[ENGINE,query,target],name));outputs.append(target.read_bytes());require(len(outputs[-1])==len(rows)*92,'Result extent')
    wanted=b''.join(expected);different=[i for i in range(len(rows)) if outputs[0][i*92:(i+1)*92]!=outputs[1][i*92:(i+1)*92] or outputs[0][i*92:(i+1)*92]!=wanted[i*92:(i+1)*92]]
    details=[dict(case=i,inputs=meta[i],native=struct.unpack_from('<23I',outputs[0],i*92),owned=struct.unpack_from('<23I',outputs[1],i*92),expected=struct.unpack_from('<23I',wanted,i*92)) for i in different[:4]];(run/'comparison.json').write_text(json.dumps(dict(differences=len(different),details=details),indent=2)+'\n');require(not different,'Overflow comparison differs: '+str(run/'comparison.json'))
    safety=execute('overflow_safety_x64',[],'safety');guards=[];malformed=[]
    for label,offset,value,fmt in [('special',4,2,'I'),('basis_count',8,7,'I'),('intensity_nan',12,float('nan'),'f'),('incoming_inf',60,float('inf'),'f'),('attenuation_inf',72,float('inf'),'f'),('ambient_nan',76,float('nan'),'f'),('basis_inf',148,float('inf'),'f')]:
        bad=bytearray(rows[0]);struct.pack_into('<'+fmt,bad,offset,value);malformed.append((label,bytes(bad)))
    bad=bytearray(rows[0]);struct.pack_into('<i',bad,0,2);struct.pack_into('<f',bad,44,-1);malformed.append(('negative_cone_exponent',bytes(bad)));malformed.append(('truncated',rows[0][:-1]))
    for label,data in malformed:
        path=run/('invalid-'+label+'.bin');path.write_bytes(data)
        for name in ['overflow_native_x86','overflow_owned_x64']:
            p=subprocess.run([str(OUT/(name+'.exe')),str(ENGINE),str(path),str(run/('invalid-'+label+'-'+name+'.bin'))],capture_output=True,text=True,timeout=20);require(p.returncode==1,'Malformed fixture accepted/faulted '+label+' '+name);guards.append(dict(test=label,executable=name,message=p.stderr.strip()))
    coverage=dict(power=sum(m['power'] for m in meta),zero_scale=sum(m['scale']==0 for m in meta),nonzero_scale=sum(m['scale']!=0 for m in meta),positive_faces=sum(m['positive'] for m in meta),empty_basis=sum(m['count']==0 for m in meta),special=sum(m['special'] for m in meta),types={str(t):sum(m['type']==t for m in meta) for t in range(6)});require(all(coverage[k] for k in ['power','zero_scale','nonzero_scale','positive_faces','empty_basis','special']) and all(coverage['types'].values()),'Coverage gap')
    require(sha(ENGINE)==DIGEST and sha(IFM)==IFM_SHA and sha(SESSION)==SESSION_SHA and sha(MAP)==MAP_SHA,'Original modified')
    sources={n:sha(ROOT/'work'/n) for n in ['source_world_light_overflow_candidate.h','probe_source_world_light_overflow.cpp','test_source_world_light_overflow.cpp','build_source_world_light_overflow.cmd','verify_source_world_light_overflow.py']}
    receipt=dict(status='PASS',smoke=smoke,cases=len(rows),bytes_per_result=92,differences=0,coverage=coverage,executables=executables,logs=logs,safety=safety,guards=guards,query_sha256=sha(query),source_sha256=sources,map_evidence=map_evidence,engine_sha256=DIGEST,ifm_sha256=IFM_SHA,session_sha256=SESSION_SHA,map_sha256=MAP_SHA,static_bytes=sum(e['bytes'] for e in evidence),static_evidence=evidence,limits=['Full original10195510 wrapper,100D1420 angular response including original CRT pow and10193390 direction accumulation execute','Original10194420 direction/visibility/distance/style query is a controlled callback; actual visibility/tracing and falloff remain unverified','Original basis pointer/count virtuals are controlled, count0..6; actual live renderer basis not established','Finite domain with original x87 host environment, valid spotlight cones; tests do not prove every possible pow input exact','Actual BSP light metadata inputs do not prove live scene light selection or visibility','No editor integration/build, application launch, dependencies or original writes'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ['status','smoke','cases','differences','coverage','safety']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
