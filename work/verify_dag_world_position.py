"""Original IFM recursion vs owned x64 graph, on controlled and evaluated snapshots.

Attribute lookup, datamodel enumeration, type declarations and input evaluation
are controlled. This is not live SFM/session-loader equivalence.
"""
import hashlib
import json
from pathlib import Path
import random
import re
import struct
import subprocess
import tempfile
from pe_image import Image
from verify_session_illumination_dag import IFM, IFM_SHA, SESSION, SESSION_SHA

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/dag-world-position-20261015'
NULL=0xffffffff
RANGES=[('position_parent',0x104c3ed0,0x104c3f55),('rotation_parent',0x104c3f60,0x104c3fe5),
        ('recursive_world_composition',0x104c5750,0x104c5863),('DAG_world_matrix',0x104c5870,0x104c58d3),
        ('DAG_world_position',0x104c5900,0x104c5975),('local_transform',0x104d2860,0x104d2938),
        ('quaternion_position_matrix',0x1049dfc0,0x1049e155),('identity',0x104990b0,0x104990dd),
        ('extract_column',0x104986e0,0x10498701),('write_column',0x10498710,0x10498731),
        ('reverse_parent_enumeration',0x10223170,0x102231f9),('reverse_parent_filter',0x10222de0,0x10222e76),
        ('typed_override_parent',0x101e3170,0x101e31f5),('element_handle',0x10335830,0x10335834)]


def require(value,message):
    if not value:raise RuntimeError(message)


def sha(path):
    with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()


def node(parent=NULL,override=NULL,flags=32,p=(0,0,0),q=(0,0,0,1),scale=1):
    return struct.pack('<III8f',parent,override,flags,*p,*q,scale)


def parse(raw):
    cases=[];offset=0
    while offset<len(raw):
        require(offset+4<=len(raw),'fixture header');count=struct.unpack_from('<I',raw,offset)[0];offset+=4
        require(0<count<=64 and offset+44*count<=len(raw),'fixture node extent')
        cases.append([struct.unpack_from('<III8f',raw,offset+44*i) for i in range(count)]);offset+=44*count
    return cases


def static_check():
    image=Image(IFM);rows=[];pending=None
    with (ROOT/'work/ifm-disassembly.txt').open() as f:
        for line in f:
            m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
            if m:
                pending=None;address=int(m[1],16)
                if any(a<=address<b for _,a,b in RANGES):
                    pending=dict(address=address,bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());rows.append(pending)
            elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line).hex()
    evidence=[]
    for name,start,end in RANGES:
        cursor=start;selected=[r for r in rows if start<=r['address']<end]
        for row in selected:
            require(row['address']==cursor,'Disassembly gap: '+name);raw=bytes.fromhex(row['bytes']);o=image.offset(cursor)
            require(image.b[o:o+len(raw)]==raw,'Stale static bytes: '+name);cursor+=len(raw)
        require(cursor==end,'Incomplete static range: '+name);evidence.append(dict(name=name,start=hex(start),end_exclusive=hex(end),bytes=end-start,instructions=selected))
    require([image.word(0x10222e78+i*4) for i in range(4)]==[0x10222e33,0x10222e26,0x10222e19,0x10222e01],'Filter table')
    return evidence


def main():
    require(sha(IFM)==IFM_SHA and sha(SESSION)==SESSION_SHA,'Pinned originals changed')
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));evidence=static_check();executables=[]
    for name,machine in [('dag_world_native_x86',0x14c),('dag_world_owned_x64',0x8664),('dag_world_export_x64',0x8664)]:
        path=OUT/(name+'.exe');b=path.read_bytes();pe=struct.unpack_from('<I',b,60)[0];require(b[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',b,pe+4)[0]==machine,'Executable architecture');executables.append(dict(path=str(path),sha256=sha(path),machine=hex(machine)))
    def execute(name,args,label):
        result=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],capture_output=True,text=True,timeout=90)
        (run/(label+'.log')).write_text(result.stdout+result.stderr);require(result.returncode==0,label+' exit='+str(result.returncode)+' '+result.stdout+result.stderr);return result.stdout
    benchmark=run/'benchmark.bin';export_log=execute('dag_world_export_x64',[SESSION,benchmark],'export')
    snapshots=parse(benchmark.read_bytes());require(len(snapshots)==6,'Expected six evaluated snapshots')
    cases=[];labels=[]
    def append(nodes,label):cases.append(struct.pack('<I',len(nodes))+b''.join(nodes));labels.append(label)
    # Independent exact expectations, not derived from either transform engine.
    append([node(flags=96,p=(10,20,30),scale=2),node(parent=0,flags=96,p=(3,4,5),scale=4)],'analytical scaled parent')
    append([node(flags=96,p=(10,0,0),scale=2),node(flags=96,p=(0,20,0),scale=4),node(parent=0,override=1,flags=120,p=(3,5,7),scale=3)],'analytical split frames')
    append([node(flags=0),node(parent=0,p=(3,5,7))],'analytical null transform')
    rng=random.Random(2026101506)
    for case in range(4096):
        nodes=[]
        for i in range(1+case%16):
            parent=rng.randrange(i) if i and rng.random()<.8 else NULL
            override=rng.randrange(i) if i and rng.random()<.7 else NULL
            flags=(case*7+i*19)%128
            p=[rng.uniform(-1024,1024) for _ in range(3)];q=[rng.uniform(-1.2,1.2) for _ in range(4)]
            scale=[0,1,-1,2,-2,.125,.5,1.0000001192092896][(case+i)%8]
            nodes.append(node(parent,override,flags,p,q,scale))
        append(nodes,'controlled')
    controlled=run/'controlled.bin';controlled.write_bytes(b''.join(cases));controlled_nodes=parse(controlled.read_bytes())
    results=[];coverage=dict(same_parent=0,split_parents=0,null_position_parent=0,null_rotation_parent=0,disabled=0,null_transform=0,explicit_scale=0)
    for label,path,inputs in [('controlled',controlled,controlled_nodes),('benchmark',benchmark,snapshots)]:
        outputs=[];logs=[]
        for name in ['dag_world_native_x86','dag_world_owned_x64']:
            target=run/(label+'-'+name+'.bin');logs.append(execute(name,[IFM,path,target],label+'-'+name));outputs.append(target.read_bytes())
        count=sum(map(len,inputs));require(all(len(b)==count*68 for b in outputs),'Output size')
        mismatches=[i for i in range(count) if outputs[0][i*68:(i+1)*68]!=outputs[1][i*68:(i+1)*68]]
        details=[dict(row=i,native=struct.unpack_from('<17I',outputs[0],i*68),owned=struct.unpack_from('<17I',outputs[1],i*68)) for i in mismatches[:5]]
        results.append(dict(label=label,cases=len(inputs),nodes=count,differences=len(mismatches),details=details,logs=logs,query_sha256=sha(path)))
        if label=='benchmark':
            selected=re.findall(r'normalized_index=(\d+) nodes=(\d+)',export_log)
            require(len(selected)==len(inputs),'Export binding metadata coverage')
            positions=[];offset=0
            for (index,size),group in zip(selected,inputs):
                index,size=int(index),int(size);require(size==len(group) and index<size,'Export binding index')
                positions.append(outputs[0][(offset+index)*68+56:(offset+index)*68+68]);offset+=size
            require(len(set(positions))==6,'Benchmark selected DAG did not vary across all six snapshots')
            results[-1]['selected_dag_positions']=[struct.unpack('<3f',point) for point in positions]
            results[-1]['distinct_selected_dag_positions']=len(set(positions))
        row=0
        for group in inputs:
            for n in group:
                pp,rp=struct.unpack_from('<II',outputs[0],row*68);coverage['same_parent' if pp==rp else 'split_parents']+=1;coverage['null_position_parent']+=pp==NULL;coverage['null_rotation_parent']+=rp==NULL;coverage['disabled']+=bool(n[2]&1);coverage['null_transform']+=not bool(n[2]&32);coverage['explicit_scale']+=bool(n[2]&64);row+=1
        if label=='controlled':
            for index,expected in [(1,[8,0,0,16,0,8,0,28,0,0,8,40]),(4,[12,0,0,16,0,12,0,10,0,0,12,14]),(6,[1,0,0,3,0,1,0,5,0,0,1,7])]:
                matrix=struct.unpack_from('<12f',outputs[0],index*68+8);require(list(matrix)==expected,'Independent matrix expectation row '+str(index));point=struct.unpack_from('<3f',outputs[0],index*68+56);require(point==(expected[3],expected[7],expected[11]),'Independent point expectation')
    require(all(coverage.values()),'Missing controlled branch coverage')
    guard_results=[]
    invalid=[('cycle',struct.pack('<I',1)+node(parent=0)),('outside parent',struct.pack('<I',1)+node(parent=7)),('nonfinite',struct.pack('<I',1)+node(p=(float('inf'),0,0))),('truncated',struct.pack('<I',1)+b'\0'),('unknown flags',struct.pack('<I',1)+node(flags=128))]
    for index,(label,data) in enumerate(invalid):
        path=run/('invalid-'+str(index)+'.bin');path.write_bytes(data)
        for name in ['dag_world_native_x86','dag_world_owned_x64']:
            result=subprocess.run([str(OUT/(name+'.exe')),str(IFM),str(path),str(run/('invalid-'+str(index)+'-'+name+'.bin'))],capture_output=True,text=True,timeout=10);require(result.returncode==1,'Invalid fixture accepted/crashed: '+label);guard_results.append(dict(test=label,executable=name,message=result.stderr.strip()))
    require(sha(IFM)==IFM_SHA and sha(SESSION)==SESSION_SHA,'Originals modified')
    differences=sum(r['differences'] for r in results)
    receipt=dict(status='PASS' if not differences else 'FAIL',scope='original recursive routines with controlled datamodel and owned-evaluated snapshots',executables=executables,original_ifm_sha256=IFM_SHA,original_session_sha256=SESSION_SHA,static_byte_count=sum(b-a for _,a,b in RANGES),static_evidence=evidence,comparisons=results,coverage=coverage,guards=guard_results,export_log=export_log,
        limitations=['No live SFM process, original DMX loader or original animation evaluation executed',
                     'Named attribute lookup and datamodel resolve/enumeration/type callbacks are controlled',
                     'Unique supplied parent relation only; actual reverse-reference order and general type ancestry not established',
                     'Private copied IFM patched only at attribute lookup and memset boundaries; original recursion/selectors/local/math routines execute',
                     'Fixture input buffers and sealed private executable bytes checked unchanged after each probe',
                     'Six snapshots use owned evaluated values; equality proves world-transform stage conditional on those inputs',
                     'No renderer/editor change or full native session lighting parity claim'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],comparisons=[{k:r[k] for k in ['label','cases','nodes','differences']} for r in results],coverage=coverage,receipt=str(run/'results.json'))));require(not differences,'Native/owned recursive DAG differences')


if __name__=='__main__':main()
