"""Retain the actual selected displacement mesh for material-property binding.
Native whole world wrapper uses controlled, distinct per-mesh property pairs;
owned geometry reads the BSP and independently supplies mesh identity.
"""
import collections,hashlib,json,math,re,shutil,struct,subprocess,tempfile
from pathlib import Path
from verify_ifm_draw_packet import check_ranges
from verify_controlled_world_geometry import fixture_world,fixture_mesh
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/world-displacement-surface-20261016'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
PRIOR=ROOT/'outputs/world-trace-surface-20261015/verification-qxnt2kzn/results.json'
ASSETS=ROOT/'outputs/loose-collision-scene-20261016/verification-ub8mvw0l/results.json'
DUMP=Path('C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/dumpbin.exe')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def main():
    prior=json.loads(PRIOR.read_text());assets=json.loads(ASSETS.read_text());require(prior['status']==assets['status']=='PASS' and not prior['smoke'],'Prior proof status');originals=assets['original_hashes']
    for p,s in originals.items():require(sha(Path(p))==s,'Original identity '+p)
    old=PRIOR.parent;require(sha(old/'prepared.bin')==prior['prepared_sha256'] and sha(old/'queries.bin')==prior['query_sha256'],'Prior fixture identity');frozen=(old/'surface_native_x86.bin').read_bytes();require(len(frozen)==prior['cases']*76,'Frozen numeric/flags span');frozenSha=sha(old/'surface_native_x86.bin')
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));prepared=run/'prepared.bin';shutil.copyfile(old/'prepared.bin',prepared);rows=bytearray((old/'queries.bin').read_bytes());data=prepared.read_bytes();at=0;meshes=[]
    for i in range(43):
        power,contents,flags=struct.unpack_from('<3I',data,at);cells=1<<power;vc=(cells+1)**2;tc=2*cells*cells;corners=[struct.unpack_from('<3f',data,at+12+12*j) for j in range(4)];positions=[struct.unpack_from('<3f',data,at+60+12*j) for j in range(vc)];triangles=[struct.unpack_from('<3H',data,at+60+16*vc+8*j) for j in range(tc)];meshes.append((power,contents,flags,corners,positions,triangles));at+=60+16*vc+8*tc
    require(at==len(data),'Prepared43 mesh span');directed=0
    for power,contents,flags,corners,positions,triangles in meshes:
        a=[corners[3][j]-corners[0][j] for j in range(3)];b=[corners[1][j]-corners[0][j] for j in range(3)];normal=[a[(j+1)%3]*b[(j+2)%3]-a[(j+2)%3]*b[(j+1)%3] for j in range(3)];length=math.sqrt(sum(x*x for x in normal));require(length>0,'Degenerate fixture face');normal=[x/length for x in normal]
        for t in sorted(set([0,1,len(triangles)//4,len(triangles)//2,len(triangles)-2,len(triangles)-1])):
            center=[sum(positions[index&511][j] for index in triangles[t])/3 for j in range(3)];start=[f32(center[j]+normal[j]*2) for j in range(3)];end=[f32(center[j]-normal[j]*2) for j in range(3)];delta=[f32(end[j]-start[j]) for j in range(3)]
            for mask in [0,1,128,0x4081]:rows+=struct.pack('<6f3I',*start,*delta,mask,1,1);directed+=1
    query=run/'queries.bin';query.write_bytes(rows);count=len(rows)//36;require(len(rows)==count*36,'Query span');executables=[]
    for name in ['world_native_x86','world_owned_x64','controlled_native_x86','controlled_owned_x64','binding_safety_x64','surface_safety_x64','world_safety_x64']:
        exe=OUT/(name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];machine=0x14c if name.endswith('x86') else 0x8664;require(binary[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',binary,pe+4)[0]==machine and b'asInvoker' in binary,'Executable identity '+name)
        imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(name+'-imports.txt')).write_text(imports);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require(modules and set(x.lower() for x in modules)=={'kernel32.dll'},'Unexpected DLL dependency '+name);executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules))
    image,evidence=check_ranges(GAME/'bin/engine.dll','engine-disassembly.txt',[('displacement_world_commit',0x1014d010,0x1014d0f8),('whole_world_wrapper',0x10149eb0,0x1014a12c)])
    require(image.string(0x10350ee0)=='**displacement**','Native displacement name identity');(run/'original-code-evidence.json').write_text(json.dumps(evidence,indent=2))
    def execute(name,args,log):
        r=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=120);log.write_text(r.stdout+r.stderr);require(r.returncode==0,'Probe failed '+str(log)+' '+r.stdout+r.stderr);return r.stdout.strip()
    bsp=GAME/'tf_movies/maps/mtt_soldier.bsp';a=run/'native.bin';b=run/'owned.bin';nativeLog=execute('world_native_x86',[bsp,prepared,query,a,GAME/'bin/engine.dll'],run/'native.log');print(nativeLog,flush=True);ownedLog=execute('world_owned_x64',[bsp,prepared,query,b],run/'owned.log');print(ownedLog,flush=True);native,owned=a.read_bytes(),b.read_bytes();require(len(native)==len(owned)==count*76,'Numeric/surface output span')
    require(native==owned,'Existing world numeric/flag outputs differ');require(native[:len(frozen)]==frozen,'Prior35157 numeric/flag regression');sa=Path(str(a)+'.displacement-surface').read_bytes();sb=Path(str(b)+'.displacement-surface').read_bytes();require(len(sa)==len(sb)==count*8,'Displacement sidecar span')
    if sa!=sb:
        first=next(i for i in range(count) if sa[i*8:(i+1)*8]!=sb[i*8:(i+1)*8]);(run/'difference.json').write_text(json.dumps(dict(query=first,ray=struct.unpack_from('<6f3I',rows,first*36),native=struct.unpack_from('<2I',sa,first*8),owned=struct.unpack_from('<2I',sb,first*8)),indent=2));raise RuntimeError('Displacement identity/property mismatch '+str(run/'difference.json'))
    coverage=collections.Counter();selected=collections.Counter();stationaryIds=set();selectors=set()
    for i in range(count):
        surface,flags=struct.unpack_from('<2I',native,i*76+68);index,prop=struct.unpack_from('<2I',sa,i*8);dispFlags=struct.unpack_from('<I',native,i*76+36)[0];stationary=not any(struct.unpack_from('<3f',rows,i*36+12))
        if surface==0xfffffffe:
            require(index<43 and prop==2*index+(2 if dispFlags&16 else 1),'Native property/mesh selection');coverage['displacement']+=1;selected[index]+=1;selectors.add(bool(dispFlags&16));coverage['stationary_displacement']+=stationary
            if stationary:stationaryIds.add(index)
        else:require((index,prop)==(0xffffffff,0),'Non-displacement surface invented mesh/property');coverage['non_displacement']+=1
        coverage['stationary']+=stationary
    require(len(selected)>1 and selectors=={False,True},'Benchmark displacement identity/selector coverage');coverage['distinct_meshes']=len(selected);coverage['distinct_stationary_meshes']=len(stationaryIds)
    # The real benchmark's stationary normal probes never finish with a terrain
    # surface. Reuse the previously proved folded-strip geometry, with a second
    # translated/window-content mesh, to exercise retained stationary identity.
    case=run/'controlled-folded';case.mkdir();world=case/'world.bsp';world.write_bytes(fixture_world());first=fixture_mesh(2,True,1,0);second=bytearray(fixture_mesh(2,True,128,0));vc=25
    for at in [12+j*12 for j in range(4)]+[60+j*12 for j in range(vc)]:y=struct.unpack_from('<f',second,at+4)[0];struct.pack_into('<f',second,at+4,y+256)
    mesh=case/'meshes.bin';mesh.write_bytes(first+second);cq=bytearray()
    for y in [16.,272.]:
        for x in [-32.,32.,80.,128.]:
            for z in [-16.,-.5,0.,.5,16.]:
                for mask in [0,1,128,0x4000,0x4081]:cq+=struct.pack('<6f3I',x,y,z,0,0,0,mask,1,1)
    for y in [16.,272.]:
        for x in [-32.,32.,80.,128.]:
            for z in [-16.,16.]:cq+=struct.pack('<6f3I',x,y,z,0,0,-2*z,0x4081,1,1)
    ci=case/'queries.bin';ci.write_bytes(cq);ca=case/'native.bin';cb=case/'owned.bin';print(execute('controlled_native_x86',[world,mesh,ci,ca,GAME/'bin/engine.dll'],case/'native.log'),flush=True);print(execute('controlled_owned_x64',[world,mesh,ci,cb],case/'owned.log'),flush=True);require(ca.read_bytes()==cb.read_bytes(),'Controlled numeric/flags mismatch');cm=Path(str(ca)+'.displacement-surface').read_bytes();require(cm==Path(str(cb)+'.displacement-surface').read_bytes() and len(cm)==len(cq)//36*8,'Controlled displacement metadata mismatch');cc=collections.Counter();witnesses=set()
    for i in range(len(cq)//36):
        q=struct.unpack_from('<6f3I',cq,i*36);index,prop=struct.unpack_from('<2I',cm,i*8)
        if index==0xffffffff:continue
        require(index<2,'Controlled mesh identity');cc['displacement']+=1
        if not any(q[3:6]):cc['stationary_displacement']+=1;cc['stationary_mesh_'+str(index)]+=1
        if q[0]==80 and q[2]==-16 and q[3:6]==(0.,0.,0.) and (q[1],q[6]) in [(16.,1),(272.,128)]:require(index==(0 if q[1]==16 else 1),'Folded-strip identity witness');witnesses.add(index)
    require(cc['stationary_mesh_0']>0 and cc['stationary_mesh_1']>0 and witnesses=={0,1},'Controlled stationary displacement coverage')
    controlled=dict(queries=len(cq)//36,coverage=dict(cc),path=str(case),world_sha256=sha(world),meshes_sha256=sha(mesh),queries_sha256=sha(ci),result_sha256=sha(cb),metadata_sha256=sha(Path(str(cb)+'.displacement-surface')))
    safety={}
    for name,args in [('binding_safety_x64',[]),('surface_safety_x64',[bsp]),('world_safety_x64',[bsp])]:safety[name]=execute(name,args,run/(name+'.log'));print(safety[name],flush=True)
    for p,s in originals.items():require(sha(Path(p))==s,'Original changed after test '+p)
    require(sha(old/'surface_native_x86.bin')==frozenSha,'Frozen reference changed')
    names=['source_displacement_trace_surface_properties_candidate.h','test_displacement_trace_surface_properties.cpp','build_world_displacement_surface_identity.cmd','verify_world_displacement_surface_identity.py','source_prepared_world_ray_candidate.h','source_collision_surface_candidate.h','probe_swept_world_trace.cpp'];changed={str(ROOT/'work'/p):sha(ROOT/'work'/p) for p in names};sources=dict(changed);pending=[ROOT/'work'/p for p in names]+[ROOT/'work'/p for p in ['test_world_trace_surface.cpp','test_world_ray_trace.cpp']];visited=set()
    while pending:
        p=pending.pop()
        if p in visited:continue
        visited.add(p);sources[str(p)]=sha(p)
        if p.suffix in ['.h','.cpp']:
            for local in re.findall(r'^\s*#include\s*"([^"]+)"',p.read_text(),re.M):
                child=p.parent/local
                if child.is_file():pending.append(child)
    logs=[]
    for p in OUT.glob('*-build.log'):
        content=p.read_text();require(not re.search(r'\b(?:warning|error) [A-Z]+\d+',content),'Compiler diagnostic '+str(p));logs.append(dict(path=str(p),sha256=sha(p),warnings=0,errors=0))
    limits=['Displacement surface identity now retains owned mesh ordinal; explicit supplied property pairs selected by actual trace flag16. Material/VMT-to-pair resolution still required.', 'Original whole world wrapper executes in private engine PE with existing controlled work-pool/release/memset host hooks and prepared native meshes; no live SFM/startup or combined prop scene.', 'Distinct 1..86 runtime properties and reversed brush surface IDs are controlled test bindings. These are not actual benchmark material properties.', '76 prior geometry/positions/surface identity/flag bytes verified; remaining complete world-trace metadata (including plane tags and real material bindings) is still unverified.', 'No static-prop lighting guard removal/editor integration/build, renderer/animation/serialization/save changes or dependencies installed.']
    result=dict(status='PASS',path=str(run),queries=count,controlled=controlled,prior_queries=prior['cases'],directed_queries=directed,numeric_and_flag_differences=0,displacement_identity_differences=0,property_differences=0,coverage=dict(coverage),mesh_hits=dict(selected),safety=safety,executables=executables,changed_sources=changed,sources=sources,build_logs=logs,original_hashes=originals,prior_76bytes_sha256=frozenSha,queries_sha256=sha(query),prepared_sha256=sha(prepared),results_sha256=sha(b),displacement_sidecar_sha256=sha(Path(str(b)+'.displacement-surface')),reference_receipts={str(p):sha(p) for p in [PRIOR,ASSETS]},limitations=limits)
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2));(OUT/'source-integrity.json').write_text(json.dumps(dict(status='PASS',authoritative_receipt=str(receipt),receipt_sha256=sha(receipt),sources=sources,changed_sources=changed,executables=executables,build_logs=logs,original_hashes=originals),indent=2))
    lines=['VERIFIED selected world displacement identity and supplied property-pair transport.','Whole application PARTIALLY VERIFIED.',str(receipt),'Queries='+str(count)+' original/owned differences=0; prior35157 output bytes unchanged',str(dict(coverage)),'Controlled folded stationary queries='+str(controlled['queries'])+' '+str(controlled['coverage']),*safety.values(),'Original files unchanged='+str(len(originals)),'','Exact freshly built/tested executables:']+[e['path']+' SHA256='+e['sha256'] for e in executables]+['','Changed files:']+list(changed)+['','Scope:']+limits+['','Reproduce:','cmd /c work\\build_world_displacement_surface_identity.cmd','C:\\Users\\wigwo\\AppData\\Local\\Python\\bin\\python3.14.exe work\\verify_world_displacement_surface_identity.py'];(OUT/'verification.txt').write_text('\n'.join(lines)+'\n');(OUT/'remaining-integration.txt').write_text('World+props composition remains pending: discovered world state discarded selected displacement identity, now fixed/verified. Need actual VMT/texture-to-runtime surface/property pairs and remaining full world trace fields (plane tags), then concrete engine point services and original combined-scene comparison before lighting guard removal/editor integration/rebuild.\nProp preparation/runtime clipping and explicit-swept spatial order already verified in prior milestones. Never substitute zero material IDs for unresolved bindings.\nContinue Sol High; goal active, no switch/pause/whole-project percentage.\n');print('PASS '+str(receipt),flush=True)
if __name__=='__main__':main()
