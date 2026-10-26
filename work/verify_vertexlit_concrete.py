"""Real registry + collector + concrete shader InitParams + system completion.

Reference uses private library copies and real material variables. Hardware,
config, command line, immediate queue/allocator and logging remain explicit
services. Owned x64 code does not load original libraries. No live SFM/GPU.
"""
from pathlib import Path
from datetime import datetime,timezone
from collections import Counter
import hashlib,itertools,json,random,re,struct,subprocess,tempfile,zlib
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
from verify_material_parameter_completion import read_queries
from verify_shader_parameter_registry import decode as decode_registry
from verify_material_keyvalues_parser import text,DUMP
from verify_ifm_draw_packet import check_ranges

OUT=ROOT/'outputs/vertexlit-concrete-20261026'
META=ROOT/'outputs/material-effective-parameters-20261017/verification-l11z2le6'
STD=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/bin/stdshader_dx9.dll')
STD_PIN='fb09e2ddc8a99aff4ef31e3fddfdb1ca84d89379336abdab4e76e94adfa7e6f8'

def execute(exe,args,run,label):
    p=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=120)
    (run/(label+'.log')).write_text(p.stdout+p.stderr)
    require(p.returncode==0,'Execution '+label+' exit='+str(p.returncode)+': '+p.stdout+p.stderr)
    return p.stdout.strip()

def decode(raw,count):
    at=0;rows=[]
    for _ in range(count):
        n,w=struct.unpack_from('<2I',raw,at);at+=8;values=[]
        for _ in range(n):
            q=struct.unpack_from('<28I',raw,at);at+=112;s=raw[at:at+q[10]];at+=q[10];name=raw[at:at+q[11]];at+=q[11]
            values.append(dict(words=list(q),text=s.decode('ascii'),name=name.decode('ascii')))
        route=struct.unpack_from('<I',raw,at)[0];at+=4;rows.append(dict(warnings=w,values=values,route=route))
    require(at==len(raw),'Complete concrete wire extent');return rows

def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    previous=json.loads((ROOT/'outputs/material-parameter-completion-20261026/summary.json').read_text());receipt=json.loads(Path(previous['receipt']).read_text())
    for p,h in receipt['sources'].items():require(sha(Path(p))==h,'Prior completion source '+p)
    shared=json.loads((ROOT/'outputs/vertexlit-shared-init-20261025/completion-check.json').read_text())
    for p,h in shared['sources_sha256'].items():require(sha(ROOT/'work'/p)==h,'Accepted shared initializer '+p)
    numeric=json.loads((ROOT/'outputs/material-keyvalues-numeric-20261026/summary.json').read_text());protected=json.loads(Path(numeric['raw_receipt']).read_text())['original_hashes']
    for p,h in protected.items():require(sha(Path(p))==h,'Protected original '+p)
    require(sha(LIB)==sha(PRIVATE)==PIN and sha(STD)==sha(META/'copied-libraries/stdshader_dx9.dll')==STD_PIN,'Protected original/private libraries')
    reg=json.loads((META/'results.json').read_text());registry=run/'registry.bin';rex=reg['executable'];require(sha(Path(rex['path']))==rex['sha256'],'Registry executable')
    for name in ['tier0.dll','stdshader_dx9.dll']:
        p=next(p for p in reg['original_hashes'] if Path(p).name==name);require(sha(Path(p))==sha(META/'copied-libraries'/name)==reg['original_hashes'][p],'Registry library')
    execute(Path(rex['path']),[META/'copied-libraries',registry],run,'registry');require(sha(registry)==reg['output_sha256'],'Fresh registry wire')
    family=next(r for r in decode_registry(registry.read_bytes()) if r['name']=='VertexLitGeneric');params=family['parameters'];names=[p['name'] for p in params];types=[p['type'] for p in params]
    schema=(ROOT/'work/vertexlit_parameter_schema_candidate.h').read_text();require(re.findall(r'^        "([^"]+)"',schema,re.M)==names,'Compiled registered names')
    require([int(s) for s in re.search(r'TypesCandidate\(\)\{return \{([^}]+)',schema)[1].split(',')]==types,'Compiled registered types')
    image,evidence=check_ranges(STD,'stdshader_dx9-disassembly.txt',[
        ('complete_real_InitParams_wrapper',0x10064a30,0x10064a57),
        ('complete_concrete_VertexLitGeneric_InitParams',0x10053420,0x10053601),
        ('complete_cloak_defaults',0x10008fa0,0x100090e2),
        ('complete_emissive_defaults',0x10015b50,0x10015c20),
        ('complete_flesh_defaults',0x1001f250,0x1001f51a),
        ('complete_flesh_mapping',0x10053360,0x1005341d)])
    # Confirm functional family mapping from descriptor index globals, including
    # the effects not covered by the earlier shared-helper proof.
    globals_to_indices={0x10000000+p['rva']+20:i for i,p in enumerate(params) if i>=13}
    expected={0x100ca658:73,0x100cad48:65,0x100caf28:77}
    require(all(globals_to_indices[a]==i for a,i in expected.items()),'Three effect enable index globals')
    require(image.word(0x100920e0+12)==0x38d1b717 and image.word(0x1008fbe8)==0x3de147ae,'Original numeric effect defaults')
    prior_dir=ROOT/'outputs/material-parameter-collection-20261026/verification-v2a4rrm7';prior=json.loads((prior_dir/'results.json').read_text())
    require(sha(prior_dir/'queries.bin')==prior['query_sha256'],'Accepted collected model declarations')
    model=[c for c in read_queries((prior_dir/'queries.bin').read_bytes()) if c['names']==names];require(len(model)==12,'Actual model declaration cases')
    cases=[];labels=[]
    def add(nodes=(),settings=(0,0,0,92,0),mode=22,repeats=1,label='controlled',source=None):
        c=dict(source) if source else dict(q=[mode,0,0,0,0,0,0,0,125,0,len(nodes),0],names=names,flags=[],nodes=list(nodes))
        require(c['q'][8]+c['q'][10]+c['q'][11]<=256,'Original collector output capacity')
        c['settings']=settings;c['repeats']=repeats;cases.append(c);labels.append(label)
    def integer(i,v):return (2,v&0xffffffff,names[i],'')
    def string(i,s):return (1,0,names[i],s)
    plain=len(cases);add(label='undefined_defaults')
    # Actual preserved model inputs, both shader branches/hardware/config/tool
    # states. No source file contents are embedded in public verifier code.
    for c in model:
        for settings in [(0,0,0,92,0),(0,1,0,92,1),(0x100,1,1,91,0),(0x180,1,0,95,1)]:add(source=c,settings=settings,repeats=2,label='actual_model_materials')
    # Real CMaterialVar types/caches, flag conflicts and native shared Phong
    # dispatch under both sides of the hardware threshold.
    for mat_phong,phong,mask,level,config,tools in itertools.product([0,1],[0,1],[0,1,2],[91,92],[0,0x80,0x100,0x180],[0,1]):
        nodes=[integer(48,phong),integer(49,mask),string(6,'fixture/base'),string(13,'fixture/albedo'),string(26,'fixture/bump'),string(20,'fixture/env'),string(22,'fixture/envmask')]
        add(nodes,(config,mat_phong,1,level,tools),repeats=2,label='Phong_hardware_config_grid')
    effect_indices=[65,73,77]
    for enabled in itertools.product([0,1,-1],repeat=3):
        for phong,tools in itertools.product([0,1],[0,1]):
            nodes=[integer(i,n) for i,n in zip(effect_indices,enabled)]+[integer(48,phong),integer(49,1),string(6,'fixture/base')]
            add(nodes,(0,1,0,92,tools),repeats=3,label='all_optional_effect_combinations')
    rng=random.Random(2026102608)
    for _ in range(1024):
        nodes=[]
        for i in rng.sample(list(range(4,125)),rng.randrange(1,30)):
            kind=rng.randrange(4)
            if i==13:nodes.append(string(i,'fixture/albedo'));continue
            if kind==0:nodes.append(integer(i,rng.choice([0,1,-1,0x10000,0x400000,0x100000,rng.getrandbits(32)])))
            elif kind==1:nodes.append((3,rng.choice([0,0x80000000,0x3f800000,0x7fc12345,0x7f800001,rng.getrandbits(32)]),names[i],''))
            else:nodes.append(string(i,rng.choice(['fixture/texture','[0 1 2]','[0 1 2 3]','[1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1]',''])))
        add(nodes,(rng.choice([0,0x80,0x100,0x180]),rng.choice([0,1,-1]),rng.choice([0,1,-1]),rng.choice([91,92,95]),rng.randrange(2)),mode=rng.randrange(8)|16,repeats=2,label='varied_real_variable_states')
    def packet(c):return struct.pack('<12I',*c['q'])+b''.join(text(s) for s in c['names']+c['flags'])+b''.join(struct.pack('<2I',t,b)+text(n)+text(s) for t,b,n,s in c['nodes'])+struct.pack('<6I',c['repeats'],*[n&0xffffffff for n in c['settings']])
    query=run/'queries.bin';query.write_bytes(b''.join(packet(c) for c in cases));outputs=[];executables=[];logs={}
    absent=run/'no-engine.dll';absent_dir=run/'no-libraries';require(not absent.exists() and not absent_dir.exists(),'Owned reference independence')
    for tag,arch,machine in [('native','x86',0x14c),('owned','x64',0x8664)]:
        exe=OUT/f'concrete_{tag}_{arch}.exe';b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0]
        require(struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==(0x10b if machine==0x14c else 0x20b) and b'asInvoker' in b,'Executable architecture/manifest')
        require(not re.search(r'\b(warning|error)\b',(OUT/(tag+'-build.log')).read_text(),re.I),'Clean compile '+tag)
        imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout
        modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require(set(m.lower() for m in modules)=={'kernel32.dll'},'Direct imports '+tag)
        output=run/(tag+'.bin');logs[tag]=execute(exe,[PRIVATE if tag=='native' else absent,META/'copied-libraries' if tag=='native' else absent_dir,query,output],run,tag)
        outputs.append(output.read_bytes());executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules))
    records=[decode(b,len(cases)) for b in outputs];differences=[dict(case=i,group=labels[i],input=cases[i],native=a,owned=b) for i,(a,b) in enumerate(zip(*records)) if a!=b]
    (run/'comparison.json').write_text(json.dumps(dict(cases=len(cases),differences=differences[:20],total_differences=len(differences)),indent=2)+'\n')
    require(not differences and outputs[0]==outputs[1],'Concrete comparison; see comparison.json')
    native=records[0];routes=Counter(d['route'] for d in native);require(set(routes)=={0,1},'Both actual shader routes')
    baseline=native[plain]['values'];require(baseline[111]['words'][3]==0x447a0000 and baseline[9]['text']=='effects/flashlight001','Independent baseline constants')
    require(baseline[68]['words'][3]==0,'Disabled emissive receives generic float zero')
    effect_case=next(i for i,c in enumerate(cases) if labels[i]=='all_optional_effect_combinations' and [x[1] for x in c['nodes'][:3]]==[1,1,1] and c['settings'][4]==0 and c['nodes'][3][1]==0)
    v=native[effect_case]['values'];require(v[86]['words'][3:7]==[0,0,0,0x38d1b717] and v[90]['words'][3:7]==[0,0,0,0],'Flesh actual defaults differ from metadata defaults')
    require(v[67]['words'][3:7]==[0x3de147ae,0x3dfdf3b6,0,0] and v[75]['words'][3:7]==[0x3f800000]*4,'Emissive/cloak vectors')
    analytical=execute(Path(executables[1]['path']),['--test'],run,'analytical');require('refusals=5' in analytical,'Owned transactional safety')
    for p,h in receipt['sources'].items():require(sha(Path(p))==h,'Prior completion source changed')
    for p,h in shared['sources_sha256'].items():require(sha(ROOT/'work'/p)==h,'Prior shared source changed')
    for p,h in protected.items():require(sha(Path(p))==h,'Original changed')
    require(sha(STD)==sha(META/'copied-libraries/stdshader_dx9.dll')==STD_PIN,'Shader original/copy changed')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());graph=set()
    def visit(p):
        key=p.relative_to(ROOT).as_posix()
        if key in graph:return
        graph.add(key)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            if (p.parent/inc).is_file():visit(p.parent/inc)
    visit(ROOT/'work/probe_vulkan_win32_surface_x64.cpp')
    require(graph==set(build['editor_source_dependencies']),'Recomputed editor dependency graph changed')
    require(all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()),'Editor dependencies changed');require(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor changed')
    require(not re.search(r'\b(warning|error)\b',(OUT/'asm-build.log').read_text(),re.I),'Clean assembler')
    sources={};pending=[ROOT/'work'/n for n in ['vertexlit_concrete_init_candidate.h','vertexlit_parameter_schema_candidate.h','vertexlit_concrete_native_fixture.h','probe_vertexlit_concrete.cpp','build_vertexlit_concrete.cmd','verify_vertexlit_concrete.py']]
    while pending:
        p=pending.pop()
        if str(p) in sources:continue
        sources[str(p)]=sha(p)
        if p.suffix in ['.h','.cpp']:
            for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(),re.M):
                if (p.parent/inc).is_file():pending.append(p.parent/inc)
    sources[str(ROOT/'work/material_trig_x87_candidate.inc')]=sha(ROOT/'work/material_trig_x87_candidate.inc')
    capture=run/'exact-records.bin.zlib';capture.write_bytes(zlib.compress(outputs[0],9));require(zlib.decompress(capture.read_bytes())==outputs[0],'Evidence roundtrip')
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),cases=len(cases),stage_invocations=sum(c['repeats'] for c in cases),groups=dict(Counter(labels)),routes=dict(routes),exact_bytes=len(outputs[0]),differences=0,output_sha256=hashlib.sha256(outputs[0]).hexdigest(),compressed_output_sha256=sha(capture),query_sha256=sha(query),executables=executables,analytical=analytical,native_log=logs['native'],sources=sources,static_ranges=evidence,registry_sha256=sha(registry),protected_original_files=len(protected),editor_dependencies_unchanged=len(build['editor_source_dependencies']),editor_sha256=build['executable_sha256'],limits=['Native collector uses actual original ShaderDLL004 VertexLitGeneric object/descriptors; material-system stage invokes real wrapper/concrete/shared/Phong/cloak/emissive/flesh initialization and real original CMaterialVar methods/resources/destruction','Explicit hardware/config/CVars/command line, owner/allocator/immediate queue and empty logging services; normal private stdshader/tier0 DLL loading/relocation, no instruction patches; executable sections compared after normal loader initialization','Owned x64 uses compiled functional125-name/type schema and reconstructed value behavior, not original libraries. Albedo GetString conversions from non-string values refuse; no nonnull material/texture resources or arbitrary FourCC objects','Shader-family fallback selection, shader resource Init/texture loading, material proxies, actual IMaterial object construction, effective patch VMT composition, GPU/render fidelity and editor integration remain UNVERIFIED','Twelve accepted model-declaration inputs reused under four settings states, not a full native VMT-file loader or screenshot comparison. No live SFM/computer use, original writes, GPU or Blender; full application remains incomplete'])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(path=str(run),receipt_sha256=sha(run/'results.json')),indent=2)+'\n')
    # Keep one losslessly compressed exact record instead of two identical
    # large disposable output files. Check bytes and the resolved directory.
    for name in ['native.bin','owned.bin']:
        p=run/name;require(p.resolve().parent==run.resolve() and p.read_bytes()==zlib.decompress(capture.read_bytes()),'Disposable duplicate output identity');p.unlink()
    print(json.dumps({k:result[k] for k in ['status','cases','stage_invocations','routes','exact_bytes','analytical']},indent=2))
if __name__=='__main__':main()
