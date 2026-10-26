"""Complete original CMaterial parameter loop vs owned pre-texture orchestration.

Supported concrete schemas: VertexLitGeneric, Wireframe, Wireframe_DX9.
External $fallbackmaterial files, other concrete shaders and patches refuse.
Owned include/base resolution is separately composed, not a claim of native
CMaterial file loading. All original files stay read-only; no SFM/GPU access.
"""
from pathlib import Path
from datetime import datetime,timezone
from collections import Counter
import hashlib,itertools,json,random,re,struct,subprocess,tempfile,zlib
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
from verify_shader_parameter_registry import decode as decode_registry
from verify_material_keyvalues_parser import text,DUMP
from verify_ifm_draw_packet import check_ranges

OUT=ROOT/'outputs/material-parameter-orchestration-20261026'
META=ROOT/'outputs/material-effective-parameters-20261017/verification-l11z2le6'
STD=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/bin/stdshader_dx9.dll')
STD_PIN='fb09e2ddc8a99aff4ef31e3fddfdb1ca84d89379336abdab4e76e94adfa7e6f8'

def execute(exe,args,run,label):
    p=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=120)
    (run/(label+'.log')).write_text(p.stdout+p.stderr)
    require(p.returncode==0,'Execution '+label+' exit='+str(p.returncode)+': '+p.stdout+p.stderr)
    return p.stdout.strip()

def decode(raw,count,files=False):
    at=0;results=[]
    def word():
        nonlocal at
        n=struct.unpack_from('<I',raw,at)[0];at+=4;return n
    def string():
        nonlocal at
        n=word();s=raw[at:at+n].decode('ascii');at+=n;return s
    for _ in range(count):
        dependencies=[string() for _ in range(word())] if files else []
        selected,n,lookups=struct.unpack_from('<3I',raw,at);at+=12;shader=raw[at:at+n].decode('ascii');at+=n;names=[string() for _ in range(lookups)]
        count,warnings=struct.unpack_from('<2I',raw,at);at+=8;values=[]
        for _ in range(count):
            q=struct.unpack_from('<28I',raw,at);at+=112;s=raw[at:at+q[10]];at+=q[10];name=raw[at:at+q[11]];at+=q[11];values.append(dict(words=list(q),text=s.decode('ascii'),name=name.decode('ascii')))
        record=dict(selected=selected,shader=shader,lookups=names,warnings=warnings,values=values)
        if files:record['dependencies']=dependencies
        results.append(record)
    require(at==len(raw),'Exact loop record extent');return results

def node(key,children=(),kind=0,bits=0,value=''):return dict(key=key,children=list(children),kind=kind,bits=bits&0xffffffff,text=value)
def integer(key,value):return node(key,kind=2,bits=value)
def string(key,value):return node(key,kind=1,value=value)
def packet(n):return struct.pack('<2I',n['kind'],n['bits'])+text(n['key'])+text(n['text'])+struct.pack('<I',len(n['children']))+b''.join(packet(c) for c in n['children'])

def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    prior=json.loads((ROOT/'outputs/shader-fallback-20261026/summary.json').read_text());accepted=json.loads(Path(prior['receipt']).read_text())
    for p,h in accepted['sources'].items():require(sha(Path(p))==h,'Prior fallback source '+p)
    concrete=json.loads((ROOT/'outputs/vertexlit-concrete-20261026/summary.json').read_text());concrete_receipt=json.loads(Path(concrete['receipt']).read_text())
    for p,h in concrete_receipt['sources'].items():require(sha(Path(p))==h,'Prior concrete source '+p)
    numeric=json.loads((ROOT/'outputs/material-keyvalues-numeric-20261026/summary.json').read_text());protected=json.loads(Path(numeric['raw_receipt']).read_text())['original_hashes']
    for p,h in protected.items():require(sha(Path(p))==h,'Protected original '+p)
    require(sha(LIB)==sha(PRIVATE)==PIN and sha(STD)==sha(META/'copied-libraries/stdshader_dx9.dll')==STD_PIN,'Pinned libraries')
    _,evidence=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('complete_original_CMaterial_parameter_loop',0x10006f10,0x10007355),('complete_hardware_override_selection',0x10003590,0x100038cc),('complete_qualified_lookup_helper',0x10003530,0x1000358f),('complete_FindKey',0x10070a80,0x10070bfc),('complete_SetChain',0x100705e0,0x100705ed)])
    image,shader_evidence=check_ranges(STD,'stdshader_dx9-disassembly.txt',[
        ('complete_empty_alias_InitParams',0x1000f6e0,0x1000f6e3),('complete_Wireframe_DX9_InitParams',0x1005f880,0x1005f90c)])
    registry=decode_registry((META/'registry-0.bin').read_bytes());wire=next(r for r in registry if r['name']=='Wireframe_DX9')
    require(len(wire['parameters'])==15 and [p['name'] for p in wire['parameters'][13:]]==['$DISPLACEMENTMAP','$DISPLACEMENTWRINKLE'],'Wireframe concrete schema')
    executables=[]
    for tag,arch,machine in [('native','x86',0x14c),('owned','x64',0x8664)]:
        exe=OUT/f'orchestration_{tag}_{arch}.exe';b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0]
        require(struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==(0x10b if machine==0x14c else 0x20b) and b'asInvoker' in b,'Architecture/manifest')
        require(not re.search(r'\b(warning|error)\b',(OUT/(tag+'-build.log')).read_text(),re.I),'Clean compiler '+tag)
        imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require(set(x.lower() for x in modules)=={'kernel32.dll'},'Direct imports '+tag)
        executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules))
    cases=[];labels=[];default=[0,0,0,0,0,0,0,0,0,0,92,0]
    def add(root,state=None,label='controlled'):
        # Conservative private original query guard prevents its unchecked
        # temporary table and final one-byte count from overflowing.
        rows=len(root['children']);block_rows=max([len(c['children']) for c in root['children']]+[0]);known=125 if root['key'].lower()=='vertexlitgeneric' else 15
        require(known+rows+block_rows<=255,'Safe final native count domain')
        cases.append((default[:] if state is None else list(state),root));labels.append(label)
    for shader in ['Wireframe','Wireframe_DX9','VertexLitGeneric','Missing_Fixture_Shader','wireframe']:
        add(node(shader),label='registered_alias_unknown_and_case')
    conditions=['GPU<1','GPU<2','GPU>=1','GPU>=2','<DX90','<DX95','<DX90_20b','>=DX90_20b','<=DX90','>=DX90','>DX90','hdr_dx9','hdr','ldr','srgb','dx9']
    base=[integer('$model',1),integer('$x',1),string('$basetexture','fixture/base')]
    for shader,gpu,level,hdr,srgb in itertools.product(['Wireframe','VertexLitGeneric'],[-1,0,1,2,3],[89,90,91,92,94,95],[0,1],[0,1,0x100]):
        state=default[:];state[0]=gpu;state[1]=hdr;state[2]=srgb;state[10]=level
        blocks=[node(c,[integer('$x',i+10),integer('$model',i%2)]) for i,c in enumerate(conditions)]
        add(node(shader,base+blocks),state,'hardware_priority_all_blocks')
    for shader,condition in itertools.product(['Wireframe','VertexLitGeneric'],conditions):
        for qualified in [False,True]:
            key=shader+'_'+condition if qualified else condition
            for empty in [False,True]:
                add(node(shader,base+[node(key,[] if empty else [integer('$x',7)])]),label='qualified_and_empty_overrides')
    for initial,override in itertools.product([0,1],repeat=2):
        for hardware_block in [False,True]:
            children=[integer('$model',initial),integer('$x',2),node('Wireframe_DX9',[integer('$model',override),integer('$x',8)])]
            if hardware_block:children.append(node('GPU<1',[integer('$x',9)]))
            add(node('Wireframe',children),label='alias_model_carry_and_block_retention')
    rng=random.Random(2026102610)
    for _ in range(384):
        shader=rng.choice(['Wireframe','Wireframe_DX9','VertexLitGeneric']);children=[]
        for i in range(rng.randrange(1,15)):
            key=rng.choice(['$model','$x','$y','$color','$alpha','$basetexture','$bumpmap','$envmap','$envmapmask','$phong','!360?$x','hdr?$x','%tool'])
            if rng.randrange(2):children.append(integer(key,rng.choice([0,1,-1,7])))
            else:children.append(string(key,rng.choice(['fixture/path','[1 2 3]','[1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1]',''])))
        for c in rng.sample(conditions,rng.randrange(4)):children.append(node(c,[integer('$x',rng.randrange(20)),integer('$model',rng.randrange(2))]))
        state=default[:];state[0]=rng.randrange(4);state[1]=rng.randrange(2);state[2]=rng.choice([0,1,256]);state[4]=rng.randrange(2);state[8]=rng.randrange(2);state[9]=rng.randrange(2);state[10]=rng.choice([89,91,92,95]);state[11]=rng.randrange(2)
        add(node(shader,children),state,'varied_whole_caller')
    add(node('VertexLitGeneric',[integer('extra'+str(i),i) for i in range(130)]),label='exact255_final_variables')
    query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<12I',*[v&0xffffffff for v in state])+packet(root) for state,root in cases));outputs=[];logs={}
    absent=run/'no-engine.dll';absent_dir=run/'no-libraries'
    for i,tag in enumerate(['native','owned']):
        p=run/(tag+'.bin');logs[tag]=execute(Path(executables[i]['path']),[PRIVATE if i==0 else absent,META/'copied-libraries' if i==0 else absent_dir,query,p],run,tag);outputs.append(p.read_bytes())
    native,owned=[decode(b,len(cases)) for b in outputs]
    differences=[dict(case=i,label=labels[i],input=cases[i],native=a,owned=b) for i,(a,b) in enumerate(zip(native,owned)) if a!=b]
    (run/'comparison.json').write_text(json.dumps(dict(cases=len(cases),total_differences=len(differences),differences=differences[:12]),indent=2)+'\n');require(not differences and outputs[0]==outputs[1],'Whole caller comparison; see comparison.json')
    require(len(native[-1]['values'])==255,'Exact final count boundary')
    require(native[0]['lookups']==['Wireframe','Wireframe_DX9'] and native[3]['shader']=='Wireframe_DX9','Alias and error default')
    # Compose the accepted owned include/base reader with this loop, then feed
    # its resolved first roots to the actual original loop for exact comparison.
    file_cases=[];file_sources=[]
    catalogs=[{'root.vmt':'#base base.vmt Wireframe{$x 1}','base.vmt':'Base{$model 1 $alpha .5}'},
              {'root.vmt':'#include extra.vmt Wireframe{$x 1}','extra.vmt':'Extra{$unused 2}'},
              {'root.vmt':'#base base.vmt VertexLitGeneric{GPU<1{$alpha .25}}','base.vmt':'Base{$basetexture fixture/base $bumpmap fixture/bump $phong 1}'}]
    for catalog in catalogs:
        for level in [91,92]:state=default[:];state[8]=1;state[10]=level;file_cases.append((state,catalog,'root.vmt'))
    benchmark=json.loads((ROOT/'outputs/material-string-value-20261025/verification-zkqa42up/results.json').read_text())['authored_material_tokens']
    for m in benchmark:
        p=Path(m['path']);require(sha(p)==m['sha256'],'Protected benchmark material');file_sources.append(m)
        for phong,level in [(0,92),(1,91),(1,92)]:state=default[:];state[8]=phong;state[10]=level;file_cases.append((state,{'root.vmt':p.read_text(encoding='ascii')},'root.vmt'))
    files_input=run/'files-input.bin';files_input.write_bytes(b''.join(struct.pack('<12I',*state)+struct.pack('<I',len(catalog))+b''.join(text(k)+text(v) for k,v in catalog.items())+text(path) for state,catalog,path in file_cases))
    files_out=run/'files-owned.bin';projected=run/'files-projected.bin';execute(Path(executables[1]['path']),['--files',files_input,files_out,projected],run,'files-owned')
    files_native=run/'files-native.bin';execute(Path(executables[0]['path']),[PRIVATE,META/'copied-libraries',projected,files_native],run,'files-native')
    f_owned=decode(files_out.read_bytes(),len(file_cases),True);f_native=decode(files_native.read_bytes(),len(file_cases))
    require([{k:v for k,v in d.items() if k!='dependencies'} for d in f_owned]==f_native,'Resolved file parameter composition')
    require(f_owned[0]['dependencies']==['root.vmt','base.vmt'] and f_owned[2]['dependencies']==['root.vmt','extra.vmt'],'Ordered file dependencies')
    analytical=execute(Path(executables[1]['path']),['--test'],run,'analytical');require('analytical_checks=11 refusals=9' in analytical,'Transactional safety refusals')
    for p,h in accepted['sources'].items():require(sha(Path(p))==h,'Accepted fallback source changed')
    for p,h in concrete_receipt['sources'].items():require(sha(Path(p))==h,'Concrete source changed')
    for p,h in protected.items():require(sha(Path(p))==h,'Original changed')
    for m in file_sources:require(sha(Path(m['path']))==m['sha256'],'Benchmark changed')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());graph=set()
    def visit(p):
        key=p.relative_to(ROOT).as_posix()
        if key in graph:return
        graph.add(key)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            if (p.parent/inc).is_file():visit(p.parent/inc)
    visit(ROOT/'work/probe_vulkan_win32_surface_x64.cpp');require(graph==set(build['editor_source_dependencies']) and all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()),'Editor graph changed');require(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor changed')
    require(not re.search(r'\b(warning|error)\b',(OUT/'asm-build.log').read_text(),re.I),'Assembler diagnostics')
    sources={};pending=[ROOT/'work'/n for n in ['material_parameter_orchestration_candidate.h','material_parameter_orchestration_native_fixture.h','probe_material_parameter_orchestration.cpp','build_material_parameter_orchestration.cmd','verify_material_parameter_orchestration.py']]
    while pending:
        p=pending.pop()
        if str(p) in sources:continue
        sources[str(p)]=sha(p)
        if p.suffix in ['.h','.cpp']:
            for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(),re.M):
                if (p.parent/inc).is_file():pending.append(p.parent/inc)
    sources[str(ROOT/'work/material_trig_x87_candidate.inc')]=sha(ROOT/'work/material_trig_x87_candidate.inc')
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),cases=len(cases),groups=dict(Counter(labels)),exact_bytes=len(outputs[0]),differences=0,query_sha256=sha(query),output_sha256=hashlib.sha256(outputs[0]).hexdigest(),file_composition_cases=len(file_cases),file_sources=file_sources,file_projected_query_sha256=sha(projected),file_native_output_sha256=sha(files_native),file_owned_output_sha256=sha(files_out),analytical=analytical,executables=executables,native_log=logs['native'],sources=sources,static_ranges=evidence,shader_ranges=shader_evidence,protected_original_files=len(protected),editor_dependencies_unchanged=len(graph),editor_sha256=build['executable_sha256'],limits=['Complete original CMaterial10006F10 executes for supported VertexLitGeneric/Wireframe/Wireframe_DX9, including actual condition selector/FindKey/SetChain, registry objects/InitParams/GetFallback, collector, variable setters/intermediate cleanup/final array allocation and one-byte count','CMaterial storage/vtable, registry lookup dispatch, key-name symbol provider, allocator, hardware/config/tools/lock/logging services are controlled. This is not a normally constructed public IMaterial object or full filesystem/resource initialization','Shader support deliberately explicit; known other concrete shaders, patch roots in file wrapper, nonempty external fallback-material keys and final counts above255 refuse with output preservation. No substitute shader is used for known unsupported families','Native reference uses whole read-only source-tree copies, mutating private chain links; owned API leaves caller tree unchanged and models chain inheritance without writing it','Owned include/base resolution plus full-loop composition uses projected resolved roots; not a dynamically verified native full VMT file/patch/fallback-file loader or mounted filesystem policy','No texture/resource Init, proxies, renderer/editor integration, live SFM/computer use, original writes, GPU or Blender. Application remains incomplete'])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(path=str(run),receipt_sha256=sha(run/'results.json')),indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ['status','cases','exact_bytes','file_composition_cases','analytical']},indent=2))
if __name__=='__main__':main()
