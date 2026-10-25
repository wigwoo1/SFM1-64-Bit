"""Private original attachment query, owned x64 and downstream engine composition."""
from datetime import datetime, timezone
import hashlib
import itertools
import json
from pathlib import Path
import random
import re
import struct
import subprocess
import tempfile
from verify_ifm_draw_packet import check_ranges
from verify_session_illumination_dag import IFM,IFM_SHA
from verify_studio_vcollide_ready import DATACACHE,DATACACHE_SHA

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/attachment-matrix-20261024'
GAME=IFM.parent.parent.parent
ENGINE=GAME/'bin/engine.dll'
IDENTITY=[1,0,0,0,0,1,0,0,0,0,1,0]


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def require(v,m):
    if not v:raise RuntimeError(m)
def f32(v):return struct.unpack('<f',struct.pack('<f',v))[0]
def point(p,m):
    return [f32(f32(f32(f32(m[r*4+1]*p[1])+f32(m[r*4]*p[0]))+f32(m[r*4+2]*p[2]))+m[r*4+3]) for r in range(3)]


def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    originals={str(p):sha(p) for p in [IFM,ENGINE,DATACACHE,GAME/'usermod/gameinfo.txt']}
    require(originals[str(IFM)]==IFM_SHA and originals[str(ENGINE)]=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f' and originals[str(DATACACHE)]==DATACACHE_SHA,'Original pins')
    tracked=['ifm_attachment_matrix_candidate.h','source_mdl_attachment_capacity_candidate.h','probe_ifm_attachment_matrix.cpp',
        'test_ifm_attachment_matrix.cpp','build_ifm_attachment_matrix.cmd','verify_ifm_attachment_matrix.py','probe_model_lighting_point.cpp',
        'source_model_lighting_point_candidate.h','ifm_bounds_frames_candidate.h','probe_vulkan_win32_surface_x64.cpp']
    sources={p:sha(ROOT/'work'/p) for p in tracked}
    image,ifm_evidence=check_ranges(IFM,'ifm-disassembly.txt',[
        ('complete_attachment_query',0x1023c3d0,0x1023c516),('count_normal_or_virtual',0x105d9870,0x105d9884),
        ('header_include_virtual_initialization_gate',0x105dab40,0x105dab8e),
        ('complete_normal_virtual_descriptor',0x105da640,0x105da697),('complete_normal_virtual_bone_map',0x105da6a0,0x105da6f4),
        ('point_transform',0x10498390,0x10498431),
    ])
    _,cache_evidence=check_ranges(DATACACHE,'datacache-disassembly.txt',[
        ('complete_attachment_append',0x10012380,0x10012656),
        ('recursive_group_append_call',0x10013b5c,0x10013b9b),('include_model_table_resolution',0x10013b9b,0x10013c12),
    ])
    require(image.word(0x10adb904+34*4)==0x1023c3d0,'Renderable slot34')
    warnings={}
    for name in ['native','owned','safety','point-native','point-owned','asm']:
        log=(OUT/(name+'-build.log')).read_text();warnings[name]=[x for x in log.splitlines() if 'warning ' in x.lower()];require(not warnings[name] and 'error C' not in log,'Build diagnostics '+name)
    executables=[]
    for name,machine in [('attachment_native_x86',0x14c),('attachment_owned_x64',0x8664),('attachment_safety_x64',0x8664),('point_native_x86',0x14c),('point_owned_x64',0x8664)]:
        exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==(0x10b if machine==0x14c else 0x20b),'PE identity '+name)
        executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine)))
    def execute(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=ROOT,capture_output=True,text=True,timeout=90)
        (run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' '+str(p.returncode)+' '+p.stdout+p.stderr);return p.stdout
    safety=execute('attachment_safety_x64',[],'safety');require('checks=7 rejections=9' in safety,'Safety coverage')
    capacity=execute('attachment_safety_x64',[GAME/'usermod/gameinfo.txt','models/player/hwm/pyro.mdl'],'pyro-capacity')
    require('upper_bound=10 expanded_groups=2' in capacity,'Actual included model capacity changed')
    models=[]
    for path in re.findall(r' file=(.+)',capacity):
        p=Path(path);raw=p.read_bytes();local,start=struct.unpack_from('<ii',raw,0xf0);includes,offset=struct.unpack_from('<ii',raw,0x150);children=[]
        for i in range(includes):
            at=offset+i*8;name=at+struct.unpack_from('<i',raw,at+4)[0];children.append(raw[name:raw.index(0,name)].decode('ascii'))
        models.append(dict(path=path,sha256=sha(p),local_attachments=local,includes=children))
    require(len(models)==2 and [m['local_attachments'] for m in models]==[10,0] and models[1]['includes']==[],'Independent include closure/capacity')
    require(models[0]['sha256']=='450f8b480e0b85ed79b8747dae79c6d537cde1342bff4e77e5216d3008eabdd1','Pyro model pin')
    raw=Path(models[0]['path']).read_bytes();extra=struct.unpack_from('<i',raw,0x190)[0];pyro_index=struct.unpack_from('<i',raw,extra+8)[0];require(pyro_index==34 and pyro_index>sum(m['local_attachments'] for m in models),'Pyro attachment refusal proof')
    rng=random.Random(2026102426);rows=[];metadata=[]
    def append(available=1,index=1,virtual=0,local_count=1,virtual_count=1,flags=0,setup=0,tick=50000,p=(10,20,30),angles=(0,0,0),local=IDENTITY,bone=IDENTITY):
        h=(available,index,virtual,local_count,virtual_count,flags,setup,tick);values=list(p)+list(angles)+list(local)+list(bone)
        rows.append(struct.pack('<IiIiiIIi30f',*h,*values));metadata.append(h)
    append(index=34,local_count=10,virtual=1,virtual_count=10)
    translated=IDENTITY.copy();translated[3:12:4]=[2,3,4];world=IDENTITY.copy();world[3:12:4]=[10,20,30]
    append(local=translated,bone=world);append(flags=0x10000,local=translated,bone=world)
    for args in itertools.product([0,1],[-2147483648,0,1,2,10,11,34,2147483647],[0,1],[-1,0,1,10],[-1,0,2,10,40],[0,0x10000],[0,1]):append(*args)
    for i in range(4096):
        available=rng.randrange(2);index=rng.choice([-1,0,1,10,34,40]);virtual=rng.randrange(2);a=rng.choice([0,1,10,40]);b=rng.choice([0,1,10,40]);flags=rng.choice([0,1,0x10000,0xffffffff]);valid=bool(available and index>=1 and (b if virtual else a)>=index)
        local=[rng.uniform(-8,8) for _ in range(12)];bone=[rng.uniform(-8,8) for _ in range(12)];p=[rng.uniform(-4096,4096) for _ in range(3)];angles=[rng.uniform(-36000,36000) for _ in range(3)]
        if valid and flags&0x10000:local[0]=float('nan') # unused orientation
        if valid and i%3==0:p[0]=float('nan') # unused fallback
        append(available,index,virtual,a,b,flags,rng.randrange(2),rng.choice([-2147483648,-1,0,50000,2147483647]),p,angles,local,bone)
    query=run/'queries.bin';query.write_bytes(b''.join(rows));outputs=[]
    for name in ['attachment_native_x86','attachment_owned_x64']:
        p=run/(name+'.bin');execute(name,[IFM,query,p],name);outputs.append(p.read_bytes())
    require(outputs[0]==outputs[1] and len(outputs[0])==80*len(rows),'Attachment result/matrix/events differ')
    successes=0;failures=0
    for i,h in enumerate(metadata):
        valid=bool(h[0] and h[1]>=1 and (h[4] if h[2] else h[3])>=h[1]);record=struct.unpack_from('<I12f7I',outputs[0],i*80)
        require(record[0]==valid,'Independent valid predicate');counts=record[13:]
        if valid:
            successes+=1;time=f32(f32(h[7]) * f32(.0001));bits=struct.unpack('<I',struct.pack('<f',time))[0]
            require(counts==(1,0,0x200,bits,1,1,h[1]-1),'Setup/descriptor/mapped-bone contract')
        else:failures+=1;require(counts==(0,)*7,'Rejected index did work')
    require(struct.unpack_from('<12f',outputs[0],4)==tuple(world),'Independent Pyro-like fallback frame')
    for i in [1,2]:require(struct.unpack_from('<12f',outputs[0],i*80+4)==(1,0,0,12,0,1,0,23,0,0,1,34),'Independent attachment translation')
    # Original IFM result bytes feed a separate original engine process. Engine
    # callback returns that result; no engine/IFM images share preferred base.
    point_rows=[];expected=[]
    for i,row in enumerate(rows):
        unpacked=struct.unpack('<IiIiiIIi30f',row);h=metadata[i];v=unpacked[8:];valid=struct.unpack_from('<I',outputs[0],i*80)[0];attachment=struct.unpack_from('<12f',outputs[0],i*80+4)
        local_point=[0,0,0] if i==0 else [f32(rng.uniform(-64,64)) for _ in range(3)];model=world
        explicit=[1.25,-2.5,3.75];use_explicit=i%3==1;flags=1|(2 if valid else 0)|(8 if use_explicit else 0)
        point_rows.append(struct.pack('<Ii42f',flags,h[1],*local_point,*model,*attachment,*IDENTITY,*explicit))
        base=point(local_point,attachment if h[1]>0 and valid else model);final=explicit if use_explicit else base
        expected.append(struct.pack('<6fIi',*base,*final,int(h[1]>0),h[1] if h[1]>0 else 0))
    pq=run/'engine-queries.bin';pq.write_bytes(b''.join(point_rows));engine_outputs=[]
    for name in ['point_native_x86','point_owned_x64']:
        p=run/(name+'.bin');execute(name,[ENGINE,pq,p],name);engine_outputs.append(p.read_bytes())
    require(engine_outputs[0]==engine_outputs[1]==b''.join(expected),'Downstream engine composition differs')
    require(all(sha(p)==h for p,h in originals.items()) and all(sha(m['path'])==m['sha256'] for m in models),'Original changed')
    require(all(sha(ROOT/'work'/p)==h for p,h in sources.items()) and all(sha(e['path'])==e['sha256'] for e in executables),'Source/binary changed')
    receipt=dict(status='PASS',checked_at_utc=datetime.now(timezone.utc).isoformat(),attachment_cases=len(rows),valid_cases=successes,rejected_cases=failures,
        exact_attachment_bytes=len(outputs[0]),engine_compositions=len(point_rows),exact_engine_bytes=len(engine_outputs[0]),safety=safety.strip(),models=models,
        pyro_attachment_index=pyro_index,pyro_capacity_upper_bound=10,capacity=capacity,executables=executables,warnings=warnings,original_sha256=originals,
        sources_sha256=sources,queries_sha256=sha(query),engine_queries_sha256=sha(pq),static_instruction_bytes=sum(e['bytes'] for e in ifm_evidence+cache_evidence),
        static_ranges=[{k:v for k,v in e.items() if k!='instructions'} for e in ifm_evidence+cache_evidence],
        limitations=[
            'Complete IFM1023C3D0 plus original count/math/accessors execute privately. Header presence/counts/clock/setup and descriptor/mapped-bone services are controlled; real virtual-table construction does not execute.',
            'Capacity is a conservative bound from complete checked MDL include closure plus byte-checked append semantics, not exact virtual table ordering/bone remapping. Index34 exceeds sum10 even before filtering/deduplication.',
            'Engine100D2780/100D5760 selection is composed in a separate private process with explicit default model matrix/optional origin. Original IFM callback result crosses through a controlled bridge.',
            'Actual benchmark renderable root-frame position/angles/center selection and native cache synchronization remain unverified. This test does not establish an actual Pyro lighting point or rendered pixels.',
            'No existing editor dependency changed/rebuilt/run; no computer use, original process probes, SFM launch/control/close, Blender, GPU job or original write.',
        ])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ['status','attachment_cases','valid_cases','rejected_cases','engine_compositions','pyro_capacity_upper_bound']}));print('receipt='+str(run/'results.json'))


if __name__=='__main__':main()
