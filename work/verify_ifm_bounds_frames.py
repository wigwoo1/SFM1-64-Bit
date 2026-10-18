"""Original complete child adjustment/world recursion and wrapper matrix vs x64.

No SFM process, original DMX evaluation or live datamodel ABI is claimed.
"""
import itertools
import json
from pathlib import Path
import random
import re
import struct
import subprocess
import tempfile
from pe_image import Image
from verify_session_illumination_dag import IFM,IFM_SHA,SESSION,SESSION_SHA
from verify_dag_world_position import sha,require,node,parse,NULL,RANGES as DAG_RANGES

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/ifm-bounds-frames-20261015'
RANGES=[('complete_child_adjustment',0x1023bcd0,0x1023bdb6),
        ('complete_wrapper_matrix',0x1023b640,0x1023b6b2),
        ('Euler_angle_matrix',0x104992b0,0x1049940d),
        ('Euler_angle_position_matrix',0x1049d830,0x1049d85b),
        ('DAG_local_transform_getter',0x104c2820,0x104c2850),
        ('DAG_child_count',0x104c2b00,0x104c2b07),
        ('typed_child_index',0x104c2e70,0x104c2ec6),
        ('child_override_predicate',0x104c3d50,0x104c3d6a),
        ('scale_corrected_transpose_inverse',0x1008bac0,0x1008bbb2),
        ('affine_composition',0x10498a60,0x10498b35),
        ('transpose_inverse',0x1049d6d0,0x1049d7db),
        ('affine_bounds',0x1049e820,0x1049ea1c),
        *DAG_RANGES]


def static_check():
    image=Image(IFM);rows=[];pending=None
    with (ROOT/'work/ifm-disassembly.txt').open() as f:
        for line in f:
            m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
            if m:
                pending=None;address=int(m[1],16)
                if any(a<=address<b for _,a,b in RANGES):pending=dict(address=address,bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());rows.append(pending)
            elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line).hex()
    evidence=[]
    for label,a,b in RANGES:
        selected=[r for r in rows if a<=r['address']<b];cursor=a
        for row in selected:
            require(row['address']==cursor,'Disassembly gap '+label);raw=bytes.fromhex(row['bytes']);offset=image.offset(cursor);require(image.b[offset:offset+len(raw)]==raw,'Stale static bytes '+label);cursor+=len(raw)
        require(cursor==b,'Incomplete static range '+label+': '+hex(cursor)+' != '+hex(b));evidence.append(dict(name=label,start=hex(a),end_exclusive=hex(b),bytes=b-a,instructions=selected))
    require(image.word(0x10a8bd00)==0x3c8efa35,'Radian factor');require(image.word(0x10a8f3c0)==0x80000000,'Negative pitch sign mask');require(image.string(0x10b8e468)=='overrideParent','Child predicate attribute literal')
    return evidence


def main():
    require(sha(IFM)==IFM_SHA and sha(SESSION)==SESSION_SHA,'Pinned original changed');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));evidence=static_check();executables=[]
    for name,machine in [('frames_native_x86',0x14c),('frames_owned_x64',0x8664),('frames_safety_x64',0x8664)]:
        path=OUT/(name+'.exe');raw=path.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine,'Executable architecture');require(b'asInvoker' in raw,'Non-elevated manifest');executables.append(dict(path=str(path),sha256=sha(path),machine=hex(machine)))
    def execute(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],capture_output=True,text=True,timeout=90);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' exit='+str(p.returncode)+' '+p.stdout+p.stderr);return p.stdout
    rows=[];metadata=[];rng=random.Random(2026101527);box=[-1,-2,-3,1,2,3];frames=[1,2,3,0,0,0,10,20,30,0,0,0]
    def append(nodes,label,root=0,mode=1,resolved=1,pose=frames,bounds=box):
        rows.append(struct.pack('<IIiI18f',len(nodes),root,mode,resolved,*(list(pose)+list(bounds)))+b''.join(nodes))
        unpacked=[struct.unpack('<III8f',n) for n in nodes];children=[i for i,n in enumerate(unpacked) if n[0]==root];child=children[0] if children else None
        branch='mode0' if not mode else 'unresolved' if not resolved else 'no_children' if child is None else 'override_world' if not(unpacked[child][2]&1) and unpacked[child][1]!=NULL else 'local'
        metadata.append(dict(label=label,root=root,mode=mode,resolved=resolved,first_child=child,branch=branch,first_child_flags=unpacked[child][2] if child is not None else None))
    def analytical(flags=32,override=NULL,mode=1,resolved=1):
        return [node(flags=96,p=(10,0,0),scale=2),node(flags=96,p=(100,0,0),scale=4),node(parent=0,override=override,flags=flags,p=(3,4,5)),node(parent=0,p=(1000,1000,1000))]
    append(analytical(),'local translation; second child ignored')
    append(analytical(126,1),'world override relative to scaled parent')
    append(analytical(96,1),'overrideParent present even without overridePos/Rot')
    append(analytical(127,1),'disableOverride forces local')
    append(analytical(126,1),'unresolved root bypass',resolved=0)
    append(analytical(126,1),'mode0 bypass',mode=0)
    append(analytical(0),'missing local transform becomes identity')
    append([node()],'no children bypass')
    # Exhaust every child flag combination, override presence, parent scale and
    # runtime gate. The second child always differs, so accidental scanning fails.
    for flags,override,parent_scale,mode,resolved in itertools.product(range(128),[NULL,1],[1,2,.125],[0,1,-1],[0,1]):
        nodes=analytical(flags,override);nodes[0]=node(flags=96,p=(10,-20,30),scale=parent_scale)
        pose=[rng.uniform(-4096,4096) for _ in range(3)]+[rng.uniform(-36000,36000) for _ in range(3)]+[rng.uniform(-4096,4096) for _ in range(3)]+[rng.uniform(-36000,36000) for _ in range(3)]
        append(nodes,'exhaustive flags and gates',mode=mode,resolved=resolved,pose=pose)
    for case in range(8192):
        nodes=[]
        for i in range(2+case%11):
            parent=rng.randrange(i) if i and rng.random()<.8 else NULL;override=rng.randrange(i) if i and rng.random()<.7 else NULL
            flags=rng.randrange(128);p=[rng.uniform(-128,128) for _ in range(3)];q=[rng.uniform(-1.2,1.2) for _ in range(4)];scale=rng.choice([1,-1,2,-2,.125,.5,1.0000001192092896])
            nodes.append(node(parent,override,flags,p,q,scale))
        pose=[rng.uniform(-8192,8192) for _ in range(3)]+[rng.uniform(-36000,36000) for _ in range(3)]+[rng.uniform(-8192,8192) for _ in range(3)]+[rng.uniform(-36000,36000) for _ in range(3)]
        bounds=[rng.uniform(-64,0) for _ in range(3)]+[rng.uniform(0,64) for _ in range(3)]
        append(nodes,'varied finite hierarchy',root=case%len(nodes),mode=rng.choice([0,1,-1,2147483647,-2147483648]),resolved=int(case%5!=0),pose=pose,bounds=bounds)
    # Euler quarter-turns, signed zero and angle-domain endpoints are separate
    # from the quaternion hierarchy cases and exercise both pose fields.
    for angle in [-36000,-360,-180,-90,-0.0,0.0,90,180,360,36000]:
        for axis in range(3):
            for pair in [0,1]:
                pose=frames.copy();pose[3+pair*6+axis]=angle;append(analytical(),'Euler axis/endpoints',pose=pose)
    prior=ROOT/'outputs/dag-world-position-20261015/verification-ltggta1s';prior_receipt=json.loads((prior/'results.json').read_text());benchmark=prior/'benchmark.bin'
    benchmark_digest=next(c['query_sha256'] for c in prior_receipt['comparisons'] if c['label']=='benchmark');require(sha(benchmark)==benchmark_digest,'Prior evaluated benchmark fixture changed');snapshots=parse(benchmark.read_bytes());require(len(snapshots)==6,'Benchmark snapshot count')
    for snapshot_index,nodes in enumerate(snapshots):
        packed=[struct.pack('<III8f',*n) for n in nodes]
        for root in range(len(nodes)):append(packed,'owned-evaluated benchmark snapshot '+str(snapshot_index),root=root,mode=1)
    query=run/'queries.bin';query.write_bytes(b''.join(rows));outputs=[];logs=[]
    for name in ['frames_native_x86','frames_owned_x64']:
        target=run/(name+'.bin');logs.append(execute(name,[IFM,query,target],name));outputs.append(target.read_bytes());require(len(outputs[-1])==76*len(rows),'Output length')
    different=[i for i in range(len(rows)) if outputs[0][i*76:(i+1)*76]!=outputs[1][i*76:(i+1)*76]]
    details=[dict(case=i,metadata=metadata[i],native=struct.unpack_from('<19I',outputs[0],i*76),owned=struct.unpack_from('<19I',outputs[1],i*76)) for i in different[:5]]
    for i,expected in enumerate([[2,2,2,4,6,8],[49,4,4,53,12,16],[2,2,2,4,6,8],[2,2,2,4,6,8],box,box,box,box]):
        require(list(struct.unpack_from('<6f',outputs[0],i*76))==expected,'Independent child bounds case '+str(i));matrix=list(struct.unpack_from('<12f',outputs[0],i*76+24));require(matrix==[1,0,0,9,0,1,0,18,0,0,1,27],'Independent wrapper translation case '+str(i))
    require([struct.unpack_from('<I',outputs[0],i*76+72)[0] for i in range(4)]==[0,1,1,0],'Independent child override policy')
    coverage={name:sum(m['branch']==name for m in metadata) for name in ['mode0','unresolved','no_children','override_world','local']};require(all(coverage.values()),'Missing frame branch coverage')
    safety=execute('frames_safety_x64',[],'safety');require(sha(IFM)==IFM_SHA and sha(SESSION)==SESSION_SHA,'Original modified')
    receipt=dict(status='PASS' if not different else 'FAIL',cases=len(rows),output_bytes_per_case=76,differences=len(different),details=details,coverage=coverage,executables=executables,logs=logs,safety=safety.strip(),ifm_sha256=IFM_SHA,session_sha256=SESSION_SHA,query_sha256=sha(query),benchmark_fixture_sha256=benchmark_digest,benchmark_cases=sum(m['label'].startswith('owned-evaluated') for m in metadata),static_byte_count=sum(b-a for _,a,b in RANGES),static_evidence=evidence,
        limitations=['Complete original1023BCD0 and1023B640 execute; no child/wrapper matrix answers substituted',
                     'Original child0 getter/predicate/local transform/recursive DAG world and affine math execute',
                     'Named attribute lookup, datamodel resolution/reverse enumeration/type declarations and memset are controlled',
                     'All supplied children are typed DAGs with unique ordinary parent relations; malformed child0 is safely rejected only in owned tests',
                     'Six prior benchmark snapshots use owned animation evaluation, not original live SFM session evaluation',
                     'Wrapper field38/44/50/5C live provenance and runtime mode lifecycle remain unverified',
                     'Euler inputs bounded to +/-36000 degrees; finite selected matrices required; singular parent frame rejected',
                     'Native fixture inputs, output canaries and sealed executable bytes checked unchanged',
                     'No editor integration, complete renderer parity, original writes, app launches or editor rebuild'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ['status','cases','differences','details','coverage','safety','benchmark_cases']}));print('receipt='+str(run/'results.json'));require(not different,'Bounds frame mismatch')


if __name__=='__main__':main()
