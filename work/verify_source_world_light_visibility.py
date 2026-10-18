"""Full original distance falloff and direction/visibility query with trace service."""
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
from verify_source_world_light_overflow import MAP,MAP_SHA
from verify_dag_world_position import sha,require
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/world-light-visibility-20261015'
COMPOSITE='--complete-overflow' in sys.argv
NAMES=['complete_native_x86','complete_owned_x64'] if COMPOSITE else ['visibility_native_x86','visibility_owned_x64']
RANGES=[('distance_falloff_after_three_byte_prologue',0x100d1243,0x100d13f9),('visibility_query',0x10194420,0x101947a5),('normalization',0x10244060,0x102440cd),('point_ray_constructor',0x1000e100,0x1000e1ac),('world_only_should_hit',0x102727e0,0x102727e5),('world_only_trace_type',0x10127d20,0x10127d26),('static_prop_trace_type',0x10218050,0x10218056)]
if COMPOSITE:
    from verify_source_world_light_overflow import RANGES as OVERFLOW_RANGES,BASIS,dot
    RANGES+=OVERFLOW_RANGES
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def squares(v):return f32(f32(f32(v[1]*v[1])+f32(v[0]*v[0]))+f32(v[2]*v[2]))
def distance_reference(t,delta,radius,constant,linear,quadratic,ignore):
    if t not in (0,1,2,4):return 1.
    sq=squares(delta)
    if t==0:
        if radius!=0 and sq>f32(radius*radius):return 0.
        if sq<1:return 1.
        return f32(1./((delta[1]*delta[1]+f32(delta[0]*delta[0]))+delta[2]*delta[2]))
    dist=f32(math.sqrt(sq))
    if t==4:
        value=f32(linear-dist);return 0. if value<0 else value
    if not ignore and radius!=0 and dist>radius:return 0.
    return f32(1./((linear*dist+constant)+quadratic*sq))
def main():
    require(sha(ENGINE)==DIGEST and sha(IFM)==IFM_SHA and sha(SESSION)==SESSION_SHA and sha(MAP)==MAP_SHA,'Pinned original changed');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));smoke='--smoke' in sys.argv
    image,evidence=check_ranges(ENGINE,'engine-disassembly.txt',RANGES);o=image.offset(0x100d1240);require(image.b[o:o+3]==bytes.fromhex('538bdc'),'Distance prologue');require([image.word(0x100d13fc+4*i) for i in range(6)]==[0x100d126f,0x100d1375,0x100d1375,0x100d13f0,0x100d131c,0x100d13f0],'Distance dispatch table')
    require([image.word(a+4*i) for a in [0x1031dee4,0x1035bb48] for i in range(2)]==[0x102727e0,0x10127d20,0x101943e0,0x10218050],'Trace filter tables')
    constants={hex(a):image.word(a) for a in [0x1033f444,0x1031df38,0x10325a18,0x10322ae8,0x1031cb58]};require(list(constants.values())==[0x475eb852,0x41000000,0x3b783e10,0x34000000,0x40400000],'Visibility constants')
    executables=[]
    for name,machine in [(NAMES[0],0x14c),(NAMES[1],0x8664),('visibility_safety_x64',0x8664)]:
        path=OUT/(name+'.exe');raw=path.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'Architecture/manifest');executables.append(dict(path=str(path),sha256=sha(path),machine=hex(machine)))
    rng=random.Random(2026101554);rows=[];expected=[];meta=[]
    def append(t=1,flags=0,styleIndex=0,styleValue=264,debug=0,surface=0,excluded=0x1234,radius=0,constant=1,linear=0,quadratic=0,threshold=0,fraction=1,position=(0,0,20),intensity=(1,2,3),normal=(0,0,-1),point=(0,0,0),initial=(11,-22,33),end=(101,202,303),label='analytical'):
        if COMPOSITE and flags not in (0,3):return
        p=list(map(f32,[radius,constant,linear,quadratic,threshold,fraction]));radius,constant,linear,quadratic,threshold,fraction=p;v=[tuple(map(f32,x)) for x in [position,intensity,normal,point,initial,end]];position,intensity,normal,point,initial,end=v
        rows.append(struct.pack('<7I24f',t&0xffffffff,flags,styleIndex,styleValue&0xffffffff,debug&0xffffffff,surface,excluded,*p,*[x for a in v for x in a]))
        raw=tuple(f32(a-b) for a,b in zip(position,point));falloff=distance_reference(t,raw,radius,constant,linear,quadratic,bool(flags&2));incoming=list(initial);attenuation=0.;events=[0]*8;start=[0.]*3;delta=[0.]*3;debugStart=[0.]*3;debugEnd=[0.]*3;branch='skyambient';styleRead=False;cutoff=False
        def trace(d):
            nonlocal start,delta
            events[0]=1;events[2]=3 if flags&4 else 1;events[3]=excluded if flags&4 else 0;events[4]=0x4481;events[5]=1;events[6]=int(squares(d)!=0);start=list(point);delta=list(d)
        if t==3:
            branch='sky';incoming=[0.,0.,0.];skyDelta=[f32(f32(point[i]-f32(normal[i]*57016.3203125))-point[i]) for i in range(3)];trace(skyDelta)
            if surface&4:incoming=[-x for x in normal];attenuation=1.
        elif t!=5:
            incoming=list(raw);a=falloff
            if not flags&8:a=f32(f32(f32(styleValue)*0.0037878789007663727)*a);styleRead=True
            maximum=max(intensity);cutoff=t!=0 and threshold>f32(maximum*a)
            if cutoff:branch='intensity cutoff'
            else:
                length=f32(math.sqrt(squares(incoming)));epsilon=struct.unpack('<f',bytes.fromhex('00000034'))[0];inverse=f32(1./f32(length+epsilon));incoming=[f32(x*inverse) for x in incoming]
                if flags&1:attenuation=a;branch='skip trace'
                else:
                    trace(raw);obstruction=f32(f32(1.-fraction)*length)
                    if obstruction<=8:attenuation=a;branch='visible within eight'
                    else:
                        branch='occluded'
                        if debug==2:events[1]=1;events[7]=1;debugStart=list(point);debugEnd=list(end)
        answer=struct.pack('<5f8I12f',falloff,*incoming,attenuation,*events,*start,*delta,*debugStart,*debugEnd);basis_calls=0
        if COMPOSITE:
            d=dot(incoming,incoming);cosine=-dot(normal,incoming)
            if t<0 or t>5:angular=0.
            elif t==5:angular=1.
            elif t==3:angular=0. if cosine<0 else cosine
            elif t==0:
                surfaceDot=-((normal[1]*incoming[1]+normal[0]*incoming[0])+normal[2]*incoming[2]);angular=f32(surfaceDot*d) if surfaceDot>.01 else 0.
            elif t in (1,4):angular=d
            else:angular=0. if cosine<=0 else d if cosine>=1 else f32(cosine*d)
            scale=f32(angular*attenuation);ambient=[f32(i*.125-.5) for i in range(18)];basis_calls=int(scale!=0)
            if basis_calls:
                for i,direction in enumerate(BASIS):
                    value=dot(direction,incoming)
                    if value>0:
                        amount=f32(value*scale)
                        for c in range(3):ambient[i*3+c]=f32(f32(intensity[c]*amount)+ambient[i*3+c])
            answer+=struct.pack('<18f10I',*ambient,*events,basis_calls,basis_calls)
        expected.append(answer);meta.append(dict(type=t,flags=flags,radius=radius,styleRead=styleRead,branch=branch,trace=events[0],debug=events[1],filter=events[2],swept=events[6],attenuation=attenuation,basis_calls=basis_calls,label=label))
    append()
    append(t=2,flags=1,linear=.1,quadratic=.01,label='spot distance falloff')
    append(t=2,linear=.1,quadratic=.01,label='spot distance with trace')
    append(t=4,linear=30,label='quake distance with trace')
    append(t=3,surface=4,label='sky with trace')
    append(fraction=.6,label='eight-unit edge accepted')
    append(fraction=.5,debug=2,label='blocked debug uses supplied trace endpoint')
    append(flags=1,label='skip point trace')
    append(radius=5,label='radius cutoff still trace if intensity threshold zero')
    append(flags=3,radius=5,label='ignore point radius')
    append(t=0,radius=5,flags=3,label='surface radius never bypassed')
    append(t=0,position=(0,0,.5),label='near surface clamps distance falloff')
    append(t=4,linear=30,flags=1,label='quake uses linear field minus distance')
    append(t=4,linear=10,flags=1,label='quake clamps negative')
    append(flags=9,styleValue=0,label='skip style')
    append(styleValue=0,label='zero style still normalizes/traces at zero threshold')
    append(threshold=4,label='cutoff preserves raw delta')
    append(t=0,threshold=999,flags=1,label='surface bypasses intensity cutoff')
    append(t=3,flags=9,surface=4,label='sky ignores skip-trace flag')
    append(t=3,flags=4,surface=0,label='sky miss zero incoming')
    append(t=5,label='skyambient preserves incoming')
    append(t=-1,flags=1,label='unknown type distance defaults one')
    append(flags=4,excluded=0xffffffff,label='static prop filter context')
    append(flags=1,position=(0,0,0),label='stationary point')
    append(t=3,normal=(0,0,0),surface=4,label='stationary sky ray')
    append(flags=1,constant=-2,threshold=-10,label='signed reciprocal')
    append(flags=1,position=(1e-25,0,0),label='tiny vector square underflow')
    map_evidence=None
    if not smoke:
        for t,flags,styleValue,threshold,fraction in itertools.product([-1,0,1,2,3,4,5,6],range(16),[0,132,264],[-1,0,4],[0,.5,.6,1]):append(t=t,flags=flags,styleValue=styleValue,threshold=threshold,fraction=fraction,linear=30 if t==4 else .01,quadratic=0 if t==4 else .001,debug=2,surface=4 if flags&2 else 0,label='type/flag/style/cutoff/trace combinations')
        for i in range(12288):
            t=rng.choice([-1,0,1,2,3,4,5,6]);point=[rng.uniform(-32768,32768) for _ in range(3)];position=[f32(point[j]+rng.uniform(-256,256)) for j in range(3)]
            append(t=t,flags=rng.randrange(16),styleIndex=rng.randrange(256),styleValue=rng.choice([-264,0,1,132,264,528]),debug=rng.choice([-1,0,1,2,3]),surface=rng.randrange(256),excluded=rng.getrandbits(32),radius=rng.choice([-1,0,8,64,512]),constant=rng.uniform(.25,2),linear=rng.uniform(.01,2) if t!=4 else rng.uniform(0,512),quadratic=rng.uniform(0,.1),threshold=rng.choice([-10,0,.001,.1,10]),fraction=rng.choice([0,.5,1,rng.random()]),position=position,intensity=[rng.uniform(-2,64) for _ in range(3)],normal=[rng.uniform(-1,1) for _ in range(3)],point=point,label='finite controlled')
        bsp=MAP.read_bytes();require(bsp[:4]==b'VBSP' and struct.unpack_from('<I',bsp,4)[0]==20,'Pinned BSP20');offset,length,version,fourcc=struct.unpack_from('<4I',bsp,8+54*16);require(version==0 and fourcc==0 and length%88==0 and offset+length<=len(bsp),'Pinned version0 worldlights')
        prior=ROOT/'outputs/ifm-draw-packet-20261015/verification-uodhatds';priorReceipt=json.loads((prior/'results.json').read_text());require(priorReceipt['status']=='PASS','Prior point receipt');points=(prior/'consume-packet_engine_x86.bin').read_bytes();require(len(points)==48*priorReceipt['cases'],'Prior point extent');types={}
        for i in range(length//88):
            o=offset+i*88;position=struct.unpack_from('<3f',bsp,o);intensity=struct.unpack_from('<3f',bsp,o+12);normal=struct.unpack_from('<3f',bsp,o+24);t,style=struct.unpack_from('<2I',bsp,o+40);radius,constant,linear,quadratic=struct.unpack_from('<4f',bsp,o+60);require(t<=5 and style<256,'Disk light type/style');types[str(t)]=types.get(str(t),0)+1
            for j in range(64):
                point=struct.unpack_from('<3f',points,((i*64+j)%priorReceipt['cases'])*48+24)
                append(t=t,flags=j%16,styleIndex=style,styleValue=[132,264,528][j%3],debug=2,surface=4 if j%2 else 0,radius=radius,constant=constant,linear=linear,quadratic=quadratic,position=position,intensity=intensity,normal=normal,point=point,fraction=[0,.5,1][j%3],label='actual BSP light and prior delivered point, controlled trace/style')
        map_evidence=dict(path=str(MAP),sha256=MAP_SHA,types=types,lights=length//88,prior_receipt=str(prior/'results.json'),prior_receipt_sha256=sha(prior/'results.json'),prior_points_sha256=sha(prior/'consume-packet_engine_x86.bin'))
    query=run/'queries.bin';query.write_bytes(b''.join(rows));outputs=[];logs=[]
    def execute(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],capture_output=True,text=True,timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' exit '+str(p.returncode)+' '+p.stderr);return p.stdout.strip()
    extent=212 if COMPOSITE else 100
    for name in NAMES:
        target=run/(name+'.bin');logs.append(execute(name,[ENGINE,query,target],name));outputs.append(target.read_bytes());require(len(outputs[-1])==extent*len(rows),'Output extent')
    wanted=b''.join(expected);different=[i for i in range(len(rows)) if outputs[0][i*extent:(i+1)*extent]!=outputs[1][i*extent:(i+1)*extent] or outputs[0][i*extent:(i+1)*extent]!=wanted[i*extent:(i+1)*extent]];details=[dict(case=i,inputs=meta[i],native=struct.unpack_from('<'+str(extent//4)+'I',outputs[0],i*extent),owned=struct.unpack_from('<'+str(extent//4)+'I',outputs[1],i*extent),expected=struct.unpack_from('<'+str(extent//4)+'I',wanted,i*extent)) for i in different[:4]];(run/'comparison.json').write_text(json.dumps(dict(differences=len(different),details=details),indent=2)+'\n');require(not different,'Visibility comparison differs: '+str(run/'comparison.json'))
    safety=execute('visibility_safety_x64',[],'safety');guards=[];malformed=[]
    for label,offset,value,fmt in [('flags',4,16,'I'),('style_index',8,256,'I'),('surface_flags',20,65536,'I'),('fraction',48,1.1,'f'),('nonfinite_radius',28,float('nan'),'f'),('nonfinite_point',88,float('inf'),'f')]:
        bad=bytearray(rows[0]);struct.pack_into('<'+fmt,bad,offset,value);malformed.append((label,bytes(bad)))
    malformed.append(('truncated',rows[0][:-1]));bad=bytearray(rows[0]);struct.pack_into('<f',bad,32,0);malformed.append(('zero_denominator',bytes(bad)))
    for label,data in malformed:
        path=run/('invalid-'+label+'.bin');path.write_bytes(data)
        for name in NAMES:
            p=subprocess.run([str(OUT/(name+'.exe')),str(ENGINE),str(path),str(run/('invalid-'+label+'-'+name+'.bin'))],capture_output=True,text=True,timeout=20);require(p.returncode==1,'Invalid fixture accepted/faulted '+label+' '+name);guards.append(dict(test=label,executable=name,message=p.stderr.strip()))
    coverage=dict(branches={k:sum(m['branch']==k for m in meta) for k in sorted({m['branch'] for m in meta})},traces=sum(m['trace'] for m in meta),debug=sum(m['debug'] for m in meta),world_only=sum(m['filter']==1 for m in meta),static_props=sum(m['filter']==3 for m in meta),stationary=sum(m['trace'] and not m['swept'] for m in meta),style_reads=sum(m['styleRead'] for m in meta),basis_calls=sum(m['basis_calls'] for m in meta),types={str(t):sum(m['type']==t for m in meta) for t in range(6)});require(all(coverage[k] for k in ['traces','debug','world_only','style_reads']+([] if COMPOSITE else ['static_props','stationary'])) and all(coverage['types'].values()),'Coverage gap')
    require(sha(ENGINE)==DIGEST and sha(IFM)==IFM_SHA and sha(SESSION)==SESSION_SHA and sha(MAP)==MAP_SHA,'Original modified');sources={n:sha(ROOT/'work'/n) for n in ['source_world_light_visibility_candidate.h','probe_source_world_light_visibility.cpp','test_source_world_light_visibility.cpp','build_source_world_light_visibility.cmd','verify_source_world_light_visibility.py']}
    receipt=dict(status='PASS',smoke=smoke,complete_overflow=COMPOSITE,cases=len(rows),bytes_per_result=extent,differences=0,coverage=coverage,executables=executables,logs=logs,safety=safety,guards=guards,query_sha256=sha(query),source_sha256=sources,map_evidence=map_evidence,engine_sha256=DIGEST,ifm_sha256=IFM_SHA,session_sha256=SESSION_SHA,map_sha256=MAP_SHA,static_bytes=sum(e['bytes'] for e in evidence)+3,static_evidence=evidence,constants=constants,limits=['Full original100D1240 falloff and10194420 visibility query/10244060 normalization/1000E100 Ray_t constructor execute',('Complete original10195510 overflow ->actual query/angular/cube executes additionally; angular cone parameters controlled defaults1/0/1, six supplied axis directions; query caller flags0/3 only' if COMPOSITE else 'Overflow composition not executed in this mode'),'Trace virtual supplies fraction/surface flags/end position; actual collision/static prop filtering/world binding not executed','Original trace-type vtable methods execute; custom ShouldHitEntity predicate/static prop manager not executed','Style array, intensity/debug ConVars and excluded-entity identity are controlled snapshots','Owned semantic data/trace inputs use native-width identities, no original32bit ABI required','Finite-result domain, valid trace fractions0..1; zero denominator rejected rather than native infinity','No editor integration/build, application launch, dependencies or original writes'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ['status','smoke','cases','differences','coverage','safety']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
