"""Whole concrete VertexLit pass dispatcher with declared leaf observers."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, itertools, json, random, re, struct, subprocess, sys, tempfile, time
from verify_material_texture_file_resolver import closure
from verify_ifm_draw_packet import check_ranges

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT/'outputs/concrete-vertexlit-draw-reference-20261028'
STD = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\stdshader_dx9.dll')
DUMP = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')

def sha(p):
    with Path(p).open('rb') as f:
        return hashlib.file_digest(f,'sha256').hexdigest()

def need(b,s):
    if not b:
        raise RuntimeError(s)

def bits(v):
    return struct.unpack('<I',struct.pack('<f',v))[0]

def asfloat(v):
    return struct.unpack('<f',struct.pack('<I',v))[0]

def opaque(v):
    factor=asfloat(v)
    clamped=1.0 if factor>1.0 else 0.0 if factor<0.0 else factor
    remainder=asfloat(bits(1.0-asfloat(bits(clamped*asfloat(0x3faccccd)))))
    return remainder<0.0 or remainder<=asfloat(0x3ecccccd)

def digest(words):
    h=2166136261
    for v in words:
        h=((h^v)*16777619)&0xffffffff
    return h

def corpus():
    rows=[]
    for enabled,changed,shadow,mutate,factor,strength in itertools.product(range(8),range(8),range(2),range(2),
            [bits(v) for v in [-1.0,-0.0,0.0,0.25,0.5,1.0,2.0,float('inf'),-float('inf')]]+[0x7fc01234],
            [bits(-1.0),bits(0.0),bits(1.0),0x7fc01234]):
        rows.append([len(rows),enabled,changed,shadow,factor,strength,mutate,(len(rows)*0x1234567)&0xffffffff])
    return rows

def prediction(rows,info):
    raw=bytearray();total_events=total_leaves=0
    for serial,initial,changed,shadow,factor,strength,mutate,compression in rows:
        first=0 if initial&1 and not shadow and opaque(factor) else 1
        live=changed if mutate else initial
        kinds=[first]
        f=asfloat(factor);e=asfloat(strength)
        if live&1:kinds.append(2 if shadow or 0.0<f<1.0 else 0)
        if live&2:kinds.append(3 if shadow or e>0.0 else 0)
        if live&4:kinds.append(4)
        leaves=sum(k!=0 for k in kinds)
        raw+=struct.pack('<5I',serial,len(kinds),live,len(kinds),leaves)
        for step,kind in enumerate(kinds):
            raw+=struct.pack('<8I',kind,digest(info[kind-1]) if kind else 0,len(info[kind-1]) if kind else 0,
                             step,initial if step==0 else live,shadow,compression if kind else 0,int(kind==1))
        total_events+=len(kinds);total_leaves+=leaves
    return bytes(raw),dict(cases=len(rows),events=total_events,helper_boundaries=total_leaves)

def make_mapping(image,static,run):
    metadata=ROOT/'outputs/material-effective-parameters-20261017/verification-l11z2le6/shader-metadata.json'
    family=next(x for x in json.loads(metadata.read_text()) if x['name']=='VertexLitGeneric')
    need(len(family['parameters'])==125,'registered VertexLit schema')
    bindings={image.base+p['rva']+20:i for i,p in enumerate(family['parameters']) if i>=13}
    def record(name,length):
        result=[0xffffffff]*length;registers={}
        for row in next(r for r in static if r['name']==name)['instructions']:
            t=row['instruction'];load=re.fullmatch(r'mov\s+(ecx|edx),dword ptr ds:\[([0-9A-F]+)h\]',t)
            if load:registers[load[1]]=bindings[int(load[2],16)]
            store=re.fullmatch(r'mov\s+dword ptr \[eax(?:\+([0-9A-F]+)h?)?\],(ecx|edx)',t)
            literal=re.fullmatch(r'mov\s+dword ptr \[eax(?:\+([0-9A-F]+)h?)?\],([0-9A-F]+)(h)?',t)
            if store:result[int(store[1] or '0',16)//4]=registers[store[2]]
            elif literal:result[int(literal[1] or '0',16)//4]=int(literal[2],16 if literal[3] else 10)
        return result
    main=record('whole_main_info_mapping',110)
    cloak=[bindings[a] if a else 0xffffffff for a in [0x100cae80,0x100ca910,0x100cacd4,0,0x100caee0,0x100ca628,0x100ca610]]
    emissive=[bindings[a] for a in [0x100caef8,0x100ca808,0x100ca898,0x100cac10,0x100ca6e8,0x100ca580,0x100ca538]]
    flesh=record('whole_flesh_info_mapping',20);info=[main,cloak,emissive,flesh]
    need(main[1:3]==[14,15],'COMPRESS/STRETCH mappings include native no-h offsets4/8; old static parser missed both')
    need(main[0]==6 and main[25]==9 and cloak[0]==74 and emissive[0]==68,'named base/flashlight/cloak/emissive info fields')
    raw=struct.pack('<I',len(bindings))+b''.join(struct.pack('<2I',a-image.base,i) for a,i in sorted(bindings.items()))
    for r in info:raw+=struct.pack('<I',len(r))+struct.pack('<'+str(len(r))+'I',*r)
    mapping=run/'mapping.bin';mapping.write_bytes(raw)
    historical=ROOT/'outputs/vertexlit-shared-init-20261025/verification-lpr0qmmc/info.bin'
    old=list(struct.unpack('<110I',historical.read_bytes()))
    differences=[dict(word=i,historical=a,verified=b) for i,(a,b) in enumerate(zip(old,main)) if a!=b]
    need(differences==[dict(word=1,historical=0xffffffff,verified=14),dict(word=2,historical=0xffffffff,verified=15)],'exact historical COMPRESS/STRETCH mapping omission')
    defect=dict(source=str(ROOT/'work/verify_vertexlit_shared_init.py'),source_sha256=sha(ROOT/'work/verify_vertexlit_shared_init.py'),
                historical_fixture=str(historical),historical_sha256=sha(historical),differences=differences,
                corrected_here=True,old_verifier_corrected=False,shared_initializer_reverification='PENDING',editor_runtime_impact='UNVERIFIED')
    return mapping,info,metadata,defect

def main():
    OUT.mkdir(exist_ok=True)
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    print('Verification directory: '+str(run),flush=True)
    parent=ROOT/'outputs/base-shader-entry-reference-20261028/verification-_b26ayuc/results.json'
    need(sha(parent)=='2e9136a5acb184826fccd3c02c7dc46d051dc1b50cec918951404f572fc14f19','accepted standard shader entry receipt')
    prior=json.loads(parent.read_text())
    protected={**prior['protected'],**prior['source'],str(parent):sha(parent)}
    for e in prior['executables'].values():
        protected[e['path']]=e['sha256']
    # Existing base helpers were previously tested only against owned fixtures.
    # Preserve them and their pre-existing tests while verifying real code now.
    for name in ['studio_vertex_lit_passes_candidate.h','studio_shader_cloak_gate_candidate.h','test_studio_vertex_lit_passes.cpp']:
        p=ROOT/'work'/name;protected[str(p)]=sha(p)
    originals=dict(prior['original_inputs']);originals[str(STD)]= 'fb09e2ddc8a99aff4ef31e3fddfdb1ca84d89379336abdab4e76e94adfa7e6f8'
    need(all(sha(p)==h for p,h in protected.items()),'prior source/artifacts unchanged before')
    need(all(sha(p)==h for p,h in originals.items()),'original/private inputs unchanged before')
    sources=closure([ROOT/'work/probe_concrete_vertexlit_draw_reference.cpp',ROOT/'work/build_concrete_vertexlit_draw_reference.cmd',ROOT/'work/test_studio_vertex_lit_passes.cpp',ROOT/'work/probe_private_boundary_observer_self_test.cpp',ROOT/'work/CONCRETE_VERTEXLIT_DRAW_REFERENCE.md',Path(__file__)])
    for module in tuple(sys.modules.values()):
        path=getattr(module,'__file__',None)
        if path and Path(path).resolve().parent==ROOT/'work' and Path(path).suffix=='.py':
            sources[str(Path(path).resolve())]=sha(path)
    image,static=check_ranges(STD,'stdshader_dx9-disassembly.txt',[
        ('whole_concrete_VertexLit_draw',0x100537a0,0x10053a02),
        ('whole_main_info_mapping',0x10052f10,0x100532a2),
        ('whole_flesh_info_mapping',0x10053360,0x1005341d),
        ('whole_original_cloak_gate',0x100096f0,0x100097c1),
        ('whole_original_skip_dispatch',0x10065a70,0x10065b4e),
        ('whole_original_memset',0x100671e0,0x1006725a)])
    need(image.word(0x1009c39c+6*4)==0x100652c0 and image.word(0x1009c39c+14*4)==0x100537a0,'original VertexLitGeneric entry and concrete draw target')
    (run/'static-ranges.private.json').write_text(json.dumps(static,indent=2)+'\n')
    mapping,info,metadata,defect=make_mapping(image,static,run);originals[str(metadata)]=sha(metadata)
    protected[defect['source']]=defect['source_sha256'];protected[defect['historical_fixture']]=defect['historical_sha256']
    rows=corpus();query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<8I',*r) for r in rows))
    expected,coverage=prediction(rows,info);(run/'analytical.bin').write_bytes(expected)
    started=time.time()
    built=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_concrete_vertexlit_draw_reference.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180)
    (run/'build-driver.log').write_text(built.stdout+built.stderr)
    need(built.returncode==0,'fresh original/owned/two existing regression builds')
    executables={}
    for tag,name,machine,magic,logname in [
        ('reference','concrete_vertexlit_draw_reference_x86.exe',0x14c,0x10b,'reference-build.log'),
        ('owned','probe_concrete_vertexlit_draw_reference_x64.exe',0x8664,0x20b,'probe_concrete_vertexlit_draw_reference-build.log'),
        ('pass_regression','test_studio_vertex_lit_passes_x64.exe',0x8664,0x20b,'test_studio_vertex_lit_passes-build.log'),
        ('observer_self_test','observer-self-test_x86.exe',0x14c,0x10b,'observer-self-test-build.log')]:
        exe=OUT/name;raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic and b'asInvoker' in raw,'fresh architecture/manifest '+tag)
        log=(OUT/logname).read_text();(run/logname).write_text(log)
        need(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log,re.I),'clean compile '+tag)
        inspected=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,timeout=30)
        (run/(tag+'-imports.txt')).write_text(inspected.stdout)
        dlls=sorted(set(re.findall(r'^\s+([^\s]+\.dll)\s*$',inspected.stdout,re.I|re.M)))
        need(inspected.returncode==0 and dlls==['KERNEL32.dll'],'system-only imports '+tag)
        executables[tag]=dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=dlls)
    selftest=subprocess.run([executables['observer_self_test']['path']],cwd=run,capture_output=True,text=True,timeout=30)
    (run/'observer-self-test.log').write_text(selftest.stdout+selftest.stderr)
    need(selftest.returncode==0 and 'PASS private_thread_boundaries=4 callee_bodies_skipped=4 caller_stack=INTACT disarmed_original_body=PASS' in selftest.stdout,'own-thread hardware boundary self-test')
    runs=[]
    for tag in ['reference','owned','reference','owned']:
        trace=run/(tag+'-'+str(len(runs))+'.bin')
        result=subprocess.run([executables[tag]['path'],*([str(STD)] if tag=='reference' else []),str(mapping),str(query),str(trace)],cwd=run,capture_output=True,text=True,timeout=90)
        (run/(trace.stem+'.log')).write_text(result.stdout+result.stderr)
        need(result.returncode==0,tag+' exit='+str(result.returncode)+'; inspect '+trace.stem+'.log')
        actual=trace.read_bytes()
        if actual!=expected:
            at=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
            raise RuntimeError(tag+' full trace mismatch at byte '+str(at))
        need(f'PASS concrete_vertexlit_cases={coverage["cases"]} events={coverage["events"]} helper_boundaries={coverage["helper_boundaries"]}' in result.stdout,'exact entry/live-state/nested coverage '+tag)
        runs.append(dict(tag=tag,output=str(trace),sha256=sha(trace),stdout=result.stdout.strip()))
    regressions={}
    for tag,pattern in [('pass_regression','vertex_lit_pass_cases=7168')]:
        p=subprocess.run([executables[tag]['path']],cwd=run,capture_output=True,text=True,timeout=60)
        (run/(tag+'.log')).write_text(p.stdout+p.stderr)
        need(p.returncode==0 and pattern in p.stdout,'fresh owned regression '+tag)
        regressions[tag]=p.stdout.strip()
    need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in protected.items()),'all source/prior artifacts unchanged after')
    need(all(sha(p)==h for p,h in originals.items()),'original/private inputs unchanged after')
    need(all(sha(p)==h for p,h in prior['editor_dependencies'].items()) and sha(prior['editor'])==prior['editor_sha256'],'313 editor dependencies and exact executable unchanged')
    outcome=dict(status='VERIFIED concrete VertexLit pass dispatcher with actual info/gate/skip bodies; four render-helper bodies INCOMPLETE',
        utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',parent=str(parent),parent_sha256=sha(parent),
        source=sources,protected=protected,original_inputs=originals,executables=executables,runs=runs,coverage=coverage,
        complete_trace_bytes=len(expected),query_sha256=sha(query),analytical_sha256=sha(run/'analytical.bin'),
        compiler_warnings=0,regressions=regressions,observer_self_test=selftest.stdout.strip(),mapping_sha256=sha(mapping),info_records=info,known_earlier_fixture_defect=defect,static_ranges=[{k:v for k,v in r.items() if k!='instructions'} for r in static],
        editor=prior['editor'],editor_sha256=prior['editor_sha256'],editor_dependencies=prior['editor_dependencies'],editor_rebuilt=False,
        findings=[
          'Whole original VertexLitGeneric slot14 executes through its actual unmodified vtable. Original main/flesh parameter-index builders, inline cloak/emissive records, cloak predicate, memset and skip dispatcher execute unchanged RX instructions.',
          'Main lighting1005A820, cloak10009140, emissive10015C60 and flesh1001F590 are explicitly intercepted using four hardware execution breakpoints confined to this disposable probe thread. Their original instructions DO NOT execute. These observers are declared single-pass consumers that validate supplied arguments and complete info-record contents, then optionally mutate enable flags.',
          'Observer mechanism first passes a fresh four-boundary self-test proving untouched callee bodies are skipped, caller argument stack preserved, and disabling observers restores original behavior. No code patch, remote process/thread access, privileges or SFM runtime used.',
          'Existing owned pass-control/cloak helpers match the real concrete dispatcher for main/skip choice, live enable rereads after the first pass and effect ordering. Float domain includes signed zero, quiet NaN and infinities with masked MXCSR exceptions; floating exception-state equivalence and signaling NaNs are not claimed.',
          'Actual native info builders emit all110+7+7+20 parameter-index words checked against registered metadata/static mapping. Native skips use actual dispatcher with valid borrowed instance records and controlled service callbacks. No missing-instance allocation path tested.',
          'Four actual rendering helper bodies, real GPU bindings, material-to-concrete cross-library composition, production ownership/threading and full application fidelity remain unverified. No editor admission or changes to prior implementation.',
          'New test-only observer fixture, self-test, concrete probe/build/verifier. All prior source/artifacts, originals,313 editor dependencies and editor executable unchanged. Four fresh clean builds, repeated original/owned traces and7168 prior pass-control regressions pass. No SFM launch/control/enumeration, dependency install or publication.'])
    receipt=run/'results.json';receipt.write_text(json.dumps(outcome,indent=2)+'\n')
    (OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt),receipt_sha256=sha(receipt)),indent=2)+'\n')
    print(json.dumps(dict(status=outcome['status'],receipt=str(receipt),coverage=coverage,trace_bytes=len(expected),editor_rebuilt=False),indent=2),flush=True)

if __name__=='__main__':
    main()
