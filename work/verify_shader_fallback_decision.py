"""All157 actual registered GetFallback methods vs owned x64 decisions.

Single decision only. The material caller initializes variables before asking;
aliases cannot be blindly chained in place of recollection/initialization/VMT
fallback handling. No live SFM, original writes, GPU or renderer integration.
"""
from pathlib import Path
from datetime import datetime,timezone
from collections import Counter
import hashlib,itertools,json,random,re,struct,subprocess,tempfile
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha
from verify_shader_parameter_registry import decode as decode_registry
from verify_material_keyvalues_parser import DUMP
from verify_ifm_draw_packet import check_ranges

OUT=ROOT/'outputs/shader-fallback-20261026'
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
        present,n,c,metadata=struct.unpack_from('<4I',raw,at);at+=16;name=raw[at:at+n].decode('ascii');at+=n
        calls=list(struct.unpack_from('<'+str(c)+'I',raw,at));at+=4*c
        require(present<=1 and (present or not n) and c<=2,'Decision output domain')
        rows.append(dict(fallback=name if present else None,hardware_queries=calls,base_metadata=metadata))
    require(at==len(raw),'Exact decision output extent');return rows

def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Run '+str(run),flush=True)
    concrete=json.loads((ROOT/'outputs/vertexlit-concrete-20261026/summary.json').read_text());prior=json.loads(Path(concrete['receipt']).read_text())
    for p,h in prior['sources'].items():require(sha(Path(p))==h,'Concrete source unchanged '+p)
    numeric=json.loads((ROOT/'outputs/material-keyvalues-numeric-20261026/summary.json').read_text());protected=json.loads(Path(numeric['raw_receipt']).read_text())['original_hashes']
    for p,h in protected.items():require(sha(Path(p))==h,'Protected original '+p)
    require(sha(LIB)==sha(PRIVATE)==PIN and sha(STD)==sha(META/'copied-libraries/stdshader_dx9.dll')==STD_PIN,'Pinned original/private libraries')
    registry_receipt=json.loads((META/'results.json').read_text());rex=registry_receipt['executable'];require(sha(Path(rex['path']))==rex['sha256'],'Registry executable')
    for name in ['tier0.dll','stdshader_dx9.dll']:
        original=next(p for p in registry_receipt['original_hashes'] if Path(p).name==name);require(sha(Path(original))==sha(META/'copied-libraries'/name)==registry_receipt['original_hashes'][original],'Registry library identity')
    registry=run/'registry.bin';execute(Path(rex['path']),[META/'copied-libraries',registry],run,'registry');require(sha(registry)==registry_receipt['output_sha256'],'Fresh registry identity')
    records=decode_registry(registry.read_bytes());require(len(records)==157,'Registry count')
    source=(ROOT/'work/shader_fallback_decision_candidate.h').read_text();names=re.findall(r'^        \{"([^"]+)"',source,re.M);require(names==[s['name'] for s in records],'Compiled functional registered names')
    conditional=[r for r in records if not r['literal_fallback_verified']];require(len(conditional)==15,'Conditional inventory')
    ranges=[('level92',0x10013830,0x10013850),('capability24_95',0x1002e960,0x1002e980),('monitor_real_base_defined',0x100317d0,0x100317f7),('level90',0x10035450,0x10035470),('proto_ice',0x1003dd70,0x1003ddb8),('prototype',0x1003e6f0,0x1003e721),('level95',0x10041f70,0x10041f90),('water_full_return_word',0x1005dda0,0x1005ddbe)]
    image,evidence=check_ranges(STD,'stdshader_dx9-disassembly.txt',ranges)
    require({0x10000000+r['methods'][1] for r in conditional}=={a for _,a,_ in ranges},'All distinct conditional methods covered')
    for r in records:
        if not r['literal_fallback_verified']:continue
        o=image.offset(0x10000000+r['methods'][1]);b=image.b[o:o+8]
        if r['literal_fallback'] is None:require(b[:5]==bytes.fromhex('33c0c20400'),'Literal null body')
        else:require(b[0]==0xb8 and b[5:8]==bytes.fromhex('c20400') and image.string(struct.unpack_from('<I',b,1)[0])==r['literal_fallback'],'Literal alias body')
    _,caller=check_ranges(LIB,'materialsystem-disassembly.txt',[('complete_CMaterial_shader_parameter_fallback_loop_static',0x10006f10,0x10007355)])
    # Specific unmodified sequence in the complete caller, NOT a dynamic claim
    # of normal CMaterial construction or its file/error/fallback-block branches.
    ins={int(x['address']):x['instruction'] for x in caller[0]['instructions']}
    require(ins[0x100070ae]=='call        10005940' and ins[0x100070ee]=='call        eax' and ins[0x100070fe]=='call        edx','Collection -> InitParams -> GetFallback ordering')
    levels=[-2147483648,-1,0,89,90,91,92,94,95,96,2147483647];return_words=[0,1,0xff,0x100,0x80000000,0xffffffff]
    inputs=[(0,0,''),(1,0,''),(1,0,'fixture/base'),(1,0,'0'),(1,0,'[1 2 3]'),(1,0,'[1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1]'),(1,0,'center .5 .5 scale 2 3 rotate 90 translate 0 0'),(1,0,'[bad]'),(2,0,''),(2,0xffffffff,''),(3,0,''),(3,0x7f800001,''),(7,0,'')]
    cases=[];labels=[]
    def add(index,first=92,second=92,cap=95,hdr=0,config=1,byte=0,params=1,base=(1,0,'fixture/base'),label='controlled'):
        t,b,s=base;cases.append(([index,first&0xffffffff,second&0xffffffff,cap&0xffffffff,hdr,config,byte,params,t,b,len(s)],s));labels.append(label)
    rng=random.Random(2026102609)
    for r in records:
        for i in range(12):add(r['index'],rng.choice(levels),rng.choice(levels),rng.choice(levels),return_words[i%6],i%2,[0,1,128,255][i%4],i%2,inputs[i%len(inputs)],'all_registered_methods')
    for r in conditional:
        for first,second in itertools.product(levels,repeat=2):
            for i,(present,byte) in enumerate([(0,0),(0,255),(1,0),(1,1),(1,128),(1,255)]):
                add(r['index'],first,second,levels[(i+levels.index(first))%len(levels)],return_words[i],present,byte,i%2,inputs[(i+levels.index(second))%len(inputs)],'conditional_cross_product')
    monitor=names.index('MonitorScreen_DX9')
    for kind in range(256):
        if kind==5:continue
        for bits in [0,1,0x80000000,0x7f800001]:add(monitor,base=(kind,bits,'fixture/base'),label='monitor_all_admitted_KeyValues_types')
    for inp in inputs:
        for present in [0,1]:add(monitor,params=present,base=inp,label='monitor_actual_variable_kinds')
    water=names.index('Water_DX9_HDR')
    for n in range(256):
        for word in [n,n<<8,n<<24]:add(water,hdr=word,label='water_whole_word_nonzero')
    query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<11I',*q)+s.encode('ascii') for q,s in cases));outputs=[];executables=[];native_log=''
    absent=run/'no-engine.dll';absent_dir=run/'no-libraries';require(not absent.exists() and not absent_dir.exists(),'Owned library absence')
    for label,arch,machine in [('native','x86',0x14c),('owned','x64',0x8664)]:
        exe=OUT/f'fallback_{label}_{arch}.exe';b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0]
        require(struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==(0x10b if machine==0x14c else 0x20b) and b'asInvoker' in b,'Architecture/manifest')
        require(not re.search(r'\b(warning|error)\b',(OUT/(label+'-build.log')).read_text(),re.I),'Clean compiler '+label)
        imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout
        modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require(set(x.lower() for x in modules)=={'kernel32.dll'},'Direct imports '+label)
        result=run/(label+'.bin');message=execute(exe,[PRIVATE if label=='native' else absent,META/'copied-libraries' if label=='native' else absent_dir,query,result],run,label)
        if label=='native':native_log=message
        outputs.append(result.read_bytes());executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules))
    native,owned=[decode(b,len(cases)) for b in outputs]
    differences=[dict(case=i,group=labels[i],input=cases[i],native=a,owned=b) for i,(a,b) in enumerate(zip(native,owned)) if a!=b]
    (run/'comparison.json').write_text(json.dumps(dict(cases=len(cases),total_differences=len(differences),differences=differences[:20]),indent=2)+'\n')
    require(not differences and outputs[0]==outputs[1],'Fallback comparison; see comparison.json')
    require({c[0][0] for c in cases}==set(range(157)),'Every registered shader executed')
    # Independent decision and exact short-circuit expectations.
    for (q,_),actual in zip(cases,native):
        r=records[q[0]];calls=[];want=None;method=r['methods'][1];signed=lambda n:struct.unpack('<i',struct.pack('<I',n))[0]
        if r['literal_fallback_verified']:want=r['literal_fallback']
        elif method in [0x35450,0x13830,0x41f70]:
            calls=[17];threshold={0x35450:90,0x13830:92,0x41f70:95}[method];want='Wireframe' if signed(q[1])<threshold else None
        elif method==0x2e960:calls=[24];want='Wireframe' if signed(q[3])<95 else None
        elif method==0x5dda0:calls=[34];want='WATER_DX90' if q[4]==0 else None
        elif method==0x317d0:want='LightmappedGeneric' if q[7] and actual['base_metadata']&15==6 else None
        else:
            calls=[17]
            if signed(q[1])<90:want='Wireframe'
            else:
                calls.append(17)
                if signed(q[2])<92:want='Wireframe'
                elif method==0x3dd70 and q[5] and q[6]:want='VertexLitGeneric'
        require(actual['fallback']==want and actual['hardware_queries']==calls,'Independent method expectation')
    kinds=Counter(d['base_metadata']&15 for d in native if d['base_metadata']!=0xffffffff);require({0,1,2,4,6,7}<=set(kinds),'Real material variable kind coverage')
    analytical=execute(Path(executables[1]['path']),['--test'],run,'analytical');require('analytical_checks=12 refusals=2' in analytical,'Owned analytical coverage')
    for p,h in prior['sources'].items():require(sha(Path(p))==h,'Concrete source changed')
    for p,h in protected.items():require(sha(Path(p))==h,'Protected original changed')
    require(sha(STD)==sha(META/'copied-libraries/stdshader_dx9.dll')==STD_PIN,'Original shader changed')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());graph=set()
    def visit(p):
        key=p.relative_to(ROOT).as_posix()
        if key in graph:return
        graph.add(key)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            if (p.parent/inc).is_file():visit(p.parent/inc)
    visit(ROOT/'work/probe_vulkan_win32_surface_x64.cpp');require(graph==set(build['editor_source_dependencies']) and all(sha(ROOT/p)==h for p,h in build['editor_source_dependencies'].items()),'Recomputed editor graph changed')
    require(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor changed')
    require(not re.search(r'\b(warning|error)\b',(OUT/'asm-build.log').read_text(),re.I),'Clean assembler')
    sources={};pending=[ROOT/'work'/n for n in ['shader_fallback_decision_candidate.h','shader_fallback_native_fixture.h','probe_shader_fallback_decision.cpp','build_shader_fallback_decision.cmd','verify_shader_fallback_decision.py']]
    while pending:
        p=pending.pop()
        if str(p) in sources:continue
        sources[str(p)]=sha(p)
        if p.suffix in ['.h','.cpp']:
            for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(),re.M):
                if (p.parent/inc).is_file():pending.append(p.parent/inc)
    sources[str(ROOT/'work/material_trig_x87_candidate.inc')]=sha(ROOT/'work/material_trig_x87_candidate.inc')
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),cases=len(cases),registered_shaders=157,conditional_shaders=15,unique_conditional_methods=8,groups=dict(Counter(labels)),exact_bytes=len(outputs[0]),differences=0,query_sha256=sha(query),output_sha256=hashlib.sha256(outputs[0]).hexdigest(),executables=executables,analytical=analytical,native_log=native_log,base_value_types=dict(kinds),outcomes=dict(Counter(d['fallback'] or '<null>' for d in native)),hardware_query_counts=dict(Counter(n for d in native for n in d['hardware_queries'])),static_ranges=evidence,static_complete_material_caller=caller,sources=sources,protected_original_files=len(protected),editor_dependencies_unchanged=len(graph),editor_sha256=build['executable_sha256'],registry_sha256=sha(registry),limits=['All157 actual original registered shader GetFallback methods execute, including15 conditional shaders/eight unique conditional bodies; only an individual post-initialization decision is proved','Real original pooled CMaterialVars/name interning and unchanged collector supply valid MonitorScreen parameter inputs; all40 object bytes checked unchanged across calls; string/matrix/scalar/Undefined values covered, not live nonnull textures/materials','Hardware slot17 returns may differ between its two calls; slot24 and slot34 are independently supplied raw service returns. Live hardware discovery and slot24 semantic identity remain UNVERIFIED; config pointer and byte38 explicit','Complete CMaterial caller is statically verified to collect, InitParams, GetFallback, clean intermediates and handle blocks/file fallbacks; that complete function and normal CMaterial construction are NOT dynamically verified here','No blind alias-chain resolver is provided. Every next shader needs its own collected/initialized values; overrides/model carry and fallback material file paths require separate composition','Private normal library loading/relocation with original executable sections unchanged; explicit allocator/owner/name/hardware/config services. Owned x64 runs with absent original-library arguments and only KERNEL32 direct imports','No editor integration/build/run, original writes, live SFM/computer use, GPU or Blender. Full project/rendering fidelity remains incomplete'])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(path=str(run),receipt_sha256=sha(run/'results.json')),indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ['status','cases','registered_shaders','conditional_shaders','exact_bytes','analytical','outcomes']},indent=2))
if __name__=='__main__':main()
