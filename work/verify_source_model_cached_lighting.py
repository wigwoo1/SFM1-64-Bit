"""Execute original indexed lookup, cached cube add, list copy and list merge."""
import itertools
import json
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
COMPOSITE='--real-overflow' in sys.argv
OUT=ROOT/('outputs/world-light-overflow-20261015' if COMPOSITE else 'outputs/model-cached-lighting-20261015')
RANGES=[('instance_table_lookup',0x100d5804,0x100d5845),('cached_post_query',0x100d58c0,0x100d5a83),('list_copy_only',0x100d0f20,0x100d0f54),('ordered_light_merge',0x100d2370,0x100d242f)]
if COMPOSITE:
    from verify_source_world_light_overflow import RANGES as OVERFLOW_RANGES,composite_overflow_reference
    RANGES+=OVERFLOW_RANGES
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def main():
    require(sha(ENGINE)==DIGEST and sha(IFM)==IFM_SHA and sha(SESSION)==SESSION_SHA,'Pinned original changed');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));smoke='--smoke' in sys.argv
    _,evidence=check_ranges(ENGINE,'engine-disassembly.txt',RANGES)
    executables=[]
    for name,machine in [('cached_native_x86',0x14c),('cached_owned_x64',0x8664),('cached_safety_x64',0x8664)]:
        path=OUT/(name+'.exe');raw=path.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'Architecture/manifest');executables.append(dict(path=str(path),sha256=sha(path),machine=hex(machine)))
    rng=random.Random(2026101552);rows=[];answers=[];meta=[];prior_evidence=None
    def append(index=2,extent=8,flags=7,currentIds=(1,2),cachedIds=(2,3),config=4,hw=(4,4),point=(10,20,30),current=None,cached=None,initial=None,label='analytical'):
        current=[f32(x) for x in (current if current is not None else [rng.uniform(-2,5) for _ in range(18)])];cached=[f32(x) for x in (cached if cached is not None else [rng.uniform(-2,5) for _ in range(18)])];initial=[f32(x) for x in (initial if initial is not None else [rng.uniform(-10,10) for _ in range(18)])];point=tuple(map(f32,point));header=[index,extent,flags,len(currentIds),len(cachedIds),config&0xffffffff,hw[0]&0xffffffff,hw[1]&0xffffffff]
        rows.append(struct.pack('<8I57f8I',*header,*current,*cached,*initial,*point,*(list(currentIds)+[0]*(4-len(currentIds))),*(list(cachedIds)+[0]*(4-len(cachedIds)))))
        selected=index!=65535;active=selected and bool(flags&4);entered=bool(flags&1 and flags&2 and active);draw=initial[:];scratch=[0.0]*18;ids=[];overflows=[];hardware=[];duplicates=full=0
        if entered:
            draw=[f32(a+b) for a,b in zip(current,cached)];ids=list(cachedIds)
            for light in currentIds:
                if len(ids)>=4:full+=1;continue
                if light in ids:duplicates+=1;continue
                capacity=hw[len(hardware)%2];hardware.append(capacity)
                if capacity<config:
                    effective=hw[len(hardware)%2];hardware.append(effective)
                else:effective=config
                if len(ids)<effective:ids.append(light)
                else:
                    overflows.append(light)
                    if COMPOSITE:scratch=composite_overflow_reference(light,point,scratch)
                    else:
                        for j in range(18):
                            value=f32(f32(light*.125)+f32(point[j%3]*.0009765625));scratch[j]=f32(scratch[j]+value)
        answers.append(struct.pack('<4I36f9I8I',index if selected else 65535,int(active),len(hardware),len(overflows),*draw,*scratch,len(ids),*(ids+[0]*(4-len(ids))),*(overflows+[0]*(4-len(overflows))),*([x&0xffffffff for x in hardware]+[0]*(8-len(hardware)))))
        meta.append(dict(label=label,index=index,flags=flags,current=list(currentIds),cached=list(cachedIds),config=config,hw=hw,entered=entered,duplicate_skips=duplicates,full_skips=full,hardware_calls=len(hardware),overflow=len(overflows)))
    append(current=[1]*18,cached=[2]*18,initial=[9]*18)
    append(index=65535,flags=7,label='sentinel no instance')
    append(index=65534,extent=65535,label='highest valid instance')
    for flags in [0,3,5,6]:append(flags=flags,label='inactive gate')
    append(currentIds=(1,1,1,2),cachedIds=(1,),label='repeated identities')
    append(currentIds=(5,6,7,8),cachedIds=(1,2,3,4),config=0,label='four skips without overflow')
    append(currentIds=(1,1,2,3),cachedIds=(),config=0,label='overflow does not add identity')
    append(currentIds=(1,2,3,4),cachedIds=(),hw=(3,0),label='second hardware call changes capacity')
    append(currentIds=(1,2,3,4),cachedIds=(),config=2,hw=(1,8),label='second hardware value can exceed configured snapshot')
    append(currentIds=(),cachedIds=(1,1,2,2),label='cached duplicate list retained')
    append(config=-2147483648,hw=(2147483647,-1),label='signed limit boundaries')
    append(current=[-0.0]*18,cached=[0.0]*18,label='signed zero addition')
    if not smoke:
        for flags,n,m,config,hw,pattern in itertools.product(range(8),range(5),range(5),[-2,0,1,2,3,4,5],[(-1,4),(0,0),(1,4),(3,0),(4,4),(8,2)],range(3)):
            source=[1,2,3,4] if pattern==0 else [4,3,2,1] if pattern==1 else [1,1,2,2];dest=[3,4,5,6] if pattern==0 else [1,2,3,4] if pattern==1 else [1,1,1,2]
            append(index=(flags+n+m)%8,flags=flags,currentIds=source[:n],cachedIds=dest[:m],config=config,hw=hw,label='exhaustive gates/counts/limits/order')
        for i in range(8192):
            index=rng.choice([0,7,65534,65535]);append(index=index,extent=65535,flags=rng.randrange(8),currentIds=[rng.randrange(1,9) for _ in range(rng.randrange(5))],cachedIds=[rng.randrange(1,9) for _ in range(rng.randrange(5))],config=rng.choice([-2147483648,-1,0,1,3,4,2147483647]),hw=tuple(rng.randrange(-1,9) for _ in range(2)),point=[rng.uniform(-32768,32768) for _ in range(3)],label='finite controlled')
        prior=ROOT/'outputs/model-cache-query-20261015/verification-2i5yb4kj';receipt=json.loads((prior/'results.json').read_text());require(receipt['status']=='PASS','Prior query receipt');queries=(prior/'queries.bin').read_bytes();raw=(prior/'query_native_x86.bin').read_bytes();require(len(raw)==receipt['cases']*196 and sha(prior/'queries.bin')==receipt['query_sha256'],'Prior input/output extent');prior_evidence=dict(receipt=str(prior/'results.json'),receipt_sha256=sha(prior/'results.json'),output_sha256=sha(prior/'query_native_x86.bin'))
        pos=0
        for i in range(receipt['cases']):
            h=struct.unpack_from('<8I',queries,pos);pos+=228+h[6]+h[7]
            if not h[0]&1:continue
            cube=struct.unpack_from('<18f',raw,i*196+104);n=struct.unpack_from('<I',raw,i*196+176)[0];require(n<=4,'Prior finite light count');tokens=struct.unpack_from('<4I',raw,i*196+180);mapping={};ids=[]
            for t in tokens[:n]:
                if t not in mapping:mapping[t]=len(mapping)+1
                ids.append(mapping[t])
            append(current=cube,currentIds=ids,cachedIds=[2,3][:i%3],flags=7,config=i%6,hw=(4,2),label='actual prior returned finite query payload')
        require(pos==len(queries),'Prior variable record extent')
    query=run/'queries.bin';query.write_bytes(b''.join(rows));outputs=[]
    def execute(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],capture_output=True,text=True,timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' exit '+str(p.returncode)+' '+p.stderr);return p.stdout.strip()
    logs=[]
    for name in ['cached_native_x86','cached_owned_x64']:
        target=run/(name+'.bin');logs.append(execute(name,[ENGINE,query,target],name));outputs.append(target.read_bytes());require(len(outputs[-1])==len(rows)*228,'Output extent')
    expected=b''.join(answers);different=[i for i in range(len(rows)) if outputs[0][i*228:(i+1)*228]!=outputs[1][i*228:(i+1)*228] or outputs[0][i*228:(i+1)*228]!=expected[i*228:(i+1)*228]]
    details=[dict(case=i,inputs=meta[i],native=struct.unpack_from('<57I',outputs[0],i*228),owned=struct.unpack_from('<57I',outputs[1],i*228),expected=struct.unpack_from('<57I',expected,i*228)) for i in different[:4]]
    (run/'comparison.json').write_text(json.dumps(dict(differences=len(different),details=details),indent=2)+'\n');require(not different,'Cached lighting comparison differs: '+str(run/'comparison.json'))
    safety=execute('cached_safety_x64',[],'safety');guards=[]
    malformed=[]
    for label,offset,value,fmt in [('table_index',0,8,'I'),('table_extent',4,1,'I'),('gate_flags',8,8,'I'),('current_count',12,5,'I'),('cached_count',16,5,'I'),('nonfinite_cube',32,float('inf'),'f'),('nonfinite_point',248,float('nan'),'f'),('light_id',260,0,'I')]:
        bad=bytearray(rows[0]);struct.pack_into('<'+fmt,bad,offset,value);malformed.append((label,bytes(bad)))
    malformed.append(('truncation',rows[0][:-1]))
    for label,data in malformed:
        path=run/('invalid-'+label+'.bin');path.write_bytes(data)
        for name in ['cached_native_x86','cached_owned_x64']:
            p=subprocess.run([str(OUT/(name+'.exe')),str(ENGINE),str(path),str(run/('invalid-'+label+'-'+name+'.bin'))],capture_output=True,text=True,timeout=20);require(p.returncode==1,'Malformed fixture accepted/faulted '+label+' '+name);guards.append(dict(test=label,executable=name,message=p.stderr.strip()))
    coverage=dict(entered=sum(m['entered'] for m in meta),skipped=sum(not m['entered'] for m in meta),sentinel=sum(m['index']==65535 for m in meta),highest=sum(m['index']==65534 for m in meta),duplicates=sum(m['duplicate_skips'] for m in meta),full_skips=sum(m['full_skips'] for m in meta),hardware_calls=sum(m['hardware_calls'] for m in meta),overflow_calls=sum(m['overflow'] for m in meta),second_hardware=sum(m['hardware_calls']>1 for m in meta));require(all(coverage.values()),'Coverage gap')
    require(sha(ENGINE)==DIGEST and sha(IFM)==IFM_SHA and sha(SESSION)==SESSION_SHA,'Original modified')
    source_names=['source_model_cached_lighting_candidate.h','probe_source_model_cached_lighting.cpp','test_source_model_cached_lighting.cpp','build_source_model_cached_lighting.cmd','verify_source_model_cached_lighting.py']
    if COMPOSITE:source_names+=['source_world_light_overflow_candidate.h','world_light_overflow_composite_fixture.h','build_source_world_light_overflow.cmd','verify_source_world_light_overflow.py']
    sources={name:sha(ROOT/'work'/name) for name in source_names}
    receipt=dict(status='PASS',smoke=smoke,real_overflow=COMPOSITE,cases=len(rows),bytes_per_result=228,differences=0,coverage=coverage,executables=executables,logs=logs,safety=safety,guards=guards,query_sha256=sha(query),prior_evidence=prior_evidence,source_sha256=sources,engine_sha256=DIGEST,ifm_sha256=IFM_SHA,session_sha256=SESSION_SHA,static_bytes=sum(e['bytes'] for e in evidence),static_evidence=evidence,limits=['Original indexed record lookup and cached post-query add/copy/merge execute; acquisition bypasses previously verified block through test jump at100D5845',('Original full overflow10195510/angular100D1420/cube10193390 execute, with controlled direction/visibility10194420 and renderer basis/hardware; count bridge invokes real overflow, not replacement math' if COMPOSITE else 'Overflow ambient contribution10195510 and hardware capability virtual are controlled callbacks, not actual worldlight math/hardware'),'Finite cubes and points, valid snapshot lists0..4 and controlled record table; full original renderer/instance allocation/lifetime not verified','Prior query payloads were supplied by controlled query callbacks, not actual cache storage','Native-width semantic production data; original32bit pointers are test-only mapped identities','No editor integration/build, apps, dependencies or original writes'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ['status','smoke','cases','differences','coverage','safety']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
