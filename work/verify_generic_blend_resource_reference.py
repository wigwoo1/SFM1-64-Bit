"""Original texture-alpha/blend classification and connected generic prefix."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, itertools, json, random, re, struct, subprocess, sys, tempfile, time
from verify_material_texture_file_resolver import closure
from verify_ifm_draw_packet import check_ranges

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT/'outputs/generic-blend-resource-reference-20261028'
STD = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\stdshader_dx9.dll')
DUMP = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')

def sha(p):
    with Path(p).open('rb') as f:
        return hashlib.file_digest(f,'sha256').hexdigest()

def need(b,s):
    if not b:
        raise RuntimeError(s)

def corpus():
    rng,rows=random.Random(2026102881),[]
    def add(integrated=0,first=6,second=17,alpha=1,snapshot=0,flags=0x100,flags2=0,base=3,detail=3,env=3,a0=1,a1=0,a2=0,mutation=None):
        rows.append([len(rows),integrated,first&0xffffffff,second&0xffffffff,alpha,snapshot,flags,flags2,base,detail,env,a0,a1,a2,len(rows)%32 if mutation is None else mutation])
    masks=[0x20,0x80,0x100,0x40,0x100000,0x200000,0x1000000]
    for bits,secondary,snap,arg,ret in itertools.product(range(128),range(2),range(2),range(2),range(2)):
        add(flags=sum(v for i,v in enumerate(masks) if bits&(1<<i)),flags2=secondary*0x400000,snapshot=snap,alpha=arg,a0=ret,a1=1-ret,a2=0x100)
    for typ,arg in itertools.product(range(256),[0,0x101]):
        add(base=typ,alpha=arg,flags=0x200100,detail=4,a0=0x101,a1=0x100)
    for first,second,mutation in itertools.product([-1,-2,-2147483648],[-1,-2,6,17,22],range(32)):
        add(first=first,second=second,mutation=mutation,flags=0x200100,a0=0,a1=0x101,a2=0x100)
    for base,detail,env,snap,ret,mutation in itertools.product([3,4,19],[3,4],[3,4],range(2),[0,0x101],range(32)):
        flags=[0,0x100,0x200100,0x80,0x180,0x200080,0x1000000,0x200140][len(rows)%8]
        add(1,base=base,detail=detail,env=env,snapshot=snap,flags=flags,flags2=0x400000 if len(rows)&8 else 0,a0=ret,a1=0x100,a2=0x101,mutation=mutation)
    for _ in range(512):
        add(rng.randrange(2),rng.choice([-1,-2,6,17,22]),rng.choice([-1,-2,6,17,22]),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.choice([3,4,19,255]),rng.choice([3,4,19]),rng.choice([3,4,19]),*[rng.getrandbits(32) for _ in range(3)],rng.randrange(32))
    return rows

def prediction(rows):
    output=bytearray();event_count=prefixes=0;classes=[0]*4;queries=0
    for row in rows:
        serial,integrated,first,second,arg,snap,f0,f2,t0,t1,t2,a0,a1,a2,mutation=row
        flags=[f0,f0^0x180];secondary=[f2,f2^0x400000];current=0;events=[];types={6:t0,17:t1,22:t2};returns=[a0,a1,a2]
        def signed(v):return struct.unpack('<i',struct.pack('<I',v))[0]
        def event(kind,a,b,c):events.append([kind,a,b,c,current,*flags,snap])
        def texture_alpha(index,argument):
            nonlocal current,snap,queries
            if signed(index)<0 or (types[index]&15)!=3:return False
            owner=current;f=flags[owner]
            if argument&255:
                if f&0x1000000 or (f&0x40 and not secondary[owner]&0x400000) or f&0x100000 or not f&0x200100:return False
            resource=({6:0,17:1,22:2}[index]+owner+bool(mutation&4))%3
            event(1,index,owner,resource)
            if mutation&1:current^=1
            value=returns[resource];event(2,resource,value,0);queries+=1
            if mutation&2:flags[0]^=0x180;flags[1]^=0x180
            if mutation&8:current^=1
            if mutation&16:snap^=1
            return bool(value&255)
        if integrated:
            first,arg,second=(6,1,0xffffffff) if t0&15==3 else (22,0,0xffffffff)
        alpha=bool(snap&1 or flags[current]&0x20)
        if not alpha:
            alpha=texture_alpha(first,arg)
            alpha=alpha and not flags[current]&0x100
        if not alpha and second!=0xffffffff:alpha=texture_alpha(second,arg)
        result=(2 if flags[current]&0x80 else 0)+int(alpha);classes[result]+=1
        header=[serial,result,current,snap,*flags,integrated,len(events)];output+=struct.pack('<8I',*header)
        for e in events:output+=struct.pack('<8I',*e)
        event_count+=len(events);prefixes+=integrated
    need(all(classes) and queries and prefixes,'four classes/resource callbacks/connected generic prefix covered')
    return bytes(output),dict(cases=len(rows),events=event_count,generic_prefix_cases=prefixes,classes=classes,texture_alpha_callbacks=queries)

def main():
    OUT.mkdir(exist_ok=True)
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    print('Verification directory: '+str(run),flush=True)
    parent=ROOT/'outputs/generic-material-draw-dispatch-20261028/verification-fgfc4x2s/results.json'
    need(sha(parent)=='1e335ff7c4c041925ec10e2bb8df81051dea16371d60f3e4dd7c587eda6fff80','accepted original generic routing receipt')
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
    sources=closure([ROOT/'work/probe_generic_blend_resource_reference.cpp',ROOT/'work/build_generic_blend_resource_reference.cmd',ROOT/'work/probe_generic_material_draw_dispatch.cpp',ROOT/'work/probe_private_boundary_observer_self_test.cpp',Path(__file__)])
    for module in tuple(sys.modules.values()):
        path=getattr(module,'__file__',None)
        if path and Path(path).resolve().parent==ROOT/'work' and Path(path).suffix=='.py':
            sources[str(Path(path).resolve())]=sha(path)
    image,static=check_ranges(STD,'stdshader_dx9-disassembly.txt',[
        ('whole_blend_classifier',0x10002d20,0x10002da6),
        ('whole_texture_alpha_predicate',0x10064d30,0x10064dce),
        ('whole_snapshot_alpha_flag',0x10064dd0,0x10064dd9),
        ('generic_preparation_prefix_ONLY',0x10055320,0x10055553),
        ('whole_generic_material_routing',0x1005a820,0x1005a99d),
        ('whole_skin_predicate',0x10054420,0x10054487),
        ('whole_flashlight_query',0x100651d0,0x10065200),
        ('whole_concrete_VertexLit_draw',0x100537a0,0x10053a02),
        ('whole_main_info_mapping',0x10052f10,0x100532a2),
        ('whole_flesh_info_mapping',0x10053360,0x1005341d),
        ('whole_original_cloak_gate',0x100096f0,0x100097c1),
        ('whole_original_skip_dispatch',0x10065a70,0x10065b4e),
        ('whole_original_memset',0x100671e0,0x1006725a)])
    need(image.word(0x1009c39c+6*4)==0x100652c0 and image.word(0x1009c39c+14*4)==0x100537a0,'original VertexLitGeneric entry and concrete draw target')
    (run/'static-ranges.private.json').write_text(json.dumps(static,indent=2)+'\n')
    correction=json.loads(Path(prior['parent']).read_text());concrete=json.loads(Path(correction['parent']).read_text());mapping_source=Path(correction['parent']).parent/'mapping.bin'
    need(sha(mapping_source)==concrete['mapping_sha256'],'accepted actual-built mapping artifact')
    mapping=run/'mapping.bin';mapping.write_bytes(mapping_source.read_bytes());protected[str(mapping_source)]=sha(mapping_source)
    rows=corpus();query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<15I',*r) for r in rows))
    expected,coverage=prediction(rows);(run/'analytical.bin').write_bytes(expected)
    started=time.time()
    built=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_generic_blend_resource_reference.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180)
    (run/'build-driver.log').write_text(built.stdout+built.stderr)
    need(built.returncode==0,'fresh original/owned/two existing regression builds')
    executables={}
    for tag,name,machine,magic,logname in [
        ('reference','generic_blend_resource_reference_x86.exe',0x14c,0x10b,'reference-build.log'),
        ('owned','probe_generic_blend_resource_reference_x64.exe',0x8664,0x20b,'probe_generic_blend_resource_reference-build.log'),
        ('dispatch_regression','probe_generic_material_draw_dispatch_x64.exe',0x8664,0x20b,'probe_generic_material_draw_dispatch-build.log'),
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
        need(f'PASS blend_resource_cases={coverage["cases"]} events={coverage["events"]} generic_prefix_cases={coverage["generic_prefix_cases"]}' in result.stdout,'exact entry/live-state/nested coverage '+tag)
        runs.append(dict(tag=tag,output=str(trace),sha256=sha(trace),stdout=result.stdout.strip()))
    target=run/'dispatch-regression.bin'
    regression=subprocess.run([executables['dispatch_regression']['path'],str(parent.parent/'mapping.bin'),str(parent.parent/'queries.bin'),str(target)],cwd=run,capture_output=True,text=True,timeout=90)
    (run/'dispatch-regression.log').write_text(regression.stdout+regression.stderr)
    need(regression.returncode==0 and sha(target)==prior['analytical_sha256'] and 'PASS generic_dispatch_cases=3200' in regression.stdout,'fresh3200-case routing regression')
    regressions={'dispatch_regression':regression.stdout.strip()}
    need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in protected.items()),'all source/prior artifacts unchanged after')
    need(all(sha(p)==h for p,h in originals.items()),'original/private inputs unchanged after')
    need(all(sha(p)==h for p,h in prior['editor_dependencies'].items()) and sha(prior['editor'])==prior['editor_sha256'],'313 editor dependencies and exact executable unchanged')
    outcome=dict(status='VERIFIED original resource-alpha/blend classification and connected generic prefix; complete generic/GPU body INCOMPLETE',
        utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',parent=str(parent),parent_sha256=sha(parent),
        source=sources,protected=protected,original_inputs=originals,executables=executables,runs=runs,coverage=coverage,
        complete_trace_bytes=len(expected),query_sha256=sha(query),analytical_sha256=sha(run/'analytical.bin'),
        compiler_warnings=0,regressions=regressions,observer_self_test=selftest.stdout.strip(),mapping_sha256=sha(mapping),static_ranges=[{k:v for k,v in r.items() if k!='instructions'} for r in static],
        editor=prior['editor'],editor_sha256=prior['editor_sha256'],editor_dependencies=prior['editor_dependencies'],editor_rebuilt=False,
        findings=[
          'Whole original blend10002D20, texture-alpha predicate10064D30 and snapshot-alpha10064DD0 execute unchanged RX instructions. Resource getter and resource slot6 are explicit observers with valid borrowed identities, not actual VTF/GPU texture implementations.',
          'Connected cases enter actual concrete14, main router1005A820 and generic renderer10055320. Real generic preparation runs through its original call to the complete blend classifier. A declared hardware checkpoint stops at10055553 and restores only the prologue-saved registers/return frame. The remaining renderer, cookie check and cleanup DO NOT execute and are not claimed.',
          'New owned helper matches original callback/list/resource capture and live flag rereads. The resource returned before list replacement stays captured; subsequent material flag checks and second texture query see the live list. Generic prefix EBX retains the original supplied list even if classifier callbacks replace the global list. Snapshot bit is sampled before callbacks. Full return words are normalized from low bytes where native callers test AL.',
          'Four blend classes, signed negative indices, texture type-byte domains, primary/secondary flag gates, high-byte alpha payloads and callback changes are compared to independent prediction. Connected prefix uses actual installed info with detail mode0 and no overlays; no other shader branch or complete prefix reconstruction claimed.',
          'Fresh isolated builds, private-thread observer self-test, repeated original/owned complete traces and fresh3200 prior routing cases pass. Original files, prior source/artifacts,313 editor dependencies and editor executable unchanged; no SFM process access, original writes, dependency install or publication.',
          'Real resource loading and generic/Phong rendering/GPU output, complete function cleanup, concurrent mutation safety and overall SFM fidelity remain unverified. New CPU helper is not admitted to the editor.'])
    receipt=run/'results.json';receipt.write_text(json.dumps(outcome,indent=2)+'\n')
    (OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt),receipt_sha256=sha(receipt)),indent=2)+'\n')
    print(json.dumps(dict(status=outcome['status'],receipt=str(receipt),coverage=coverage,trace_bytes=len(expected),editor_rebuilt=False),indent=2),flush=True)

if __name__=='__main__':
    main()
