"""Full original bounds dispatcher + animated kernel + child/world/outer frames.

Controlled input contracts are explicitly retained; this is not live SFM parity.
"""
import itertools
import json
from pathlib import Path
import random
import struct
import subprocess
import sys
import tempfile
from verify_session_illumination_dag import IFM,IFM_SHA,SESSION,SESSION_SHA
from verify_dag_world_position import sha,require,node,parse,NULL
from verify_ifm_bounds_frames import static_check as frame_static
from verify_ifm_bounds_dispatch import static_check as dispatch_static
from verify_ifm_animated_bounds import static_check as animated_static

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/ifm-bounds-composite-20261015'
IDENTITY=[1,0,0,0,0,1,0,0,0,0,1,0]
BOX=[-1,-2,-3,1,2,3]
HITBOX=[-4,-5,-6,4,5,6]
FRAMES=[0,0,0,0,0,0,100,0,0,0,0,0]
NODES=[node(),node(p=(100,0,0)),node(parent=0,p=(3,0,0)),node(parent=0,p=(1000,1000,1000))]


def main():
    require(sha(IFM)==IFM_SHA and sha(SESSION)==SESSION_SHA,'Pinned originals changed');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));smoke='--smoke' in sys.argv
    evidence=frame_static()+dispatch_static()+animated_static();executables=[]
    for name,machine in [('composite_native_x86',0x14c),('composite_owned_x64',0x8664),('composite_safety_x64',0x8664)]:
        path=OUT/(name+'.exe');raw=path.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine,'Executable architecture');require(b'asInvoker' in raw,'Non-elevated execution manifest');executables.append(dict(path=str(path),sha256=sha(path),machine=hex(machine)))
    def execute(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],capture_output=True,text=True,timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' exit='+str(p.returncode)+' '+p.stdout+p.stderr);return p.stdout
    rows=[];metadata=[];rng=random.Random(2026101531);models=[]
    def append(flags=1|2|4|8|16|64,category=3,sequence=0,mode=0,render_flags=0,root=0,nodes=NODES,sequences=(BOX,),palette=(IDENTITY,),boxes=((0,HITBOX),),tick=50000,frames=FRAMES,world=IDENTITY,first_position=(10,0,0),first_orientation=(0,0,0,1),engine_bounds=BOX,hull=BOX,view=BOX,label='controlled'):
        values=list(engine_bounds)+list(hull)+list(view)+list(frames)+list(world)+list(first_position)+list(first_orientation);require(len(values)==49,'Composite values length')
        header=struct.pack('<Iiii6Ii',flags,category,sequence,mode,render_flags,root,len(nodes),len(sequences),len(palette),len(boxes),tick)
        rows.append(header+struct.pack('<49f',*values)+b''.join(nodes)+b''.join(struct.pack('<6f',*v) for v in sequences)+b''.join(struct.pack('<12f',*m) for m in palette)+b''.join(struct.pack('<i6f',bone,*b) for bone,b in boxes))
        raw_nodes=[struct.unpack('<III8f',n) for n in nodes];children=[i for i,n in enumerate(raw_nodes) if n[0]==root];child=children[0] if children else None
        metadata.append(dict(label=label,flags=flags,category=category,sequence=sequence,mode=mode,render_flags=render_flags,children=len(children),child=child,child_override=child is not None and not(raw_nodes[child][2]&1) and raw_nodes[child][1]!=NULL,boxes=len(boxes)))
    append(flags=0,label='analytical default')
    append(flags=3,category=1,mode=1,label='analytical fallback child then wrapper')
    append(flags=1|2|4|8|16|64|128,mode=1,label='analytical animated success false setup result')
    append(flags=1|2|4|8|32|64|512|1024,label='analytical lazy header acquired before animated bounds')
    append(flags=1|2|4|8|16,boxes=(),label='analytical unready empty header fallback')
    append(flags=1|2|4|8|16,label='analytical animated ignores studio readiness')
    append(sequence=-1,boxes=(),label='analytical sequence-minus1 fallback')
    append(flags=1|2|4|8,label='analytical unsuccessful lazy load repeats')
    append(flags=2,mode=1,label='analytical default bounds still get outer frame')
    append(flags=1|2|4|8|16|64|512|1024,sequence=2147483647,boxes=(),label='analytical invalid sequence clamps0 and direct first bone')
    override_nodes=[node(flags=96,p=(10,0,0),scale=2),node(flags=96,p=(100,0,0),scale=4),node(parent=0,override=1,flags=126,p=(3,4,5)),node(parent=0,p=(1000,1000,1000))]
    append(flags=3,category=1,mode=1,nodes=override_nodes,label='analytical override child then wrapper')
    append(flags=1|2|4|16|64,render_flags=0x20,label='analytical renderable force-compute flag')
    if not smoke:
        for flags,category,mode in itertools.product(range(2048),[0,1,3,7],[-1,0,1]):
            # Alternate empty/nonempty first set, local/override child, scale and
            # pose inputs while covering every external flag combination.
            override=bool(flags&128);nodes=override_nodes if override else NODES
            append(flags=flags,category=category,mode=mode,sequence=[-2,-1,0,1,99][(flags//64)%5],render_flags=[0,0x20,0xff,0xdf][(flags//16)%4],nodes=nodes,sequences=(BOX,[-9,-8,-7,9,8,7]),boxes=() if flags&512 else ((0,HITBOX),),tick=[0,-1,50000,2147483647,-2147483648][flags%5],hull=[0]*6 if flags%3==0 else BOX,label='exhaustive composite state')
        def random_box():return [rng.uniform(-32,0) for _ in range(3)]+[rng.uniform(0,32) for _ in range(3)]
        for case in range(4096):
            nodes=[]
            for i in range(3+case%8):
                parent=rng.randrange(i) if i and rng.random()<.8 else NULL;override=rng.randrange(i) if i and rng.random()<.6 else NULL
                nodes.append(node(parent,override,rng.randrange(128),[rng.uniform(-128,128) for _ in range(3)],[rng.uniform(-1.2,1.2) for _ in range(4)],rng.choice([1,-1,2,-2,.125,.5])))
            palette=[]
            for i in range(1+case%16):
                matrix=IDENTITY.copy();matrix[0]=rng.choice([1,-1,2,.125]);matrix[5]=rng.choice([1,-1,2,.5]);matrix[10]=rng.choice([1,-1,2,.25]);matrix[3],matrix[7],matrix[11]=[rng.uniform(-128,128) for _ in range(3)];palette.append(matrix)
            boxes=[(rng.randrange(len(palette)),random_box()) for _ in range(case%7)]
            frames=[rng.uniform(-512,512) for _ in range(3)]+[rng.uniform(-36000,36000) for _ in range(3)]+[rng.uniform(-512,512) for _ in range(3)]+[rng.uniform(-36000,36000) for _ in range(3)]
            world=IDENTITY.copy();world[0],world[5],world[10]=[rng.choice([1,-1,2,.125]) for _ in range(3)];world[3],world[7],world[11]=[rng.uniform(-128,128) for _ in range(3)]
            append(flags=rng.randrange(2048),category=rng.choice([0,1,3,7]),mode=rng.choice([-2147483648,-1,0,1,2147483647]),render_flags=rng.randrange(256),sequence=rng.choice([-2147483648,-2,-1,0,1,2147483647]),root=case%len(nodes),nodes=nodes,sequences=(random_box(),random_box()),palette=palette,boxes=boxes,frames=frames,world=world,first_position=[rng.uniform(-32,32) for _ in range(3)],first_orientation=[rng.uniform(-1.2,1.2) for _ in range(4)],engine_bounds=random_box(),hull=[0]*6 if case%4==0 else random_box(),view=random_box(),tick=rng.choice([0,-1,50000,2147483647,-2147483648]),label='varied finite full composition')
        old=json.loads((ROOT/'outputs/ifm-animated-bounds-20261015/verification-4io_ucwf/results.json').read_text())
        for m in old['models']:
            path=Path(m['path']);require(sha(path)==m['sha256'],'MDL metadata changed');raw=path.read_bytes();seq_count,seq_at=struct.unpack_from('<ii',raw,188);bone_count=struct.unpack_from('<i',raw,156)[0];sets,set_at=struct.unpack_from('<ii',raw,172);require(0<seq_count<=4096 and 0<=seq_at<=len(raw)-212*seq_count and 0<bone_count<=256 and sets>0,'MDL composite metadata range')
            box_count,box_rel=struct.unpack_from('<ii',raw,set_at+4);require(0<=box_count<=256 and 0<=set_at+box_rel<=len(raw)-68*box_count,'MDL hitbox table range')
            sequences=[list(struct.unpack_from('<6f',raw,seq_at+i*212+32)) for i in range(seq_count)];hull=list(struct.unpack_from('<6f',raw,128));view=list(struct.unpack_from('<6f',raw,104));boxes=[]
            for i in range(box_count):
                at=set_at+box_rel+i*68;bone=struct.unpack_from('<i',raw,at)[0];require(0<=bone<bone_count,'MDL hitbox bone range');boxes.append((bone,list(struct.unpack_from('<6f',raw,at+8))))
            models.append(dict(path=str(path),sha256=m['sha256'],sequences=seq_count,bones=bone_count,first_set_boxes=box_count))
            for j in range(16):
                palette=[]
                for i in range(bone_count):matrix=IDENTITY.copy();matrix[3]=float(i%7+j);matrix[7]=float(i%5-j);palette.append(matrix)
                append(flags=1|2|4|8|32|64|512|1024|(16 if j&1 else 0)|(128 if j&2 else 0)|(256 if j&4 else 0),category=3,mode=[-1,0,1][j%3],sequence=[-2,-1,0,seq_count-1,seq_count+7][j%5],nodes=override_nodes if j&8 else NODES,sequences=sequences,palette=palette,boxes=boxes,hull=hull,view=view,tick=50000+j,label=path.name+' actual MDL metadata with controlled poses')
        prior=ROOT/'outputs/dag-world-position-20261015/verification-ltggta1s';prior_receipt=json.loads((prior/'results.json').read_text());benchmark=prior/'benchmark.bin';digest=next(c['query_sha256'] for c in prior_receipt['comparisons'] if c['label']=='benchmark');require(sha(benchmark)==digest,'Prior evaluated snapshot changed')
        for index,nodes in enumerate(parse(benchmark.read_bytes())):
            packed=[struct.pack('<III8f',*n) for n in nodes]
            for root in range(len(nodes)):
                for mode in [0,1]:append(root=root,nodes=packed,mode=mode,flags=1|2|4|8|32|64|128|512|1024,label='owned-evaluated DAG snapshot '+str(index)+' with controlled hitbox pose')
    query=run/'queries.bin';query.write_bytes(b''.join(rows));outputs=[];logs=[]
    for name in ['composite_native_x86','composite_owned_x64']:
        target=run/(name+'.bin');logs.append(execute(name,[IFM,query,target],name));outputs.append(target.read_bytes());require(len(outputs[-1])==240*len(rows),'Composite output length')
    differences=[]
    for i in range(len(rows)):
        for phase in range(4):
            at=i*240+phase*60
            if outputs[0][at:at+60]!=outputs[1][at:at+60]:differences.append((i,phase))
    details=[dict(case=i,entry='wrapper' if phase//2 else 'core',repeat=phase%2,metadata=metadata[i],native=struct.unpack_from('<15I',outputs[0],i*240+phase*60),owned=struct.unpack_from('<15I',outputs[1],i*240+phase*60)) for i,phase in differences[:6]]
    (run/'comparison.json').write_text(json.dumps(dict(differences=len(differences),details=details),indent=2)+'\n')
    expectations={0:[[-16,-16,0,16,16,72]]*4,1:[[2,-2,-3,4,2,3]]*2+[[102,-2,-3,104,2,3]]*2,2:[HITBOX]*2+[[96,-5,-6,104,5,6]]*2,3:[HITBOX]*4,4:[BOX]*4,5:[HITBOX]*4,6:[BOX]*4,7:[BOX]*4,8:[[-16,-16,0,16,16,72]]*2+[[84,-16,0,116,16,72]]*2,9:[[-1,-2,-3,11,2,3]]*4,10:[[49,4,4,53,12,16]]*2+[[149,4,4,153,12,16]]*2,11:[HITBOX]*4}
    for i,bounds in expectations.items():
        for phase,expected in enumerate(bounds):require(list(struct.unpack_from('<6f',outputs[0],i*240+phase*60))==expected,'Independent composite bounds case '+str(i)+' phase '+str(phase))
    # Lazy acquisition already occurs inside the first animated query. The next
    # call reuses that header without a second load; failed loads retry twice.
    for phase,expected in enumerate([[1,1,0,1,256,256,0,1,0],[1,0,0,1,256,256,0,1,0]]*2):require(list(struct.unpack_from('<9I',outputs[0],3*240+phase*60+24))==expected,'Lazy-load event ordering/counts')
    for phase in range(4):require(struct.unpack_from('<I',outputs[0],7*240+phase*60+28)[0]==2,'Failed load must retry in studio path')
    coverage=dict(default_or_unsupported=0,engine_fallback=0,animated_success=0,studio_sequence=0,lazy_load_attempts=0,repeat_header_reuse=0,false_setup_filled=0,first_bone_queries=0,child_override_paths=0,outer_frame_applications=0)
    for i,m in enumerate(metadata):
        records=[struct.unpack_from('<9I',outputs[0],i*240+phase*60+24) for phase in range(4)]
        for phase,e in enumerate(records):
            sync,load,engine,setup,maximum,mask,time,world,bone=e
            coverage['default_or_unsupported']+=not sync and not engine;coverage['engine_fallback']+=bool(engine);coverage['animated_success']+=bool(setup);coverage['studio_sequence']+=bool(sync and not engine and not setup);coverage['lazy_load_attempts']+=load;coverage['false_setup_filled']+=bool(setup and not(m['flags']&256));coverage['first_bone_queries']+=bone
            coverage['child_override_paths']+=bool((engine or (sync and not setup)) and m['flags']&2 and m['mode'] and m['child_override']);coverage['outer_frame_applications']+=bool(phase//2 and m['mode'] and m['flags']&2 and m['children'])
        coverage['repeat_header_reuse']+=sum(bool(records[phase][1]) and not records[phase+1][1] for phase in [0,2])
    require(all(coverage.values()),'Missing complete composition coverage');safety=execute('composite_safety_x64',[],'safety');require(sha(IFM)==IFM_SHA and sha(SESSION)==SESSION_SHA and all(sha(Path(m['path']))==m['sha256'] for m in models),'Original input modified')
    receipt=dict(status='PASS' if not differences else 'FAIL',smoke=smoke,cases=len(rows),comparisons=len(rows)*4,bytes_per_fixture=240,differences=len(differences),details=details,coverage=coverage,executables=executables,logs=logs,safety=safety.strip(),ifm_sha256=IFM_SHA,session_sha256=SESSION_SHA,models=models,query_sha256=sha(query),static_range_byte_sum=sum(e.get('bytes',0) for e in evidence),static_evidence=evidence,
        limits=['Full original1023F310/10240520 execute with original1023EF30/1023BCD0/1023B640 and recursive DAG world math; no animated/child/wrapper result substitutions',
                'Sync, lazy header loading, studio readiness, engine category/bounds, named attributes, datamodel resolve/reverse enumeration/type declarations, clock, SetupBones palette and model-world virtual remain controlled',
                'State is a stable post-sync snapshot; real SFM model synchronization/animation/bone setup and live pose/mode binding remain unverified',
                'GetModel virtual supplies a nonnull controlled model tag; null virtual model and original engine fallback behavior are not covered',
                'Each entrypoint is queried twice on the same object/cache; loader only allowed to initialize header pointer, all other fixture buffers/canaries/sealed code unchanged',
                'Classic local sequence table and first hitbox set only; virtual-model/include-model remapping not covered',
                'Actual MDL metadata poses/palettes controlled; prior benchmark DAG snapshots are owned-evaluated, not original live session evaluation',
                'No renderer/editor integration, original write, application launch, dependency install or full editor rebuild'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ['status','smoke','cases','comparisons','differences','details','coverage','safety']}));print('receipt='+str(run/'results.json'));require(not differences,'Complete bounds composition differs')


if __name__=='__main__':main()
