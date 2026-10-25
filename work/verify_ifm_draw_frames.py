"""Private original frame update/decomposition; no app or live-process access."""
from datetime import datetime,timezone
import hashlib
import itertools
import json
import math
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from verify_ifm_draw_packet import check_ranges
from verify_session_illumination_dag import IFM,IFM_SHA,SESSION,SESSION_SHA

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/draw-frames-20261025'
IDENTITY=[1,0,0,0,0,1,0,0,0,0,1,0]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def require(v,m):
    if not v:raise RuntimeError(m)
def f32(v):return struct.unpack('<f',struct.pack('<f',v))[0]
def point(p,m):return [f32(f32(f32(f32(m[r*4+1]*p[1])+f32(m[r*4]*p[0]))+f32(m[r*4+2]*p[2]))+m[r*4+3]) for r in range(3)]

def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    originals={str(p):sha(p) for p in [IFM,SESSION]}
    require(originals[str(IFM)]==IFM_SHA and originals[str(SESSION)]==SESSION_SHA,'Original pin')
    tracked=['ifm_draw_frames_candidate.h','probe_ifm_draw_frames.cpp','test_ifm_draw_frames.cpp','build_ifm_draw_frames.cmd','verify_ifm_draw_frames.py',
        'transform_scale.h','ik_matrix_angles.h','ik_matrix_vector.h','ifm_bounds_frames_candidate.h','probe_vulkan_win32_surface_x64.cpp']
    sources={p:sha(ROOT/'work'/p) for p in tracked}
    editor=ROOT/'outputs/vtf-legacy-thumbnail-20261024/sfm64_session_ui_vtf_legacy.exe'
    editor_sha=sha(editor);require(editor_sha=='2e79aed74e3831196a609346251680132f7beeadcc0cb810318f1fb9f5156ed2','Prior editor changed')
    image,evidence=check_ranges(IFM,'ifm-disassembly.txt',[
        ('complete_frame_update',0x102402c0,0x1024051d),('complete_frame_decomposition',0x1023ba60,0x1023bbe5),
        ('complete_geometric_scale',0x1007fdf0,0x1007ff04),('complete_matrix_angles',0x10497030,0x10497164),
        ('point_transform',0x10498390,0x10498431),('identity_matrix',0x104990b0,0x104990dd),
    ])
    constants={hex(a):hex(image.word(a)) for a in [0x10aa53c8,0x10aa53c4,0x10a8bd04,0x10a8e010,0x10a8a56c,0x10a8a87c]}
    require(list(constants.values())==['0x34000000','0xb4000000','0x42652ee1','0x3a83126f','0x3f800000','0x3f000000'],'Math constants')
    require(struct.unpack_from('<d',image.b,image.offset(0x10aa53a0))[0]==0.3333333432674408,'Scale exponent')
    warnings={};executables=[]
    for name,machine in [('frames_native_x86',0x14c),('frames_owned_x64',0x8664),('frames_safety_x64',0x8664)]:
        p=OUT/(name+'.exe');raw=p.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==(0x10b if machine==0x14c else 0x20b),'Architecture '+name)
        executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)))
    for name in ['native','owned','safety']:
        log=(OUT/(name+'-build.log')).read_text();warnings[name]=[x for x in log.splitlines() if 'warning ' in x.lower()];require(not warnings[name] and 'error C' not in log,'Build diagnostics '+name)
    def execute(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=ROOT,capture_output=True,text=True,timeout=90)
        (run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' '+str(p.returncode)+' '+p.stdout+p.stderr);return p.stdout
    safety=execute('frames_safety_x64',[],'safety');require('checks=7 rejections=5' in safety,'Safety coverage')
    rng=random.Random(2026102501);matrices=[IDENTITY,[0]*12];labels=['identity','zero basis']
    def add(matrix,label):matrices.append(list(struct.unpack('<12f',struct.pack('<12f',*matrix))));labels.append(label)
    for scale in [-2,-0.0,1e-12,2**-23,1-2**-24,1,1+2**-23,2,1024]:
        for axis in [0,1,2,3]:
            m=IDENTITY.copy()
            for r in range(3):
                for c in range(3):m[r*4+c]*=scale if axis==3 or c==axis else 1
            m[3:12:4]=[13,-27,31];add(m,'axis/uniform scale')
    for horizontal in [0,.0009999,.001,.0010001,1]:
        for sign in [-1,1]:
            m=IDENTITY.copy();m[0]=horizontal;m[8]=sign;add(m,'Euler branch boundary')
    for i in range(8192):
        # Arbitrary finite affine matrices include shear, reflection and unequal
        # scales; not just orthogonal rotations that trivially cancel scale.
        m=[rng.uniform(-8,8) for _ in range(12)]
        magnitude=rng.choice([1e-8,.125,1,2,1e4])
        for r in range(3):
            for c in range(3):m[r*4+c]*=magnitude
        add(m,'finite affine')
    prior=ROOT/'outputs/bone-world-palette-20261024/verification-0v8wip82'
    receipt=json.loads((prior/'results.json').read_text());corpus=prior/'queries.bin';require(sha(corpus)==receipt['query_sha256'],'Prior benchmark corpus hash')
    raw=corpus.read_bytes();cursor=0;total=0;benchmark=[]
    while cursor<len(raw):
        count,mask=struct.unpack_from('<II',raw,cursor)
        if total>=receipt['controlled_cases']:benchmark.append(list(struct.unpack_from('<12f',raw,cursor+8)))
        cursor+=56+count*40;total+=1
    require(cursor==len(raw) and total==receipt['total_cases'] and len(benchmark)==4,'Benchmark corpus shape')
    for m in benchmark:add(m,'retained owned benchmark world matrix; native draw binding unverified')
    decompose_query=run/'decompose-queries.bin';decompose_query.write_bytes(b''.join(struct.pack('<12f',*m) for m in matrices))
    def compare(operation,query,count,width):
        outputs=[]
        for name in ['frames_native_x86','frames_owned_x64']:
            p=run/(operation+'-'+name+'.bin');execute(name,[IFM,operation,query,p],operation+'-'+name);outputs.append(p.read_bytes())
        require(len(outputs[0])==len(outputs[1])==count*width,'Output length '+operation)
        differences=[i for i in range(count) if outputs[0][i*width:(i+1)*width]!=outputs[1][i*width:(i+1)*width]]
        comparison=dict(cases=count,exact_bytes=len(outputs[0]),differing_cases=len(differences),examples=[dict(case=i,native=struct.unpack_from('<'+str(width//4)+'I',outputs[0],i*width),owned=struct.unpack_from('<'+str(width//4)+'I',outputs[1],i*width)) for i in differences[:4]])
        (run/(operation+'-comparison.json')).write_text(json.dumps(comparison,indent=2)+'\n')
        require(not differences,'Original/owned '+operation+' mismatch; retain evidence for focused diagnosis')
        return outputs[0],comparison
    decomposed,decomposition=compare('decompose',decompose_query,len(matrices),28)
    require(struct.unpack_from('<7f',decomposed,0)==(0,0,0,0,0,0,1),'Independent identity decomposition')
    for i,m in enumerate(matrices):require(list(struct.unpack_from('<3f',decomposed,i*28+12))==m[3:12:4],'Decomposition translation preservation')
    rows=[];meta=[]
    def append(model=1,mode=1,count=1,child=1,root=IDENTITY,child_world=IDENTITY,bounds=(-16,-16,0,16,16,72)):
        row=struct.pack('<4i30f',model,mode,count,child,*root,*child_world,*bounds);rows.append(row);meta.append(struct.unpack('<4i30f',row))
    translated=IDENTITY.copy();translated[3:12:4]=[10,20,30]
    append(root=translated,bounds=(2,4,6,4,8,10))
    for flags in itertools.product([0,1],[-2147483648,-1,0,1,2147483647],[-2147483648,-1,0,1,2147483647],[0,1]):append(*flags,root=translated)
    for i in range(4096):
        root=rng.choice(matrices);cw=rng.choice(matrices);bounds=[rng.uniform(-64,64) for _ in range(6)]
        append(rng.randrange(2),rng.choice([-1,0,1]),rng.choice([-1,0,1,2]),rng.randrange(2),root,cw,bounds)
    for m in benchmark:
        for mode in [0,1]:append(mode=mode,root=m,child_world=translated)
    # Original skip gates must bypass unused poisoned matrix/bounds inputs.
    poison=[float('nan')]*12
    append(model=0,root=poison,child_world=poison,bounds=[float('nan')]*6)
    append(mode=0,child_world=poison,bounds=[float('nan')]*6)
    append(count=0,child_world=poison,bounds=[float('nan')]*6)
    append(child=0,child_world=poison,bounds=[float('nan')]*6)
    query=run/'frame-queries.bin';query.write_bytes(b''.join(rows));frames,frame_comparison=compare('frames',query,len(rows),136)
    active=0
    for i,h in enumerate(meta):
        model,mode,count,child=h[:4];root=h[4:16];bounds=h[28:34];selected=bool(model and mode and count>0 and child)
        events=[1]+([2] if model else [])+[1]+([3] if mode and model else [])+([4] if mode and model and count>0 else [])
        if selected:events += [5,6,2];active+=1
        events += [7];record=frames[i*136:(i+1)*136];observed=struct.unpack_from('<10I',record,96)
        require(observed==(len(events),*events,*([0]*(9-len(events)))),'Independent call ordering')
        expected=point([f32(f32(bounds[j+3]+bounds[j])*.5) for j in range(3)],root) if selected else list(root[3:12:4]) if model else [0,0,0]
        require(struct.pack('<3f',*expected)==record[:12],'Independent exact center/translation')
        require(list(struct.unpack_from('<3f',record,24))==(list(root[3:12:4]) if model else [0,0,0]),'Base position50 retained')
        if not selected:require(record[48:96]==bytes(48),'Unused bounds provider')
        else:require(record[48:60]==record[24:36] and record[60:72]==record[12:24],'Intermediate bounds frame ordering')
    require(struct.unpack_from('<3f',frames)==(13,26,38),'Analytical changed draw center')
    require(all(sha(p)==v for p,v in originals.items()) and all(sha(ROOT/'work'/p)==v for p,v in sources.items()),'Original/source changed')
    require(all(sha(e['path'])==e['sha256'] for e in executables) and sha(editor)==editor_sha,'Binary changed')
    result=dict(status='PASS',checked_at_utc=datetime.now(timezone.utc).isoformat(),decomposition=decomposition,frames=frame_comparison,
        selected_bounds_cases=active,safety=safety.strip(),constants=constants,static_instruction_bytes=sum(e['bytes'] for e in evidence),
        static_ranges=[{k:v for k,v in e.items() if k!='instructions'} for e in evidence],sources_sha256=sources,original_sha256=originals,
        executables=executables,warnings=warnings,decomposition_query_sha256=sha(decompose_query),frame_query_sha256=sha(query),
        prior_benchmark_corpus=dict(path=str(corpus),sha256=sha(corpus),cases=4,binding='owned evaluated world matrices, not native live draw frame'),
        prior_editor=dict(path=str(editor),sha256=editor_sha,rebuilt=False,run=False),limitations=[
            'Original complete102402C0 and1023BA60 execute in private PE copy. DM resolution, world/child providers, inner bounds and spatial notification are controlled.',
            'HandleFFFF bypasses changed-object notification branch. Full spatial/cache lifetime behavior is not implemented by the owned frame-value helper.',
            'Original scale/angle math executes. Exact results apply to tested finite matrices and default test-process floating environment, not every float or native engine runtime math configuration.',
            'Four retained owned benchmark matrices are numerical inputs only; actual graph child0, bounds provider and native draw-frame binding remain to compose.',
            'No editor dependency modified or viewport changed; no computer use, live process probes, original writes, original application launch/control/close, Blender or GPU job.',
        ])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(status='PASS',decomposition_cases=len(matrices),frame_cases=len(rows),selected_bounds_cases=active)));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
