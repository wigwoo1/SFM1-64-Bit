"""Original cached/point acquisition routing with controlled query services."""
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
OUT=ROOT/'outputs/model-cache-query-20261015'
RANGES=[('cached_query_route',0x100d5845,0x100d58c0),('point_query_route',0x100d5a83,0x100d5b95),
        ('original_lighting_state_copy',0x100d16a0,0x100d1734),('ASCII_substring_search',0x102505c0,0x10250673),
        ('substring_search_wrapper',0x10250680,0x10250689)]
KEEP=0xffffffff


def main():
    require(sha(ENGINE)==DIGEST and sha(IFM)==IFM_SHA and sha(SESSION)==SESSION_SHA,'Pinned originals changed');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));smoke='--smoke' in sys.argv
    image,evidence=check_ranges(ENGINE,'engine-disassembly.txt',RANGES);require(image.string(0x1031c308)=='' and image.string(0x1031c30c)=='FCVAR_NEVER_AS_STRING','Filter literals changed');executables=[]
    for name,machine in [('query_native_x86',0x14c),('query_owned_x64',0x8664),('query_safety_x64',0x8664)]:
        path=OUT/(name+'.exe');raw=path.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine,'Executable architecture');require(b'asInvoker' in raw,'Non-elevated manifest');executables.append(dict(path=str(path),sha256=sha(path),machine=hex(machine)))
    def execute(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],capture_output=True,text=True,timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' exit='+str(p.returncode)+' '+p.stdout+p.stderr);return p.stdout
    rng=random.Random(2026101551);rows=[];expected=[];metadata=[]
    def payload():return struct.pack('<18f5I',*[rng.uniform(-1,4) for _ in range(18)],rng.randrange(5),*[rng.getrandbits(32) for _ in range(4)])
    def fold(s):return s.split(b'\0',1)[0].translate(bytes.maketrans(b'ABCDEFGHIJKLMNOPQRSTUVWXYZ',b'abcdefghijklmnopqrstuvwxyz'))
    def append(flags=0,mode=0,handle=0x123,renderable=0x456,post1=KEEP,post2=KEEP,name=b'prefix_PYRO_suffix',filter=b'pyro',point=(11,22,33),label='controlled',states=None):
        states=states or (payload(),payload());row=struct.pack('<8I3f',flags,mode&KEEP,handle,renderable,post1,post2,len(name),len(filter),*point)+b''.join(states)+name+filter;rows.append(row);control=bool(flags&2);valid_calls=filter_calls=0;events=[]
        def event(kind,query_flags,special=False):
            nonlocal control
            ordinal=len(events)+1;words=[kind,query_flags,int(special),handle if kind==1 else renderable,0,0,0,0x90000000+ordinal]
            if kind==2:words[4:7]=struct.unpack('<3I',struct.pack('<3f',*point))
            events.append(words);setting=[post1,post2][ordinal-1]
            if setting!=KEEP:control=bool(setting)
        if flags&1:
            if control:
                valid_calls=1;event(1,7 if flags&4 else 6)
                if not control:event(1,7)
            else:event(1,7)
        else:
            if control:event(2,6)
            if not control:
                special=False
                if mode==5 and flags&8 and flags&16:
                    filter_calls=1;needle=b'FCVAR_NEVER_AS_STRING' if flags&32 else filter if flags&64 else b'';hay=fold(name);needle=fold(needle);special=bool(hay and needle and needle in hay)
                event(2,15,special)
        count=len(events);tail=struct.pack('<4I',valid_calls,filter_calls,int(control),count)+b''.join(struct.pack('<8I',*e) for e in events+[([0]*8)]*(2-count))
        scratch=(101.25,-202.5,303.75) if flags&1 else point;value=0x90000000+count;aux=0xa4a4a4a4 if flags&1 else 0xa0000000+count
        tail+=struct.pack('<III3f',value,aux,int(not(flags&1)),*scratch)+states[count-1];require(len(tail)==196,'Expected query output extent');expected.append(tail);metadata.append(dict(flags=flags,mode=mode,post1=post1,post2=post2,name=name.hex(),filter=filter.hex(),event_flags=[e[1] for e in events],special=bool(events[-1][2]),label=label))
    append(label='analytical point15')
    append(flags=2,label='analytical point6')
    append(flags=2|8|16|64,mode=5,post1=0,label='analytical point6 then15 substring match')
    append(flags=1,label='analytical cached7 skips validity')
    append(flags=1|2,label='analytical cached6 after invalid handle')
    append(flags=1|2|4,label='analytical cached7 after valid handle')
    append(flags=1|2,post1=0,label='analytical cached6 then7')
    append(flags=8|16|64,mode=5,name=b'xxFCVAR_NEVER_AS_STRINGyy',filter=b'no',label='analytical custom filter mismatch')
    append(flags=8|16|32|64,mode=5,name=b'xxFCVAR_NEVER_AS_STRINGyy',filter=b'no',label='analytical never-as-string override matches')
    append(flags=8|16,mode=5,filter=b'pyro',label='analytical null string pointer uses empty, not match')
    append(flags=8|16|64,mode=5,name=b'PyRo',filter=b'',label='analytical empty needle returns no match')
    append(flags=8|16|64,mode=5,name=b'xxPyRo\0zzz',filter=b'zzz',label='analytical NUL terminates name')
    append(flags=0,post1=1,label='analytical initially disabled still only point15')
    append(flags=1,handle=0,point=(float('nan'),22,33),label='analytical cached zero handle bypasses nonfinite point')
    prior=None;prior_evidence=None
    if not smoke:
        patterns=[(b'prefix_PYRO_suffix',b'pyro'),(b'prefix_PYRO_suffix',b'medic'),(b'xxFCVAR_NEVER_AS_STRINGyy',b''),(b'',b'')]
        for flags,mode,post1,post2,strings in itertools.product(range(128),[-1,0,4,5,6],[KEEP,0,1],[KEEP,0,1],patterns):append(flags=flags,mode=mode,post1=post1,post2=post2,name=strings[0],filter=strings[1],handle=[0,1,KEEP][flags%3],renderable=0x7000+flags,label='exhaustive routing inputs')
        for i in range(8192):
            name=bytes(rng.randrange(256) for _ in range(rng.randrange(48)));filter=name[rng.randrange(len(name)+1):][:rng.randrange(16)] if i%2 else bytes(rng.randrange(256) for _ in range(rng.randrange(16)))
            append(flags=rng.randrange(128),mode=rng.choice([-2147483648,-1,0,5,2147483647]),handle=rng.getrandbits(32),renderable=rng.getrandbits(32),post1=rng.choice([KEEP,0,1]),post2=rng.choice([KEEP,0,1]),name=name,filter=filter,point=[rng.uniform(-32768,32768) for _ in range(3)],label='finite point and byte-string inputs')
        previous=ROOT/'outputs/ifm-draw-packet-20261015/verification-uodhatds';prior=json.loads((previous/'results.json').read_text());delivered=previous/'consume-packet_engine_x86.bin';raw=delivered.read_bytes();require(prior['status']=='PASS' and len(raw)==prior['cases']*48,'Prior engine receipt/extent')
        prior_evidence=dict(receipt=str(previous/'results.json'),receipt_sha256=sha(previous/'results.json'),delivered_points=str(delivered),delivered_points_sha256=sha(delivered),cases=prior['cases'])
        # Original delivered points, conditional on the previous controlled pose
        # and attachment inputs; query services remain controlled in this stage.
        for i in range(prior['cases']):
            point=struct.unpack_from('<3f',raw,i*48+24)
            append(flags=(i*13)%128,mode=5 if i%3 else 0,post1=0 if i%4==0 else KEEP,point=point,label='previous original material-delivered lighting point')
            append(flags=8|16|64,mode=5,point=point,name=b'model/pyro',filter=b'PYRO',label='previous delivered point filtered full query')
    query=run/'queries.bin';query.write_bytes(b''.join(rows));outputs=[];logs=[]
    for name in ['query_native_x86','query_owned_x64']:
        target=run/(name+'.bin');logs.append(execute(name,[ENGINE,query,target],name));outputs.append(target.read_bytes());require(len(outputs[-1])==196*len(rows),'Query output extent')
    wanted=b''.join(expected);different=[i for i in range(len(rows)) if outputs[0][i*196:(i+1)*196]!=outputs[1][i*196:(i+1)*196] or outputs[0][i*196:(i+1)*196]!=wanted[i*196:(i+1)*196]]
    details=[dict(case=i,metadata=metadata[i],native=struct.unpack_from('<49I',outputs[0],i*196),owned=struct.unpack_from('<49I',outputs[1],i*196),expected=struct.unpack_from('<49I',wanted,i*196)) for i in different[:4]]
    (run/'comparison.json').write_text(json.dumps(dict(differences=len(different),details=details),indent=2)+'\n')
    coverage=dict(cached=0,point=0,cache6=0,cache7=0,point6=0,point15=0,two_queries=0,filter_match=0,filter_no_match=0,validity_calls=0,control_changed=0)
    for i,m in enumerate(metadata):
        valid_calls,filter_calls,final,count=struct.unpack_from('<4I',outputs[0],i*196);coverage['cached' if m['flags']&1 else 'point']+=1;coverage['two_queries']+=count==2;coverage['validity_calls']+=valid_calls;coverage['control_changed']+=final!=bool(m['flags']&2)
        if filter_calls:coverage['filter_match' if m['special'] else 'filter_no_match']+=1
        for ordinal in range(count):
            kind,flags=struct.unpack_from('<2I',outputs[0],i*196+16+ordinal*32);coverage[('cache' if kind==1 else 'point')+str(flags)]+=1
    require(all(coverage.values()),'Missing cache query branch coverage');safety=execute('query_safety_x64',[],'safety')
    invalid=[];bad=bytearray(rows[0]);struct.pack_into('<I',bad,0,128);invalid.append(('unknown_flags',bytes(bad)));bad=bytearray(rows[0]);struct.pack_into('<I',bad,16,2);invalid.append(('post_control',bytes(bad)));bad=bytearray(rows[0]);struct.pack_into('<f',bad,32,float('inf'));invalid.append(('nonfinite_selected_point',bytes(bad)));invalid.append(('truncated',rows[0][:37]));guards=[]
    for label,data in invalid:
        path=run/('invalid-'+label+'.bin');path.write_bytes(data)
        for name in ['query_native_x86','query_owned_x64']:
            p=subprocess.run([str(OUT/(name+'.exe')),str(ENGINE),str(path),str(run/('invalid-'+label+'-'+name+'.bin'))],capture_output=True,text=True,timeout=20);require(p.returncode==1,'Invalid fixture accepted/faulted '+label+' '+name);guards.append(dict(test=label,executable=name,message=p.stderr.strip()))
    require(sha(ENGINE)==DIGEST and sha(IFM)==IFM_SHA and sha(SESSION)==SESSION_SHA,'Original modified')
    sources={name:sha(ROOT/'work'/name) for name in ['source_model_cache_query_candidate.h','probe_source_model_cache_query.cpp','test_source_model_cache_query.cpp','build_source_model_cache_query.cmd','verify_source_model_cache_query.py']}
    receipt=dict(status='PASS' if not different else 'FAIL',smoke=smoke,cases=len(rows),output_bytes=196,differences=len(different),details=details,coverage=coverage,executables=executables,logs=logs,safety=safety.strip(),guards=guards,engine_sha256=DIGEST,ifm_sha256=IFM_SHA,session_sha256=SESSION_SHA,query_sha256=sha(query),prior_evidence=prior_evidence,source_sha256=sources,static_bytes=sum(e['bytes'] for e in evidence),static_evidence=evidence,
        limits=['Original100D5845..58BF cached acquisition route and100D5A83..5B94 point acquisition route execute, with original92byte lighting-state copy and ASCII substring search',
                'Test returns stop before cached-instance blend100D58C0 and downstream100D5B95; setup/eligibility, instance data lookup, blending, shader/cache lifecycle not executed',
                'Handle validity10193E30, cached query10196A40 and point query10197750 are controlled callbacks; actual query storage, allocation, BSP/newrecord lookup and lifecycle not executed',
                'Callbacks supply two finite lighting payloads and may change the shared control byte; original decisions must observe the updated byte',
                'Original substring callsite routes through a counter bridge which calls actual10250680/102505C0, never a substituted match result',
                'Mode5 filter flags/value, instance record/name and cache-handle availability are controlled snapshots; real ConVar names/lifecycle still unverified',
                'Owned production routing is payload-generic and native-width; original23word32bit payload exists only in the test fixture',
                'Prior material points come from original conditional stages with supplied poses, not live DMX evaluation/render parity',
                'No editor integration/build, original write, app launch or dependency install'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ['status','smoke','cases','differences','details','coverage','safety']}));print('receipt='+str(run/'results.json'));require(not different,'Model cache query routing differs')


if __name__=='__main__':main()
