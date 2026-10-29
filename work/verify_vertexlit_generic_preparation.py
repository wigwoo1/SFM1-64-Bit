"""Extended installed VertexLitGeneric preparation, before either render body."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, itertools, json, random, re, struct, subprocess, sys, tempfile, time
from verify_material_texture_file_resolver import closure
from verify_ifm_draw_packet import check_ranges

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT/'outputs/vertexlit-generic-preparation-correction-20261028'
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
    # Preserve all 5088 historical queries, but make material values explicit;
    # classifier call arguments must never overwrite these inputs.
    for row in rows:
        serial, material_integer = row[0], row[4]
        scalar = [0x7fc00001,0x80000000,0xbf800000,0x3f800000,0x7f800000][serial%5]
        row.extend([material_integer]*4+[scalar,scalar,4,4,serial&1,0])
    seed=[0,1,6,17,1,0,0x40,0,3,4,4,0x101,0x100,0,0,
          1,0,0,0,0,0x3f800000,4,4,0,0]
    def extended(**fields):
        row=seed.copy();row[0]=len(rows)
        indices=dict(flags=6,flags2=7,mutation=14,integer98=15,mode=16,d4=17,d8=18,
                     scalar33=19,scalar105=20,envmap=21,mask=22,shadow=23,api=24)
        for key,value in fields.items():row[indices[key]]=value
        rows.append(row)
    # Four independent integers: both feature gates can be true in mode zero.
    for i98,mode,d4,d8,mutation in itertools.product([0,1,2,0xffffffff],
            [0,1,2,3,0xffffffff,0x80000000,0x7fffffff],[0,1,0xffffffff],[0,1,0xffffffff],[0,11]):
        extended(integer98=i98,mode=mode,d4=d4,d8=d8,mutation=mutation)
    # Both flashlight-query sources, including disagreement between API and flags.
    # Each combination is observed from the ORIGINAL router's actual arguments.
    for shadow,flag,api,base_mask,envmap,mask,mutation in itertools.product(
            range(2),range(2),range(2),range(2),[3,4,19],[3,4,19],[0,11]):
        extended(shadow=shadow,flags2=flag*0x400,api=api,flags=base_mask*0x40,
                 envmap=envmap,mask=mask,mutation=mutation)
    floats=[0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc00001,1,0x80000001]
    for f33,f105,flash,base_mask in itertools.product(floats,floats,range(2),range(2)):
        extended(scalar33=f33,scalar105=f105,api=flash,flags=base_mask*0x40)
    return rows

def prediction(rows):
    output=bytearray();event_count=prefixes=0;classes=[0]*4;queries=0
    field_values=[set() for _ in range(18)];route_cases={}
    for row in rows:
        serial,integrated,first,second,arg,snap,f0,f2,t0,t1,t2,a0,a1,a2,mutation=row[:15]
        i98,mode,d4,d8,f33,f105,envmap,mask,shadow,api_flash=row[15:]
        flashlight=((f2>>10)&1) if shadow else api_flash
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
        if integrated:
            value=signed(mode)
            # Retained input list0; callback replacement list1 has different values.
            scalar33,scalar105=[struct.unpack('<f',struct.pack('<I',v))[0] for v in (f33,f105)]
            base_mask=not flashlight and bool(flags[0]&0x40)
            mask_texture=base_mask and mask&15==3
            state=[result,int(result not in (1,3) and not f0&0x100 and not flashlight),
                   int(not flashlight and envmap&15==3),int(i98!=1),0,0,
                   int(mode!=0),max(0,min(2,value)),int(d4!=0 and value==0),
                   int(d8!=0 and value==0),0,int(base_mask),int(scalar33!=0),
                   int(not base_mask and not mask_texture and scalar105>0),
                   int(mask_texture),1,flashlight,0]
            output+=struct.pack('<18I',*state)
            for seen,v in zip(field_values,state):seen.add(v)
            route=f'shadow={shadow},flag={(f2>>10)&1},api={api_flash},flash={flashlight}'
            route_cases[route]=route_cases.get(route,0)+1
        for e in events:output+=struct.pack('<8I',*e)
        event_count+=len(events);prefixes+=integrated
    need(all(classes) and queries and prefixes,'four classes/resource callbacks/connected generic prefix covered')
    for i in [1,2,3,6,8,9,11,12,13,14,16]:
        need(field_values[i]=={0,1},'both outcomes for field '+str(i))
    need(field_values[7]=={0,1,2} and len(route_cases)==8,'clamp and all flashlight query combinations')
    return bytes(output),dict(cases=len(rows),historical_cases=5088,events=event_count,generic_prefix_cases=prefixes,classes=classes,texture_alpha_callbacks=queries,
                             observed_field_values=[sorted(s) for s in field_values],flashlight_routes=route_cases)

def main():
    OUT.mkdir(exist_ok=True)
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    print('Verification directory: '+str(run),flush=True)
    parent=ROOT/'outputs/generic-blend-resource-reference-20261028/verification-y6zhwxfp/results.json'
    need(sha(parent)=='62bae65c5670fcf20ef6336192610c372009bf30d928e417877e1a72b5c27146','accepted original generic routing receipt')
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
    sources=closure([ROOT/'work/probe_vertexlit_generic_preparation.cpp',ROOT/'work/build_vertexlit_generic_preparation.cmd',ROOT/'work/probe_generic_blend_resource_reference.cpp',ROOT/'work/probe_private_boundary_observer_self_test.cpp',Path(__file__)])
    for module in tuple(sys.modules.values()):
        path=getattr(module,'__file__',None)
        if path and Path(path).resolve().parent==ROOT/'work' and Path(path).suffix=='.py':
            sources[str(Path(path).resolve())]=sha(path)
    image,static=check_ranges(STD,'stdshader_dx9-disassembly.txt',[
        ('whole_blend_classifier',0x10002d20,0x10002da6),
        ('whole_texture_alpha_predicate',0x10064d30,0x10064dce),
        ('whole_snapshot_alpha_flag',0x10064dd0,0x10064dd9),
        ('generic_preparation_prefix_ONLY',0x10055320,0x100557b0),
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
    mapping_source=parent.parent/'mapping.bin'
    need(sha(mapping_source)==prior['mapping_sha256'],'accepted actual-built mapping artifact')
    mapping=run/'mapping.bin';mapping.write_bytes(mapping_source.read_bytes());protected[str(mapping_source)]=sha(mapping_source)
    rows=corpus();query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<25I',*r) for r in rows))
    expected,coverage=prediction(rows);(run/'analytical.bin').write_bytes(expected)
    started=time.time()
    built=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_vertexlit_generic_preparation.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180)
    (run/'build-driver.log').write_text(built.stdout+built.stderr)
    need(built.returncode==0,'fresh original/owned/two existing regression builds')
    executables={}
    for tag,name,machine,magic,logname in [
        ('reference','vertexlit_generic_preparation_x86.exe',0x14c,0x10b,'reference-build.log'),
        ('owned','probe_vertexlit_generic_preparation_x64.exe',0x8664,0x20b,'probe_vertexlit_generic_preparation-build.log'),
        ('dispatch_regression','probe_generic_blend_resource_reference_x64.exe',0x8664,0x20b,'probe_generic_blend_resource_reference-build.log'),
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
        need(f'PASS extended_preparation_cases={coverage["cases"]} events={coverage["events"]} generic_prefix_cases={coverage["generic_prefix_cases"]}' in result.stdout,'exact entry/live-state/nested coverage '+tag)
        runs.append(dict(tag=tag,output=str(trace),sha256=sha(trace),stdout=result.stdout.strip()))
    target=run/'dispatch-regression.bin'
    regression=subprocess.run([executables['dispatch_regression']['path'],str(parent.parent/'mapping.bin'),str(parent.parent/'queries.bin'),str(target)],cwd=run,capture_output=True,text=True,timeout=90)
    (run/'dispatch-regression.log').write_text(regression.stdout+regression.stderr)
    need(regression.returncode==0 and sha(target)==prior['analytical_sha256'] and 'PASS blend_resource_cases=5088' in regression.stdout,'fresh5088-case blend regression')
    regressions={'dispatch_regression':regression.stdout.strip()}
    need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in protected.items()),'all source/prior artifacts unchanged after')
    need(all(sha(p)==h for p,h in originals.items()),'original/private inputs unchanged after')
    need(all(sha(p)==h for p,h in prior['editor_dependencies'].items()) and sha(prior['editor'])==prior['editor_sha256'],'313 editor dependencies and exact executable unchanged')
    outcome=dict(status='VERIFIED connected material preparation through static/dynamic boundary for stated VertexLit route; render bodies INCOMPLETE',
        utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',parent=str(parent),parent_sha256=sha(parent),
        source=sources,protected=protected,original_inputs=originals,executables=executables,runs=runs,coverage=coverage,
        complete_trace_bytes=len(expected),query_sha256=sha(query),analytical_sha256=sha(run/'analytical.bin'),
        compiler_warnings=0,regressions=regressions,observer_self_test=selftest.stdout.strip(),mapping_sha256=sha(mapping),static_ranges=[{k:v for k,v in r.items() if k!='instructions'} for r in static],
        editor=prior['editor'],editor_sha256=prior['editor_sha256'],editor_dependencies=prior['editor_dependencies'],editor_rebuilt=False,
        findings=[
          'Whole original generic preparation55320..557B0 executes in connected concrete14/router cases, including complete blend and texture-alpha methods. Checkpoint557B0 precedes both static shadow setup and dynamic render bodies; neither body executes. Final function cleanup remains bypassed by a declared private-thread checkpoint.',
          'Fifteen original local values plus three captured route arguments agree with independent prediction and owned x64 post-blend preparation for installed VertexLitGeneric, vertexLit1, both flashlight states, flashlightMode0, detail mode0, null context, valid registered schema. Both shared-shadow flag and API flashlight-query paths are covered; no shadow object method is called.',
          'The retained input list supplies post-blend feature values despite callback changes to the shared list. Signed mode clamps to0..2; boolean uses nonzero; float !=zero includes NaN whereas >zero excludes it. Quiet NaN, infinity, negative, signedzero and positive controls are covered under masked FP exceptions.',
          'Owned helper covers only the stated post-blend route, not the full original generic function. Unlit routes, nonzero flashlight modes, absent-schema variants, hardware-dependent detail modes and cached-context reuse remain unverified. Four integers and two float inputs are independently varied; envmap and mask texture type gates are exercised. DC is absent in this installed schema and remains zero.',
          'Fresh builds and observer self-test, repeated full traces, and freshly compiled5088-case preceding blend regression pass. Original inputs, prior artifacts,313 editor dependencies and executable unchanged. No original process access, original writes, dependency changes, publication or GPU/image-fidelity claim.'])
    receipt=run/'results.json';receipt.write_text(json.dumps(outcome,indent=2)+'\n')
    (OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt),receipt_sha256=sha(receipt)),indent=2)+'\n')
    print(json.dumps(dict(status=outcome['status'],receipt=str(receipt),coverage=coverage,trace_bytes=len(expected),editor_rebuilt=False),indent=2),flush=True)

if __name__=='__main__':
    main()
