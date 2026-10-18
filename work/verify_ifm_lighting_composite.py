"""Original origin draw fragment + actual bounds and DAG positions vs owned x64."""
import json
from pathlib import Path
import random
import struct
import subprocess
import sys
import tempfile
from verify_session_illumination_dag import IFM,IFM_SHA,SESSION,SESSION_SHA
from verify_dag_world_position import sha,require,NULL
from verify_ifm_bounds_frames import static_check as frame_static
from verify_ifm_bounds_dispatch import static_check as dispatch_static
from verify_ifm_animated_bounds import static_check as animated_static
from verify_ifm_lighting_origin import static_check as origin_static

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/ifm-lighting-composite-20261015'
BASE=ROOT/'outputs/ifm-bounds-composite-20261015/verification-r9b3o81r'
NON_DAG=NULL-1


def parse_base(raw):
    offset=0;records=[]
    while offset<len(raw):
        require(offset+240<=len(raw),'Base fixture header');h=struct.unpack_from('<11I',raw,offset);length=240+h[6]*44+h[7]*24+h[8]*48+h[9]*28;require(offset+length<=len(raw),'Base fixture extent');records.append(raw[offset:offset+length]);offset+=length
    require(len(records)==28932,'Pinned base fixture count');return records


def main():
    require(sha(IFM)==IFM_SHA and sha(SESSION)==SESSION_SHA,'Pinned originals changed');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));smoke='--smoke' in sys.argv
    prior=json.loads((BASE/'results.json').read_text());require(prior['status']=='PASS' and sha(BASE/'queries.bin')==prior['query_sha256'],'Base query provenance');base=parse_base((BASE/'queries.bin').read_bytes())
    evidence=frame_static()+dispatch_static()+animated_static()+origin_static();executables=[]
    for name,machine in [('lighting_native_x86',0x14c),('lighting_owned_x64',0x8664),('lighting_safety_x64',0x8664)]:
        path=OUT/(name+'.exe');raw=path.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine,'Executable architecture');require(b'asInvoker' in raw,'Non-elevated manifest');executables.append(dict(path=str(path),sha256=sha(path),machine=hex(machine)))
    def execute(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],capture_output=True,text=True,timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' exit='+str(p.returncode)+' '+p.stdout+p.stderr);return p.stdout
    rows=[];metadata=[];rng=random.Random(2026101537)
    def modify(raw,flags_or=0,render=None,root=None):
        raw=bytearray(raw);flags=struct.unpack_from('<I',raw)[0];struct.pack_into('<I',raw,0,flags|flags_or)
        if render is not None:struct.pack_into('<I',raw,16,render)
        if root is not None:struct.pack_into('<I',raw,20,root)
        return bytes(raw)
    def append(raw,present=0,explicit=NULL,prefix=0,header2=1,index=0,label='controlled',expected=None):
        h=struct.unpack_from('<11I',raw);require(present or h[0]&(16|32),'Native missing-header input excluded');require(present or prefix==0 or h[3]==0,'Native null-child bounds fault excluded')
        rows.append(raw[:44]+struct.pack('<4Ii',present,explicit,prefix,header2,index)+raw[44:]);nodes=[struct.unpack_from('<III8f',raw,240+44*i) for i in range(h[6])];children=[i for i,n in enumerate(nodes) if n[0]==h[5]]
        dag=explicit if present and explicit<h[6] else children[0] if present and prefix==0 and children else NULL
        effective_index=index if header2 else 0;bounds=not present and effective_index<=0 and bool(h[0]&8 or h[4]&0x20)
        metadata.append(dict(label=label,present=present,explicit=explicit,prefix=prefix,header2=header2,index=index,flags=h[0],mode=struct.unpack_from('<i',raw,12)[0],render_flags=h[4],children=len(children),dag=dag,bounds=bool(bounds),expected=expected))
    append(base[0],1,1,label='analytical explicit world point',expected=[100,0,0])
    append(base[0],1,label='analytical child0 world point',expected=[3,0,0])
    append(base[0],1,NON_DAG,label='analytical non-DAG explicit falls back child0',expected=[3,0,0])
    append(base[0],1,prefix=1,label='analytical null child0 ignores later valid child',expected=[])
    append(base[0],1,prefix=2,label='analytical non-DAG child0 ignores later valid child',expected=[])
    append(base[0],1,1,prefix=1,label='analytical explicit precedes null child0',expected=[100,0,0])
    append(modify(base[0],16),label='analytical disabled bounds',expected=[])
    append(base[2],index=1,label='analytical positive attachment bypass',expected=[])
    append(base[2],header2=0,index=100,label='analytical no header2 ignores index',expected=[100,0,0])
    append(base[3],label='analytical header load precedes animated query',expected=[0,0,0])
    append(base[9],label='analytical direct first bone union midpoint',expected=[5,0,0])
    append(modify(base[10],16|8),label='analytical child override and outer bounds midpoint',expected=[151,8,10])
    append(modify(base[8],16,render=0x20),label='analytical default bounds plus outer midpoint',expected=[100,0,36])
    append(base[2],index=-2147483648,label='analytical negative attachment permits bounds',expected=[100,0,0])
    append(base[2],index=2147483647,label='analytical maximum positive bypass',expected=[])
    append(base[0],1,0,index=2147483647,label='analytical explicit root ignores header',expected=[0,0,0])
    append(modify(base[0],root=3),1,label='analytical present null with no children',expected=[])
    append(base[10],1,2,label='analytical split-override recursive world position',expected=[112,16,20])
    if not smoke:
        for flags in range(2048):
            raw=base[12+flags*12+6+flags%3];available=raw if flags&(16|32) else modify(raw,16)
            append(available,index=0,label='all bounds flags')
            append(available,index=1,label='all flags positive attachment bypass')
            append(raw,1,flags%4,index=1,label='all flags explicit DAG')
            append(raw,1,NULL,prefix=flags%3,label='all flags child0/null selection')
        for i,raw in enumerate(base[12+24576:12+24576+4096]):
            h=struct.unpack_from('<11I',raw);available=raw if h[0]&(16|32) else modify(raw,16)
            append(available,header2=rng.randrange(2),index=rng.choice([-2147483648,-1,0,1,257,2147483647]),label='varied finite bounds and header')
            append(raw,1,rng.choice([NULL,NON_DAG,rng.randrange(h[6])]),prefix=rng.randrange(3),label='varied hierarchy explicit/fallback')
            append(raw,1,NULL,label='varied hierarchy child0')
        start=12+24576+4096
        for raw in base[start:start+176]:
            h=struct.unpack_from('<11I',raw)
            for index in [-1,0,1]:append(raw,index=index,label='actual MDL metadata with controlled poses')
            append(raw,1,h[5],label='MDL fixture explicit world');append(raw,1,NULL,label='MDL fixture child0 world');append(raw,1,NULL,prefix=1,label='MDL fixture first null child')
        for raw in base[start+176:]:
            h=struct.unpack_from('<11I',raw)
            append(raw,1,h[5],label='owned-evaluated benchmark explicit root');append(raw,1,NULL,label='owned-evaluated benchmark child0');append(raw,label='owned-evaluated benchmark bounds with controlled palette');append(raw,index=1,label='owned-evaluated benchmark attachment bypass')
    query=run/'queries.bin';query.write_bytes(b''.join(rows));outputs=[];logs=[]
    for name in ['lighting_native_x86','lighting_owned_x64']:
        target=run/(name+'.bin');logs.append(execute(name,[IFM,query,target],name));outputs.append(target.read_bytes());require(len(outputs[-1])==len(rows)*136,'Lighting output length')
    different=[]
    for i in range(len(rows)):
        for repeat in range(2):
            at=i*136+repeat*68
            if outputs[0][at:at+68]!=outputs[1][at:at+68]:different.append((i,repeat))
    details=[dict(case=i,repeat=repeat,metadata=metadata[i],native=struct.unpack_from('<17I',outputs[0],i*136+repeat*68),owned=struct.unpack_from('<17I',outputs[1],i*136+repeat*68)) for i,repeat in different[:6]]
    (run/'comparison.json').write_text(json.dumps(dict(differences=len(different),details=details),indent=2)+'\n')
    coverage=dict(explicit_dag=0,child0_dag=0,present_no_dag=0,attachment_bypass=0,disabled_bounds=0,bounds=0,animated_bounds=0,studio_bounds=0,engine_bounds=0,default_bounds=0,header_loads=0,header_reuse=0,first_bone_queries=0)
    for i,m in enumerate(metadata):
        require(outputs[0][i*136:i*136+16]==outputs[0][i*136+68:i*136+84],'Repeated query changed stable point')
        for repeat in range(2):
            at=i*136+repeat*68;has,*point=struct.unpack_from('<I3f',outputs[0],at);dag,world,bounds,header=struct.unpack_from('<4I',outputs[0],at+16);events=struct.unpack_from('<9I',outputs[0],at+32)
            require((dag,world,bounds,header)==(m['dag'],int(m['dag']!=NULL),int(m['bounds']),int(not m['present'])),'Independent provider selection case '+str(i));require(bool(has)==bool(m['dag']!=NULL or m['bounds']),'Independent point presence')
            if m['expected'] is not None:require((point if has else [])==m['expected'],'Independent analytical point '+str(i))
            if m['present']:coverage['explicit_dag' if m['explicit']!=NULL and m['explicit']!=NON_DAG else 'child0_dag' if dag!=NULL else 'present_no_dag']+=1
            elif m['header2'] and m['index']>0:coverage['attachment_bypass']+=1
            elif not bounds:coverage['disabled_bounds']+=1
            if bounds:
                coverage['bounds']+=1;coverage['animated_bounds' if events[3] else 'engine_bounds' if events[2] else 'studio_bounds' if events[0] else 'default_bounds']+=1
            coverage['header_loads']+=events[1];coverage['first_bone_queries']+=events[8]
        first=struct.unpack_from('<9I',outputs[0],i*136+32);second=struct.unpack_from('<9I',outputs[0],i*136+100);coverage['header_reuse']+=bool(first[1] and not second[1])
    require(all(coverage.values()),'Missing lighting composition coverage');safety=execute('lighting_safety_x64',[],'safety')
    # Reject invalid test snapshots before invoking the original fragment; never
    # probe the original's known null-header/null-child dereference with bad data.
    def wire(raw,present=1,explicit=NULL,prefix=0,header2=1,index=0):return raw[:44]+struct.pack('<4Ii',present,explicit,prefix,header2,index)+raw[44:]
    invalid=[('missing_header',wire(base[0],present=0)),('explicit_span',wire(base[0],explicit=1000)),
             ('prefix_enum',wire(base[0],prefix=3)),('header2_enum',wire(base[0],header2=2)),
             ('null_child_bounds',wire(base[2],present=0,prefix=1)),('presence_enum',wire(base[0],present=2)),
             ('truncated_origin',base[0][:44]+b'\0\0')]
    guards=[]
    for label,data in invalid:
        path=run/('invalid-'+label+'.bin');path.write_bytes(data)
        for name in ['lighting_native_x86','lighting_owned_x64']:
            p=subprocess.run([str(OUT/(name+'.exe')),str(IFM),str(path),str(run/('invalid-'+label+'-'+name+'.bin'))],capture_output=True,text=True,timeout=20)
            require(p.returncode==1,'Invalid fixture accepted or native fault: '+label+' '+name+' '+p.stderr);guards.append(dict(test=label,executable=name,returncode=p.returncode,message=p.stderr.strip()))
    require(sha(IFM)==IFM_SHA and sha(SESSION)==SESSION_SHA and all(sha(Path(m['path']))==m['sha256'] for m in prior['models']),'Original modified')
    receipt=dict(status='PASS' if not different else 'FAIL',smoke=smoke,cases=len(rows),comparisons=len(rows)*2,bytes_per_comparison=68,differences=len(different),details=details,coverage=coverage,executables=executables,logs=logs,safety=safety.strip(),guards=guards,ifm_sha256=IFM_SHA,session_sha256=SESSION_SHA,models=prior['models'],base_query_sha256=prior['query_sha256'],query_sha256=sha(query),static_range_byte_sum=sum(e.get('bytes',0) for e in evidence),static_evidence=evidence,
        limits=['Original10243967..10243A56 draw fragment executes with actual104C5900 recursive DAG world and actual10240520 complete bounds composition',
                'Draw world/header callsites and bounds vtable route through counting bridges which call actual original routines, never point/bounds answer substitutes; next block replaced by test RET',
                'Upstream draw eligibility, live model synchronization/asset binding, type declarations, attribute/datamodel enumeration, clock, bone palette/model-world and header acquisition remain controlled',
                'All normalized nodes declared DAGs; explicit/prefixed non-DAG uses separate controlled DmElement; unique supplied ordinary parent relation only',
                'Absent attribute requires an acquirable studio header; original null-header dereference is not executed or treated as attachment index zero',
                'Actual metadata and prior benchmark DAG snapshots retained, but original session animation/bone evaluation is not executed',
                'Only specified header-cache initialization allowed; fixture input buffers/output canaries/sealed private code unchanged',
                'No original write, application launch, dependency install, editor rebuild or renderer integration'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ['status','smoke','cases','comparisons','differences','details','coverage','safety']}));print('receipt='+str(run/'results.json'));require(not different,'Lighting composition differs')


if __name__=='__main__':main()
