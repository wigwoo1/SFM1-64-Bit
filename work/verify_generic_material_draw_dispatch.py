"""Real generic material routing and connected no-overlay concrete profile."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, itertools, json, random, re, struct, subprocess, sys, tempfile, time
from verify_material_texture_file_resolver import closure
from verify_ifm_draw_packet import check_ranges

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT/'outputs/generic-material-draw-dispatch-20261028'
STD = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\stdshader_dx9.dll')
DUMP = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')

def sha(p):
    with Path(p).open('rb') as f:
        return hashlib.file_digest(f,'sha256').hexdigest()

def need(b,s):
    if not b:
        raise RuntimeError(s)

def corpus():
    rng,rows=random.Random(2026102871),[]
    def add(integrated=0,shadow=0,shared=0,vertex=1,f14=0,f15=0,q17=0,q59=0,mat=0,phong=0,warp=0,mask=0,bump=0,hw=92,mutation=None):
        rows.append([len(rows),integrated,shadow,shared,vertex,f14,f15,q17,q59,mat,phong,warp,mask,bump,hw&0xffffffff,len(rows)%32 if mutation is None else mutation,rng.getrandbits(32)])
    for mat,phong,warp,mask,bump,hw,shadow,integrated in itertools.product(range(2),range(2),[0,3],range(2),[0,0x101],[91,92,0x80000000],range(2),range(2)):
        i=len(rows);add(integrated,shadow,(i//2)%2,[0,1,0x100,0x101][i%4],(i//2)%3,(i//6)%3,[0,1,0x100,0x101][i%4],[0,1,0x100,0x101][(i//4)%4],mat,phong,warp,mask,bump,hw)
    for shadow,shared,vertex,f14,f15,q17,q59 in itertools.product(range(2),range(2),[0,1,0x100,0x101],range(3),range(3),[0,1,0x100,0x101],[0,1,0x100,0x101]):
        add(0,shadow,shared,vertex,f14,f15,q17,q59)
    for _ in range(512):
        add(rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.getrandbits(32),rng.randrange(3),rng.randrange(3),rng.getrandbits(32),rng.getrandbits(32),rng.choice([0,1,0xffffffff]),rng.choice([0,1,0xffffffff]),rng.choice([0,3,19,6]),rng.randrange(2),rng.getrandbits(32),rng.choice([-1,91,92,2147483647]),rng.randrange(32))
    return rows

def prediction(rows):
    output=bytearray();total_events=total_leaves=0;coverage=dict(generic=0,phong=0,skip_before=0,skip_after=0,double_shadow=0,integrated=0,hardware_queries=0,bump_queries=0,query17=0,query59=0)
    for row in rows:
        serial,integrated,shadow,shared,vertex,f14,f15,q17,q59,mat,phong,warp,mask,bump,hw,mutation,compression=row
        vertex=1 if integrated else vertex;v100=int(f14==2);v101=int(f15==2);api=2;step=0;events=[];leaves=0
        coverage['integrated']+=integrated
        def event(kind,a=0,b=0,receiver=0):
            events.append([kind,a,b,receiver,vertex,step,shared,api,v100,v101,mat,shadow])
        wanted=bool(mat and phong and ((warp&15)==3 or mask==1))
        if mat and phong and (warp&15)!=3 and mask!=1:
            event(3,bump,0,26);coverage['bump_queries']+=1;wanted=bool(bump&255)
            if mutation&1:mat=0;v100^=1
        skin=False
        if wanted:
            event(4,hw,0,3);coverage['hardware_queries']+=1
            if mutation&2:v100^=1;v101^=1
            skin=struct.unpack('<i',struct.pack('<I',hw))[0]>=92
        def consume(kind,flash=0,mode=0):
            nonlocal step,leaves,v100,v101,shared,api
            event(kind,flash,mode,1);leaves+=1
            if mutation&16:v100^=1;v101^=1;shared^=1;api=1
            step+=1
        def skip():
            nonlocal step
            event(0);step+=1
        if skin:
            coverage['phong']+=1;consume(2)
        else:
            coverage['generic']+=1;flash=False
            if (vertex&255) or (not integrated and f14 and v100):
                if shared:flash=bool(q17&255)
                else:
                    event(5,q17,0,api);coverage['query17']+=1;flash=bool(q17&255)
                    if mutation&4:v101^=1
            special=flash and not integrated and f15 and v101
            if special and shadow:
                coverage['double_shadow']+=1;consume(1,1,0);consume(1,1,1)
            elif special:
                event(6,q59,0,1);coverage['query59']+=1
                if mutation&8:shared^=1
                if q59&255:coverage['skip_before']+=1;skip();consume(1,1,1)
                else:coverage['skip_after']+=1;consume(1,1,0);skip()
            else:consume(1,int(flash),0)
        header=[serial,len(events),leaves,step,shared,api,v100,v101,mat]
        output+=struct.pack('<9I',*header)
        for e in events:output+=struct.pack('<12I',*e)
        total_events+=len(events);total_leaves+=leaves
    need(all(coverage.values()),'all routing/query/skip-order classes covered')
    return bytes(output),dict(cases=len(rows),events=total_events,renderer_boundaries=total_leaves,classes=coverage)

def main():
    OUT.mkdir(exist_ok=True)
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    print('Verification directory: '+str(run),flush=True)
    parent=ROOT/'outputs/vertexlit-shared-mapping-correction-20261028/verification-pdu9ibr9/results.json'
    need(sha(parent)=='1136316d7ebc915128da7394c738b3687ca985189a205283605f4f90d15af4d1','accepted corrected mapping/initializer receipt')
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
    sources=closure([ROOT/'work/probe_generic_material_draw_dispatch.cpp',ROOT/'work/build_generic_material_draw_dispatch.cmd',ROOT/'work/test_studio_vertex_lit_passes.cpp',ROOT/'work/probe_private_boundary_observer_self_test.cpp',Path(__file__)])
    for module in tuple(sys.modules.values()):
        path=getattr(module,'__file__',None)
        if path and Path(path).resolve().parent==ROOT/'work' and Path(path).suffix=='.py':
            sources[str(Path(path).resolve())]=sha(path)
    image,static=check_ranges(STD,'stdshader_dx9-disassembly.txt',[
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
    concrete=json.loads(Path(prior['parent']).read_text());mapping_source=Path(prior['parent']).parent/'mapping.bin'
    need(sha(mapping_source)==concrete['mapping_sha256'],'accepted actual-built mapping artifact')
    mapping=run/'mapping.bin';mapping.write_bytes(mapping_source.read_bytes());protected[str(mapping_source)]=sha(mapping_source)
    rows=corpus();query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<17I',*r) for r in rows))
    expected,coverage=prediction(rows);(run/'analytical.bin').write_bytes(expected)
    started=time.time()
    built=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_generic_material_draw_dispatch.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180)
    (run/'build-driver.log').write_text(built.stdout+built.stderr)
    need(built.returncode==0,'fresh original/owned/two existing regression builds')
    executables={}
    for tag,name,machine,magic,logname in [
        ('reference','generic_material_dispatch_reference_x86.exe',0x14c,0x10b,'reference-build.log'),
        ('owned','probe_generic_material_draw_dispatch_x64.exe',0x8664,0x20b,'probe_generic_material_draw_dispatch-build.log'),
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
        need(f'PASS generic_dispatch_cases={coverage["cases"]} events={coverage["events"]} renderer_boundaries={coverage["renderer_boundaries"]}' in result.stdout,'exact entry/live-state/nested coverage '+tag)
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
    outcome=dict(status='VERIFIED real generic routing including connected concrete no-overlay profile; generic/Phong rendering bodies INCOMPLETE',
        utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',parent=str(parent),parent_sha256=sha(parent),
        source=sources,protected=protected,original_inputs=originals,executables=executables,runs=runs,coverage=coverage,
        complete_trace_bytes=len(expected),query_sha256=sha(query),analytical_sha256=sha(run/'analytical.bin'),
        compiler_warnings=0,regressions=regressions,observer_self_test=selftest.stdout.strip(),mapping_sha256=sha(mapping),static_ranges=[{k:v for k,v in r.items() if k!='instructions'} for r in static],
        editor=prior['editor'],editor_sha256=prior['editor_sha256'],editor_dependencies=prior['editor_dependencies'],editor_rebuilt=False,
        findings=[
          'Original whole main-lighting router1005A820, skin predicate10054420, flashlight query100651D0 and skip dispatcher10065A70 execute unchanged RX instructions. Integrated no-overlay cases enter actual VertexLitGeneric slot14 and its actual main info builder before reaching the real router.',
          'Only generic renderer10055320 and Phong renderer100380F0 are hardware boundary observers. Generic renderer receives API in EDX plus nine stack arguments; Phong receives seven cdecl stack arguments. No claim that either body executes or binds GPU resources.',
          'Native/owned/analytical complete traces cover skin eligibility, signed hardware92 gate, global-versus-supplied API queries, low-byte flags, direct/synthetic info14C/150 declarations, single/double dispatch and skip-before/after ordering under controlled live callbacks. Generic flashlight local has unspecified upper stack bytes, so only its low byte is compared.',
          'Installed VertexLitGeneric info14C/150 are undeclared. Integrated cases retain that actual mapping and use vertexLit1 with effects disabled. Additional direct-router declared14C/150 cases are controlled synthetic contracts, not proof of legal other-family assets or benchmark material routing.',
          'New x64 routing helper adds only CPU orchestration with externally owned predicate/services/render callbacks. It is not admitted to editor. Real generic/Phong rendering, overlays within this composition, resource ownership/threading and full image fidelity remain unverified.',
          'Four clean fresh builds, private-thread observer self-test, repeated original/owned traces and7168 prior pass-control regressions pass. All original/private files, prior source/artifacts,313 editor dependencies and exact editor executable unchanged. No SFM access, dependencies or publication.'])
    receipt=run/'results.json';receipt.write_text(json.dumps(outcome,indent=2)+'\n')
    (OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt),receipt_sha256=sha(receipt)),indent=2)+'\n')
    print(json.dumps(dict(status=outcome['status'],receipt=str(receipt),coverage=coverage,trace_bytes=len(expected),editor_rebuilt=False),indent=2),flush=True)

if __name__=='__main__':
    main()
