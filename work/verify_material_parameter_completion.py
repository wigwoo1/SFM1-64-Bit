"""Complete original material-system InitParams stage, controlled shader callback.

No concrete shader initialization, texture loading, live SFM, GPU or original
writes. Original callback services and empty logging buffer are explicit fixtures.
"""
from pathlib import Path
from datetime import datetime,timezone
from collections import Counter
import hashlib,json,random,re,struct,subprocess,tempfile
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
from verify_material_keyvalues_parser import text,DUMP
from verify_ifm_draw_packet import check_ranges
from verify_shader_parameter_registry import decode as decode_registry

OUT=ROOT/'outputs/material-parameter-completion-20261026'
PRIOR=ROOT/'outputs/material-parameter-collection-20261026/verification-v2a4rrm7'

def execute(exe,args,run,label):
    p=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=120)
    (run/(label+'.log')).write_text(p.stdout+p.stderr)
    require(p.returncode==0,'Execution '+label+' exit='+str(p.returncode)+': '+p.stdout+p.stderr)
    return p.stdout.strip()

def read_queries(raw):
    at=0;cases=[]
    def string():
        nonlocal at
        n=struct.unpack_from('<I',raw,at)[0];at+=4;s=raw[at:at+n].decode('ascii');at+=n;return s
    while at<len(raw):
        q=struct.unpack_from('<12I',raw,at);at+=48
        names=[string() for _ in range(q[8])];flags=[string() for _ in range(q[9])];nodes=[]
        for _ in range(q[10]+q[11]):
            kind,bits=struct.unpack_from('<2I',raw,at);at+=8;nodes.append((kind,bits,string(),string()))
        cases.append(dict(q=list(q),names=names,flags=flags,nodes=nodes))
    require(at==len(raw),'Original collection query span');return cases

def decode(raw,count):
    at=0;result=[]
    for _ in range(count):
        n,w=struct.unpack_from('<2I',raw,at);at+=8;values=[]
        for _ in range(n):
            q=struct.unpack_from('<28I',raw,at);at+=112
            s=raw[at:at+q[10]];at+=q[10];name=raw[at:at+q[11]];at+=q[11]
            values.append(dict(words=list(q),text_hex=s.hex(),name=name.decode('ascii')))
        size=struct.unpack_from('<I',raw,at)[0];at+=4
        lookups=list(struct.unpack_from('<'+str(size)+'I',raw,at));at+=4*size
        result.append(dict(warnings=w,values=values,lookups=lookups))
    require(at==len(raw),'Completion exact output framing');return result

def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    prior=json.loads((PRIOR/'results.json').read_text());require(prior['status']=='PASS' and sha(PRIOR/'queries.bin')==prior['query_sha256'],'Accepted collector input')
    require(sha(LIB)==sha(PRIVATE)==PIN,'Pinned original library')
    for p,h in prior['sources'].items():require(sha(Path(p))==h,'Prior component source '+p)
    numeric=json.loads((ROOT/'outputs/material-keyvalues-numeric-20261026/summary.json').read_text())
    protected=json.loads(Path(numeric['raw_receipt']).read_text())['original_hashes']
    for p,h in protected.items():require(sha(Path(p))==h,'Protected original '+p)
    image,evidence=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('complete_material_system_InitParams',0x10036bb0,0x10036de2),
        ('complete_empty_log_drain',0x10076bf0,0x10076d7f),
        ('complete_identity_matrix',0x1006c9a0,0x1006ca02)])
    expected=[0x10036ce1,0x10036cf2,0x10036d1d,0x10036d3d,0x10036d60,0x10036dc8,0x10036d89,0x10036ce1,0x10036d9e,0x10036dae,0x10036cd0]
    require([image.word(0x10036de4+4*i) for i in range(11)]==expected,'Original type dispatch table')
    require(image.word(0x1009c9dc+14*4)==0x10036bb0,'Actual shader system method')
    # Revalidate actual registry independently; the stage itself still uses a
    # controlled shader callback, never a claimed complete VertexLitGeneric.
    regdir=ROOT/'outputs/material-effective-parameters-20261017/verification-l11z2le6'
    reg=json.loads((regdir/'results.json').read_text());rex=reg['executable'];require(sha(Path(rex['path']))==rex['sha256'],'Registry executable')
    libs=regdir/'copied-libraries'
    for name in ['stdshader_dx9.dll','tier0.dll']:
        original=next(p for p in reg['original_hashes'] if Path(p).name==name)
        require(sha(Path(original))==sha(libs/name)==reg['original_hashes'][original],'Registry protected library')
    registry=run/'registry.bin';execute(Path(rex['path']),[libs,registry],run,'registry')
    require(sha(registry)==reg['output_sha256'],'Registry wire identity')
    records=decode_registry(registry.read_bytes());vl=next(r for r in records if r['name']=='VertexLitGeneric')['parameters']
    actual_names=[p['name'] for p in vl];actual_types=[p['type'] for p in vl]
    rng=random.Random(2026102607);cases=[];groups=[]
    def add(c,types,actions=(),repeats=1,label='controlled'):
        c=dict(c);c['types']=types;c['actions']=list(actions);c['repeats']=repeats
        require(len(types)==len(c['names']) and len(types)>=6,'Stage descriptor count')
        cases.append(c);groups.append(label)
    # All admitted collector inputs with6+ descriptors, plus random type
    # completion and explicit initializer mutations. Actual125-name inputs use
    # their real registry types rather than invented type assignments.
    prior_cases=read_queries((PRIOR/'queries.bin').read_bytes())
    for c in prior_cases:
        if len(c['names'])<6:continue
        real=c['names']==actual_names
        types=actual_types if real else [rng.choice(list(range(12))+[12,0xffffffff]) for _ in c['names']]
        actions=[]
        if not real:
            for _ in range(rng.randrange(4)):
                actions.append((rng.randrange(len(types)),rng.choice([0,1,2,3,4,6]),*[rng.choice([0,0x80000000,0x3e800000,0x3f800000,0x7fc12345,rng.getrandbits(32)]) for _ in range(4)]))
        add(c,types,actions,1+rng.randrange(3),label='actual_registry_model' if real else 'prior_collector_composition')
    names=['$flags','$flags_defined','$flags2','$flags_defined2','$color','$alpha']+['$p'+str(i) for i in range(16)]
    type_domain=list(range(12))+[12,0xffffffff,0x80000000,256]
    def simple(nodes=()):return dict(q=[6,0,0,0,0,0,0,0,len(names),0,len(nodes),0],names=names,flags=[],nodes=list(nodes))
    basic=len(cases);add(simple(),[1,1,1,1,0,11]+type_domain,label='all_type_defaults')
    # Undefined vector/matrix/string caches and resources must clear correctly;
    # callback writes occur before defaults, and defined values bypass lookup.
    for original in ['', '[1 2 3]', '[1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1]', 'hello']:
        for t in type_domain:
            for op in [0,1,2,3,4,6]:
                add(simple([(1,0,'$p0',original)]),[1]*6+[t]*16,[(6,op,0x3e800000,0x80000000,0x3f800000,0)],3,label='callback_resource_cache_transitions')
    for t in type_domain:
        for slot in [4,5]:
            for op in [0,1,2,3,4,6]:add(simple(),[t]*len(names),[(slot,op,0x3e800000,0,0,0)],2,label='color_alpha_special_slots')
    # Every type with both explicit numeric/trig feature flags and poisoned
    # descriptor default text. No default text is supplied to the owned stage.
    for mode in range(8):
        for t in type_domain:
            c=simple();c['q'][0]=mode;add(c,[t]*len(names),repeats=2,label='all_type_cpu_model_states')
    def packet(c):
        return struct.pack('<12I',*c['q'])+b''.join(text(s) for s in c['names']+c['flags'])+b''.join(struct.pack('<2I',t,b)+text(n)+text(s) for t,b,n,s in c['nodes'])+struct.pack('<2I',c['repeats'],len(c['actions']))+struct.pack('<'+str(len(c['types']))+'I',*c['types'])+b''.join(struct.pack('<6I',*a) for a in c['actions'])
    query=run/'queries.bin';query.write_bytes(b''.join(packet(c) for c in cases));outputs=[];executables=[];native_log=''
    absent=run/'no-original-engine.dll';require(not absent.exists(),'Owned engine independence')
    for label,arch,machine in [('native','x86',0x14c),('owned','x64',0x8664)]:
        exe=OUT/f'completion_{label}_{arch}.exe';b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0]
        require(struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==(0x10b if machine==0x14c else 0x20b) and b'asInvoker' in b,'Architecture/manifest')
        log=OUT/(label+'-build.log');require(not re.search(r'\b(warning|error)\b',log.read_text(),re.I),'Clean build '+label)
        imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout
        modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require(set(m.lower() for m in modules)=={'kernel32.dll'},'Direct dependency '+label)
        result=run/(label+'.bin');message=execute(exe,[PRIVATE if label=='native' else absent,query,result],run,label)
        if label=='native':native_log=message
        outputs.append(result.read_bytes());executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules))
    native,owned=[decode(b,len(cases)) for b in outputs]
    differences=[dict(case=i,group=groups[i],input=cases[i],native=a,owned=b) for i,(a,b) in enumerate(zip(native,owned)) if a!=b]
    (run/'comparison.json').write_text(json.dumps(dict(cases=len(cases),differences=differences),indent=2)+'\n')
    require(not differences and outputs[0]==outputs[1],'Completion comparison; see comparison.json')
    # Independent expected constants, not merely matching two implementations.
    first=native[basic];v=first['values'];require([x['words'][1]&15 for x in v[6:]]==[6,4,2,2,2,2,6,0,4,5,7,8,6,6,6,6],'Analytical type mapping')
    require(v[4]['words'][3:7]==[0x3f800000]*3+[0] and v[5]['words'][3]==0x3f800000,'Color white/alpha one')
    require(first['lookups']==list(range(21,5,-1)),'Reverse descriptor lookup order/defined skip')
    require(v[15]['words'][7]==1 and v[17]['words'][7]==2,'Null resources')
    require(v[16]['words'][8:10]==[1,1] and v[16]['words'][12:28]==[0x3f800000 if i%5==0 else 0 for i in range(16)],'Identity matrix')
    analytical=execute(Path(executables[1]['path']),['--test'],run,'analytical');require('refusals=6' in analytical,'Transactional refusals')
    for p,h in prior['sources'].items():require(sha(Path(p))==h,'Prior source changed')
    for p,h in protected.items():require(sha(Path(p))==h,'Protected original changed')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());require(all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()),'Production source changed')
    require(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor changed')
    sources={};pending=[ROOT/'work'/n for n in ['material_parameter_completion_candidate.h','material_parameter_completion_native_fixture.h','probe_material_parameter_completion.cpp','build_material_parameter_completion.cmd','verify_material_parameter_completion.py']]
    while pending:
        p=pending.pop()
        if str(p) in sources:continue
        sources[str(p)]=sha(p)
        if p.suffix in ['.h','.cpp']:
            for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(),re.M):
                if (p.parent/inc).is_file():pending.append(p.parent/inc)
    sources[str(ROOT/'work/material_trig_x87_candidate.inc')]=sha(ROOT/'work/material_trig_x87_candidate.inc')
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),cases=len(cases),stage_invocations=sum(c['repeats'] for c in cases),groups=dict(Counter(groups)),exact_bytes=len(outputs[0]),differences=0,query_sha256=sha(query),executables=executables,analytical=analytical,native_log=native_log,value_types=dict(Counter(v['words'][1]&15 for d in native for v in d['values'])),static_ranges=evidence,sources=sources,protected_original_files=len(protected),editor_dependencies_unchanged=len(build['editor_source_dependencies']),editor_sha256=build['executable_sha256'],registry_output_sha256=sha(registry),limits=['Material-system InitParams surrounding/completion stage executes unchanged with real collected CMaterialVars, real setters/resources/destruction, explicit fake shader initializer callback and empty logging buffer','Not concrete VertexLitGeneric/Phong initialization, descriptor default text parsing, fallback selection, texture resources, proxy execution, IMaterial completion or render equivalence','Shader callback actions deliberately cover numeric/vector/Undefined setters only. Initial collected strings and matrices plus subsequently created zero FourCC/null material/identity matrix resources are covered; no nonnull resource or live concurrency claim','Public value-owned API is transactional on unsupported input or callback failure; no original heap/notification timing equivalence claim','Native original instructions are RX/unpatched; node/name/hardware/owner/allocator/logging services remain controlled single-thread verification scaffolding','No production integration/build/run, live SFM/computer use, original writes, GPU or Blender. Full application remains incomplete'])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(path=str(run),receipt_sha256=sha(run/'results.json')),indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ['status','cases','stage_invocations','exact_bytes','value_types','analytical']},indent=2))
if __name__=='__main__':main()
