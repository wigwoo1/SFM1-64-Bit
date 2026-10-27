"""Paired original material/proxy caller and owned x64 lifetime verification.

No live SFM. Original code is private, unpatched RX; synthetic services are
explicit spies, not implementations of the named built-in material proxies.
"""
import hashlib,json,re,shlex,shutil,struct,subprocess,tempfile,time,zlib
from datetime import datetime,timezone
from pathlib import Path
from verify_material_numeric_value import ROOT,PRIVATE,require,sha
from verify_material_keyvalues_parser import DUMP
from verify_builtin_error_material_seed import source_closure
from verify_material_object_parameters import node,string,packet,text,render
from verify_ifm_draw_packet import check_ranges

OUT=ROOT/'outputs/material-proxy-lifetime-20261026'
LIBS=ROOT/'outputs/material-fallback-owned-20261026/verification-0mhr77s9/copied-libraries'
CHANGED=['material_object_store_candidate.h','material_object_parameter_initialization_candidate.h']
NEW=['material_proxy_lifetime_candidate.h','probe_material_proxy_lifetime.cpp','build_material_proxy_lifetime.cmd','verify_material_proxy_lifetime.py']

def corpus():
    cases=[];files={}
    def declaration(var='$alpha',tag='one',key='Bind'):
        return node(key,[string('var',var),string('tag',tag)])
    for shader in ['UnlitGeneric','UnlitTwoTexture','LightmappedGeneric','WorldVertexTransition']:
        for route in range(3):
            for queued in range(2):
                for variant in range(8):
                    fields=[string('$alpha','.375'),string('$temp','[.25 .5]'),string('$basetexture','fixture/base'),string('$texture2','fixture/second')]
                    active=[]
                    if variant==1:fields+=[node('Proxies')]
                    if variant==2:active=[declaration(),declaration('$TEMP','two')];fields+=[node('Proxies',active)]
                    if variant==3:
                        active=[declaration(n,str(i)) for i,n in enumerate(['$ALPHA','$temp','$basetexture','$basetexturetransform','$texture2','$absent',' $alpha','alpha','$alpha[0]',''])];fields+=[node('Proxies',active)]
                    if variant==4:active=[string('Scalar','literal'),declaration('$alpha','after-scalar')];fields+=[node('Proxies',active)]
                    if variant==5:
                        active=[declaration('$TEMP','override'),declaration('$alpha','override2')];fields+=[node('Proxies',[declaration('$alpha','ignored-root')]),node('GPU<1',[node('Proxies',active)])]
                    if variant==6:active=[declaration('$alpha','ignored-root')];fields+=[node('Proxies',active),node('GPU<1',[string('$alpha','.375')])]
                    if variant==7:active=[declaration('$alpha','root')];fields+=[node('Proxies',active),node('GPU>=1',[node('Proxies',[declaration(key='Missing')])])]
                    i=len(cases);name='proxy/case'+str(i);root=node(shader,fields);cases.append(dict(name=name,root=root,route=route,queued=queued,variant=variant,active=active))
                    if route==2:files['materials/'+name+'.vmt']=render(root).encode('ascii')
    for route in range(3):
        for queued in range(2):
            active=[declaration('$alpha',str(i)) for i in range(255)];i=len(cases);name='proxy/case'+str(i);root=node('UnlitTwoTexture',[string('$alpha','.375'),node('Proxies',active)]);cases.append(dict(name=name,root=root,route=route,queued=queued,variant=255,active=active))
            if route==2:files['materials/'+name+'.vmt']=render(root).encode('ascii')
    for route in range(3):
        for queued in range(2):
            for scalar in [False,True]:
                i=len(cases);name='proxy/case'+str(i);local=string('Proxies','literal') if scalar else node('Proxies');root=node('UnlitTwoTexture',[string('$alpha','.375'),node('Proxies',[declaration()]),node('GPU<1',[local])]);cases.append(dict(name=name,root=root,route=route,queued=queued,variant='empty-override-hides-root',active=[]))
                if route==2:files['materials/'+name+'.vmt']=render(root).encode('ascii')
    return cases,files

def independent(cases,trace):
    sections=trace.split('CASE ')[1:];require(len(sections)==len(cases),'Every paired case recorded');checks=0
    for index,(case,section) in enumerate(zip(cases,sections)):
        lines=section.splitlines();require(int(lines[0])==index,'Case identity');lines=lines[1:];n=len(case['active'])
        created=[line for line in lines if line.startswith('C ')];inits=[line for line in lines if line.startswith('I ')];deletes=[line for line in lines if line.startswith('D ')];states=[line for line in lines if line.startswith('S ')];bindings=[shlex.split(line) for line in lines if line.startswith('B ')]
        require(len(created)==len(inits)==len(deletes)==len(bindings)==2*n,'Create/init/bind/release cardinality');checks+=1
        require([int(x.split()[1]) for x in deletes]==list(range(n,0,-1))+list(range(2*n,n,-1)),'Reverse teardown in both generations');checks+=1
        require(all(x.endswith(' 0 0') for x in deletes),'Parameters unavailable before Release');checks+=1
        expectedStates=[(0,[]),(1,list(range(1,n+1))),(2,list(range(1,n+1))),(3,[]),(4,list(range(n+1,2*n+1)))]+([(5,list(range(n+1,2*n+1)))] if case['queued'] else [])+[(6,[])]
        require(states==['S '+str(stage)+' '+str(len(ids))+(' '+' '.join(map(str,ids)) if ids else '') for stage,ids in expectedStates],'Guard/preserve/uncache/reinit/queue states');checks+=1
        for row in bindings:
            request=row[2];key=request.lower();found=key in ['$alpha','$temp','$basetexture','$basetexturetransform','$texture2'];require(row[3]==str(int(found)),'No synthetic missing/component bindings');checks+=1
            if not found:continue
            require(row[4].lower()==key,'Bound actual variable name');checks+=1
            if key=='$alpha':require(int(row[7])==0x3ec00000,'Analytical alpha .375');checks+=1
            if key=='$temp':require([int(x) for x in row[7:11]]==[0x3e800000,0x3f000000,0,0],'Analytical two-component vector');checks+=1
            if key in ['$basetexture','$texture2']:require(row[11]==('fixture/base' if key=='$basetexture' else 'fixture/second'),'Texture declaration binding, not loaded GPU resource');checks+=1
            if key=='$basetexturetransform':require(row[12]=='1' and [int(x) for x in row[14:]]==[0x3f800000 if i%5==0 else 0 for i in range(16)],'Analytical identity matrix');checks+=1
    return checks

def main():
    preflight=json.loads((OUT/'preflight.json').read_text());priorPointer=preflight['prior'];priorPath=Path(priorPointer['receipt']);require(sha(priorPath)==priorPointer['receipt_sha256'],'Prior receipt identity');prior=json.loads(priorPath.read_text());require(prior['status']=='PASS','Prior accepted verification')
    replay=Path(priorPointer['replay']);require(sha(replay/'manifest.json')==priorPointer['replay_manifest_sha256'],'Prior frozen manifest');frozen=json.loads((replay/'manifest.json').read_text());require(all(sha(replay/p)==h for p,h in frozen.items()),'Prior frozen files unchanged')
    changed=[Path(p).name for p,h in prior['sources'].items() if sha(Path(p))!=h];require(set(changed)==set(CHANGED),'Only bounded production edit set')
    for name in CHANGED:require(sha(OUT/'before/work'/name)==prior['sources'][str(ROOT/'work'/name)],'Preserved old source '+name)
    sources=source_closure([Path(p) for p in prior['sources']]+[ROOT/'work'/n for n in NEW]);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));
    for group in [prior['original_hashes'],prior['private_library_hashes'],prior['benchmark_material_hashes']]:
        require(all(sha(Path(p))==h for p,h in group.items()),'Protected originals/benchmarks unchanged')
    image,ranges=check_ranges(PRIVATE,'materialsystem-disassembly.txt',[
        ('proxy_init',0x10002c50,0x10002dcd),('proxy_release',0x10002dd0,0x10002e2d),('find_var',0x10004520,0x100045e2),('forward_find_var',0x10007610,0x1000761e),('uncache',0x10005d10,0x10005db1),('first_child',0x10070870,0x1007087b),('next_child',0x10070880,0x1007088b),('set_kv_chain',0x100705e0,0x100705ed),('missing_key_chain_lookup',0x10070b55,0x10070b6e),('override_chain_setup',0x1000705f,0x10007095)])
    require(image.word(0x10095a9c)==0x10007520 and image.word(0x10095a9c+44)==0x10007610,'Actual forward vtable identities')
    started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_proxy_lifetime.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=300);(run/'build-driver.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Fresh isolated proxy and full regression probe builds');exes=[];logs={}
    native=['material_proxy_lifetime','unlit_two_texture_parameters','world_transition_parameters','lightmapped_parameters','material_object_parameters'];owned=native+['material_store']
    for suffix,machine,stems,tag in [('reference_x86',0x14c,native,'native'),('owned_x64',0x8664,owned,'owned')]:
        for stem in stems:
            exe=OUT/(stem+'_'+suffix+'.exe');b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];require(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==(0x10b if machine==0x14c else 0x20b) and b'asInvoker' in b,'Fresh PE architecture '+exe.name)
            imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require({s.lower() for s in modules}=={'kernel32.dll'},'Runtime imports '+exe.name);exes.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules))
            log=OUT/(stem+'-'+tag+'-build.log');require(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log.read_text(),re.I),'Clean build '+exe.name);logs[str(log)]=sha(log)
    logs[str(OUT/'asm-build.log')]=sha(OUT/'asm-build.log')
    cases,files=corpus();mount=run/'mount';mount.mkdir()
    for name,data in files.items():p=mount/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
    fixtures={str(p):sha(p) for p in mount.rglob('*') if p.is_file()};query=run/'query.bin';query.write_bytes(struct.pack('<I',len(cases))+b''.join(struct.pack('<2I',c['route'],c['queued'])+text(c['name'])+packet(c['root']) for c in cases));(run/'cases.json').write_text(json.dumps(cases,indent=2))
    def execute(stem,native,query,dest,mount):
        exe=OUT/(stem+('_reference_x86.exe' if native else '_owned_x64.exe'));p=subprocess.run([str(exe),str(PRIVATE if native else run/'absent-original.dll'),str(LIBS if native else run/'absent-original-directory'),str(mount),str(query),str(dest)],cwd=run,capture_output=True,text=True,timeout=180);(run/(dest.stem+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,exe.name+' exit='+str(p.returncode)+'; inspect '+dest.stem+'.log');return p.stdout
    a=run/'reference.txt';nativeExecution=execute('material_proxy_lifetime',True,query,a,mount);b=run/'owned.txt';execute('material_proxy_lifetime',False,query,b,mount)
    first=json.loads((OUT/'first-comparison.json').read_text());oldQuery=Path(first['query']);oldReference=Path(first['reference']);require(sha(oldQuery)==first['query_sha256'] and sha(oldReference)==first['reference_sha256'],'Initial failing comparison retained');require(query.read_bytes()[4:oldQuery.stat().st_size]==oldQuery.read_bytes()[4:] and a.read_bytes().split(b'CASE 198\n')[0]==oldReference.read_bytes(),'Original198 inputs/observations unchanged after selector correction')
    require(a.read_bytes()==b.read_bytes(),'Full paired proxy creation/bind/teardown traces');checks=independent(cases,a.read_text());counts=re.search(r'native_allocations=(\d+) releases=(\d+)',nativeExecution);require(counts and counts[1]==counts[2],'Paired native allocations balanced')
    boundary=run/'native-boundaries.txt';boundaryExecution=execute('material_proxy_lifetime',True,'--native-boundaries',boundary,mount);require('native_proxy_boundaries=6' in boundaryExecution,'All original boundary scenarios');boundaryCounts=re.search(r'allocations=(\d+) releases=(\d+)',boundaryExecution);require(boundaryCounts and boundaryCounts[1]==boundaryCounts[2],'Native boundary fixture cleanup balanced')
    boundaryText=boundary.read_text();mixed=boundaryText.split('CASE 0\n')[1].split('CASE 1\n')[0];require('C 2 Missing\nW 1\nC 3 Fail\n' in mixed and 'D 3 1 0\nW 2\nC 4 Bind\n' in mixed and 'D 4 0 0\nD 1 0 0\n' in mixed,'Native best-effort failure warning/order and reverse survivor cleanup');require(all(x in boundaryText for x in ['BOUNDARY switched_factory_used=1','BOUNDARY null_factory_retains_list=1','BOUNDARY count256_wraps_and_leaks=1']),'Unsafe native boundaries explicitly observed')
    regressions=[];previous=prior;previousPath=priorPath
    for stem in ['unlit_two_texture_parameters','world_transition_parameters','lightmapped_parameters','material_object_parameters']:
        oldrun=previousPath.parent;oldquery=oldrun/'query.bin';require(sha(oldquery)==previous['query_sha256'],'Prior corpus identity')
        for native in [True,False]:
            dest=run/(stem+('-reference.bin' if native else '-owned.bin'));execute(stem,native,oldquery,dest,oldrun/'mount');require(sha(dest)==previous['output_sha256'],'Whole accepted corpus unchanged '+dest.name);regressions.append(dict(executable=stem,native=native,cases=previous['cases'],records=previous['records'],output_sha256=sha(dest)))
        previousPath=Path(previous['prior_receipt']);previous=json.loads(previousPath.read_text())
    storeQuery=previousPath.parent/'query.txt';require(sha(storeQuery)==previous['query_sha256'],'Original ownership query');dest=run/'store-regression.txt';execute('material_store',False,storeQuery,dest,mount);require(sha(dest)==previous['trace_sha256'],'6064-operation ownership trace unchanged')
    safety={}
    for stem,arg,key in [('material_proxy_lifetime','--test-proxies','proxies'),('unlit_two_texture_parameters','--test-unlit-two-texture','two'),('world_transition_parameters','--test-world-transition','world'),('lightmapped_parameters','--test-lightmapped','lightmapped'),('material_object_parameters','--test','parameters'),('material_store','--test','ownership')]:
        p=subprocess.run([str(OUT/(stem+'_owned_x64.exe')),arg],cwd=run,capture_output=True,text=True,timeout=120);(run/(key+'-safety.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,'Safety '+key);safety[key]=p.stdout
    require(all(safety[k]==prior['safety'][k] for k in ['two','world','lightmapped','parameters','ownership']),'Every prior safety count unchanged')
    pilotCopy=priorPath.parent/'mount/materials/pilot-original.vmt';require(sha(pilotCopy)==prior['pilot_projection']['original_sha256'],'Untouched disposable pilot copy');p=subprocess.run([str(OUT/'unlit_two_texture_parameters_owned_x64.exe'),'--test-pilot-refusal',str(pilotCopy)],cwd=run,capture_output=True,text=True,timeout=30);(run/'pilot-refusal.log').write_text(p.stdout+p.stderr);require(p.returncode==0 and p.stdout==prior['pilot_refusal'],'Full original pilot remains explicitly refused')
    build=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());graph=set()
    def visit(p):
        name=p.relative_to(ROOT).as_posix()
        if name in graph:return
        graph.add(name)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
            q=p.parent/inc
            if q.is_file():visit(q.resolve())
    visit((ROOT/'work/probe_vulkan_win32_surface_x64.cpp').resolve());require(graph==set(build['editor_source_dependencies']) and all(sha(ROOT/k)==h for k,h in build['editor_source_dependencies'].items()),'Complete editor dependency graph unchanged');require(sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==build['executable_sha256'],'Editor binary unchanged')
    for group in [sources,prior['original_hashes'],prior['private_library_hashes'],prior['benchmark_material_hashes'],fixtures]:require(all(sha(Path(p))==h for p,h in group.items()),'Protected identities after execution')
    require(not (run/'absent-original.dll').exists() and not (run/'absent-original-directory').exists(),'Owned independence arguments absent')
    capture=run/'complete-traces.txt.zlib';capture.write_bytes(zlib.compress(a.read_bytes(),9));require(zlib.decompress(capture.read_bytes())==a.read_bytes(),'Lossless trace capture')
    result=dict(status='PASS',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',scope='Actual material proxy factory/Init/FindVar/reverse release and safe owned store lifetime; built-in evaluation remains incomplete',cases=len(cases),events=sum(1 for line in a.read_text().splitlines() if not line.startswith('CASE ')),exact_bytes=a.stat().st_size,query_sha256=sha(query),output_sha256=sha(a),independent_checks=checks,native_allocations=int(counts[1]),native_releases=int(counts[2]),native_boundary_scenarios=6,native_boundary_allocations=int(boundaryCounts[1]),native_boundary_releases=int(boundaryCounts[2]),safety=safety,regressions=regressions,store_regression=dict(operations=6064,trace_sha256=sha(dest)),sources=sources,changed_sources=CHANGED,new_sources=NEW,original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],benchmark_material_hashes=prior['benchmark_material_hashes'],fixture_hashes=fixtures,executables=exes,build_logs=logs,static_ranges=ranges,prior_receipt=str(priorPath),prior_receipt_sha256=sha(priorPath),editor_dependencies_unchanged=len(graph),editor_sha256=build['executable_sha256'],pilot_refusal=p.stdout,limits=['Opt-in factory service and read-only stable bindings only; no built-in proxy evaluation or mutation, texture loads, GPU resources or renderer integration','Default factory absent: nonempty Proxies remain refused; store rejects partial initialization transactionally even though original skips failures','Owned >255 declarations refuse before callbacks; original256 overflow and null/switched-factory teardown hazards are recorded with explicit host-fixture cleanup','Owner-thread serialized API; callbacks must not destroy the store itself, and factory destroy must be noexcept','No live SFM, Blender or original writes; no whole-project completion or visual-equivalence claim'])
    # Do not accidentally record the last unrelated subprocess stdout as pilot
    # evidence: preserve the actual refusal log explicitly.
    result['pilot_refusal']=(run/'pilot-refusal.log').read_text()
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');replay=OUT/('replay-'+run.name.removeprefix('verification-'));replay.mkdir()
    included=[Path(p) for p in sources]+[Path(e['path']) for e in exes]+[Path(p) for p in logs]+[receipt,query,capture,OUT/'preflight.json',OUT/'first-comparison.json',oldQuery,oldReference,oldReference.parent/'owned.txt',oldReference.parent/'reference.log',oldReference.parent/'owned.log']+list(run.glob('*.json'))+list(run.glob('*.log'))+[boundary]+[Path(p) for p in fixtures]+[OUT/'before/work'/n for n in CHANGED]
    for p in included:
        t=replay/p.relative_to(ROOT);t.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,t)
    manifest={p.relative_to(replay).as_posix():sha(p) for p in replay.rglob('*') if p.is_file()};(replay/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');pointer=dict(receipt=str(receipt),receipt_sha256=sha(receipt),replay=str(replay),replay_manifest_sha256=sha(replay/'manifest.json'));(OUT/'latest-verification.json').write_text(json.dumps(pointer,indent=2)+'\n');print(json.dumps(dict(status='PASS',receipt=str(receipt),cases=len(cases),exact_bytes=a.stat().st_size,independent_checks=checks,safety=safety['proxies'])))

if __name__=='__main__':main()
