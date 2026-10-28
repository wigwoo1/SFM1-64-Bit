"""Whole original base shader draw + shadow setup vs existing x64 helpers."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, itertools, json, random, re, struct, subprocess, sys, tempfile, time
from verify_material_texture_file_resolver import closure
from verify_ifm_draw_packet import check_ranges

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT/'outputs/base-shader-entry-reference-20261028'
STD = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\stdshader_dx9.dll')
DUMP = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')

def sha(p):
    with Path(p).open('rb') as f:
        return hashlib.file_digest(f,'sha256').hexdigest()

def need(b,s):
    if not b:
        raise RuntimeError(s)

def corpus():
    rng, rows = random.Random(2026102861), []
    masks = [0x8000,0x10000,0x2000,0x400,0x10000000,0x20000000]
    for combo, mutation, shadow in itertools.product(range(64),range(32),range(2)):
        flags = sum(v for i,v in enumerate(masks) if combo & (1<<i)) | (rng.getrandbits(32)&~0x3001a400)
        rows.append([len(rows),shadow,flags,mutation,rng.getrandbits(32),rng.getrandbits(32)])
    for _ in range(512):
        rows.append([len(rows),rng.randrange(2),rng.getrandbits(32),rng.randrange(32),rng.getrandbits(32),rng.getrandbits(32)])
    return rows

def prediction(rows):
    raw, total_events, nested, shadow_count = bytearray(), 0, 0, 0
    for serial, shadow, flags, mutation, snapshot, compression in rows:
        state = [1,shadow,1,snapshot,0,1]  # params,shadow,API,snapshot,pass,data
        parameter_flags = [flags,flags^0x3001a400]
        events = []
        def event(kind,receiver=0,slot=0,a=0,b=0,depth=0,params=0,shadow_arg=0,api=0,c=0,ctx=0):
            events.append([kind,receiver,slot,a,b,depth,*state,params,shadow_arg,api,c,ctx])
        if shadow:
            shadow_count += 1
            event(0,1)
            if mutation&1:
                state = [2,2,2,snapshot^0x11223344,17,2]
            if mutation&4:
                parameter_flags = [v^0xffffffff for v in parameter_flags]
            cached = parameter_flags[state[0]-1]
            actions = []
            if cached&0x8000:
                actions += [(3,0,0xffffffff),(2,0,0xffffffff)]
            if cached&0x10000:
                actions += [(4,1,0xffffffff),(2,0,0xffffffff)]
            if cached&0x2000:
                actions.append((15,0,0xffffffff))
            if cached&0x400:
                actions.append((1,1,0xffffffff))
            if cached&0x10000000:
                actions.append((14,2,1))
            if cached&0x20000000:
                actions.append((24,1,0xffffffff))
            for slot,a,b in actions:
                event(1,state[1],slot,a,b)
                if mutation&2:
                    state[1] = 3-state[1]
                    parameter_flags = [v^0xffffffff for v in parameter_flags]
                    state[3] = (state[3]+5)&0xffffffff
                    state[4] = (state[4]+3)&0xffffffff
        event(2,1,14,params=1,shadow_arg=shadow,api=1,c=compression,ctx=1)
        if mutation&8:
            nested += 1
            state = [2,0,2,0xfedcba98,0,2]
            event(2,2,14,depth=1,params=2,shadow_arg=0,api=2,c=compression^0x12345678,ctx=2)
            state = [0]*6
            event(3,1)
        # Concrete callback may replace every field (bit16); the actual wrapper
        # then clears all six regardless. Callback args always stayed original.
        state = [0]*6
        header = [serial,1+bool(mutation&8),*parameter_flags,len(events),*state]
        raw += struct.pack('<11I',*header)
        for e in events:
            raw += struct.pack('<17I',*e)
        total_events += len(events)
    return bytes(raw),dict(cases=len(rows),events=total_events,nested_draws=nested,
                          shadow_setups=shadow_count,independent_checks=2*len(rows)+2*nested)

def main():
    OUT.mkdir(exist_ok=True)
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    print('Verification directory: '+str(run),flush=True)
    parent=ROOT/'outputs/material-shader-composition-20261028/verification-88fuvw4v/results.json'
    need(sha(parent)=='921369d76e0443db6e134f729674fd14319f04abeac9ff8feab1ac1ff289e778','accepted connected material/system receipt')
    prior=json.loads(parent.read_text())
    protected={**prior['protected'],**prior['source'],str(parent):sha(parent)}
    for e in prior['executables'].values():
        protected[e['path']]=e['sha256']
    # Existing base helpers were previously tested only against owned fixtures.
    # Preserve them and their pre-existing tests while verifying real code now.
    for name in ['studio_base_shader_draw_candidate.h','studio_shader_shadow_setup_candidate.h','test_studio_base_shader_draw.cpp','test_studio_shader_shadow_setup.cpp']:
        p=ROOT/'work'/name;protected[str(p)]=sha(p)
    originals=dict(prior['original_inputs']);originals[str(STD)]= 'fb09e2ddc8a99aff4ef31e3fddfdb1ca84d89379336abdab4e76e94adfa7e6f8'
    need(all(sha(p)==h for p,h in protected.items()),'prior source/artifacts unchanged before')
    need(all(sha(p)==h for p,h in originals.items()),'original/private inputs unchanged before')
    sources=closure([ROOT/'work/probe_base_shader_entry_reference.cpp',ROOT/'work/build_base_shader_entry_reference.cmd',ROOT/'work/test_studio_base_shader_draw.cpp',ROOT/'work/test_studio_shader_shadow_setup.cpp',Path(__file__)])
    for module in tuple(sys.modules.values()):
        path=getattr(module,'__file__',None)
        if path and Path(path).resolve().parent==ROOT/'work' and Path(path).suffix=='.py':
            sources[str(Path(path).resolve())]=sha(path)
    image,static=check_ranges(STD,'stdshader_dx9-disassembly.txt',[
        ('whole_base_shader_draw',0x100652c0,0x1006534b),
        ('whole_base_shadow_setup',0x10064ab0,0x10064b73)])
    need(image.word(0x1009c39c+6*4)==0x100652c0 and image.word(0x1009c39c+14*4)==0x100537a0,'original VertexLitGeneric entry and concrete draw target')
    (run/'static-ranges.private.json').write_text(json.dumps(static,indent=2)+'\n')
    rows=corpus();query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<6I',*r) for r in rows))
    expected,coverage=prediction(rows);(run/'analytical.bin').write_bytes(expected)
    started=time.time()
    built=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_base_shader_entry_reference.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180)
    (run/'build-driver.log').write_text(built.stdout+built.stderr)
    need(built.returncode==0,'fresh original/owned/two existing regression builds')
    executables={}
    for tag,name,machine,magic,logname in [
        ('reference','base_shader_entry_reference_x86.exe',0x14c,0x10b,'reference-build.log'),
        ('owned','probe_base_shader_entry_reference_x64.exe',0x8664,0x20b,'probe_base_shader_entry_reference-build.log'),
        ('base_regression','test_studio_base_shader_draw_x64.exe',0x8664,0x20b,'test_studio_base_shader_draw-build.log'),
        ('shadow_regression','test_studio_shader_shadow_setup_x64.exe',0x8664,0x20b,'test_studio_shader_shadow_setup-build.log')]:
        exe=OUT/name;raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic and b'asInvoker' in raw,'fresh architecture/manifest '+tag)
        log=(OUT/logname).read_text();(run/logname).write_text(log)
        need(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log,re.I),'clean compile '+tag)
        inspected=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,timeout=30)
        (run/(tag+'-imports.txt')).write_text(inspected.stdout)
        dlls=sorted(set(re.findall(r'^\s+([^\s]+\.dll)\s*$',inspected.stdout,re.I|re.M)))
        need(inspected.returncode==0 and dlls==['KERNEL32.dll'],'system-only imports '+tag)
        executables[tag]=dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=dlls)
    runs=[]
    for tag in ['reference','owned','reference','owned']:
        trace=run/(tag+'-'+str(len(runs))+'.bin')
        result=subprocess.run([executables[tag]['path'],*([str(STD)] if tag=='reference' else []),str(query),str(trace)],cwd=run,capture_output=True,text=True,timeout=90)
        (run/(trace.stem+'.log')).write_text(result.stdout+result.stderr)
        need(result.returncode==0,tag+' exit='+str(result.returncode)+'; inspect '+trace.stem+'.log')
        actual=trace.read_bytes()
        if actual!=expected:
            at=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
            raise RuntimeError(tag+' full trace mismatch at byte '+str(at))
        need(f'PASS base_shader_entry_cases={coverage["cases"]} events={coverage["events"]} independent_checks={coverage["independent_checks"]}' in result.stdout,'exact entry/live-state/nested coverage '+tag)
        runs.append(dict(tag=tag,output=str(trace),sha256=sha(trace),stdout=result.stdout.strip()))
    regressions={}
    for tag,pattern in [('base_regression','base_shader_draw_cases=131072'),('shadow_regression','shadow_setup_cases=16384')]:
        p=subprocess.run([executables[tag]['path']],cwd=run,capture_output=True,text=True,timeout=60)
        (run/(tag+'.log')).write_text(p.stdout+p.stderr)
        need(p.returncode==0 and pattern in p.stdout,'fresh owned regression '+tag)
        regressions[tag]=p.stdout.strip()
    need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in protected.items()),'all source/prior artifacts unchanged after')
    need(all(sha(p)==h for p,h in originals.items()),'original/private inputs unchanged after')
    need(all(sha(p)==h for p,h in prior['editor_dependencies'].items()) and sha(prior['editor'])==prior['editor_sha256'],'313 editor dependencies and exact executable unchanged')
    outcome=dict(status='VERIFIED existing base draw/shadow helpers vs original standard shader entry; concrete shader14 INCOMPLETE',
        utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',parent=str(parent),parent_sha256=sha(parent),
        source=sources,protected=protected,original_inputs=originals,executables=executables,runs=runs,coverage=coverage,
        complete_trace_bytes=len(expected),query_sha256=sha(query),analytical_sha256=sha(run/'analytical.bin'),
        compiler_warnings=0,regressions=regressions,static_ranges=[{k:v for k,v in r.items() if k!='instructions'} for r in static],
        editor=prior['editor'],editor_sha256=prior['editor_sha256'],editor_dependencies=prior['editor_dependencies'],editor_rebuilt=False,
        findings=[
          'Whole original standard shader slot6 and internal shadow setup execute unchanged RX instructions. The actual VertexLitGeneric vtable slot6 is retained; only its slot14 data entry is replaced with an observer. No original machine-code arrays, DllMain or imports loaded.',
          'Existing x64 base draw and shadow setup helpers match native complete traces for all controlled state publication, shadow flag/callback order, retained supplied arguments and successful-return cleanup. Existing helper source is unchanged.',
          'Reset can replace shared parameter/shadow/API/data pointers and snapshot/pass words; flags are read from the live parameter pointer after reset and retained across later calls. Shadow receivers reload after every callback. Concrete14 still receives original supplied parameter/shadow/API/compression/context arguments despite shared-global changes.',
          'One-level nested calls overwrite shared globals and clear them on return instead of restoring the outer state. Outer callback observes all six cleared; subsequent callback mutations are cleared by the outer return. This does not establish reentrancy/thread safety; owned calls must remain serialized.',
          'Concrete VertexLit draw14 at100537A0, real shadow service implementation, shader resource/constant binding, material-to-standard-shader cross-library composition, exception cleanup and actual GPU output remain unverified. No renderer admission or editor changes.',
          'New probe/build/verifier only; all prior source/artifacts, originals,313 editor dependencies and editor executable unchanged. Four fresh clean builds, original/owned traces twice each, and131072+16384 prior regression cases pass. No SFM process access/control, dependencies or publication.'])
    receipt=run/'results.json';receipt.write_text(json.dumps(outcome,indent=2)+'\n')
    (OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt),receipt_sha256=sha(receipt)),indent=2)+'\n')
    print(json.dumps(dict(status=outcome['status'],receipt=str(receipt),coverage=coverage,trace_bytes=len(expected),editor_rebuilt=False),indent=2),flush=True)

if __name__=='__main__':
    main()
