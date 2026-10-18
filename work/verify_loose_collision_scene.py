"""Configured scripts/MDL/PHY -> real surface IDs + runtime-state prop clipping.
Fresh original surface/cache and original engine/physics clips are separate
reference processes. No full world/prop scene or editor integration claim.
"""
import collections,hashlib,json,re,shutil,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/loose-collision-scene-20261016'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
SURFACE=ROOT/'outputs/studio-surface-properties-20261016/verification-gcz0eqku/results.json'
SPATIAL=ROOT/'outputs/static-prop-point-spatial-20261016/verification-lt9fea6j/results.json'
CLIP=ROOT/'outputs/static-prop-point-registry-20261016/verification-72v49r60/results.json'
SCOPE=ROOT/'outputs/studio-vcollide-read-20261016/verification-gr1jjsch/results.json'
DUMP=Path('C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/dumpbin.exe')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def main():
    surface,spatial,clip,scope=[json.loads(p.read_text()) for p in [SURFACE,SPATIAL,CLIP,SCOPE]];require(all(r['status']=='PASS' for r in [surface,spatial,clip,scope]),'Prior proof status');originals=surface['original_hashes']
    for p,s in originals.items():require(sha(Path(p))==s,'Original changed before tests '+p)
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libs=run/'libraries';libs.mkdir()
    for name in ['tier0.dll','vphysics.dll']:shutil.copyfile(GAME/'bin'/name,libs/name)
    mounts=run/'mounts.txt';shutil.copyfile(Path(spatial['path'])/'mounts.txt',mounts);bsp=GAME/'tf_movies/maps/mtt_soldier.bsp'
    executables=[]
    for name in ['clip_native_x86','surface_native_x86','scene_owned_x64','scene_safety_x64','spatial_regression_x64','spatial_safety_x64']:
        exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];machine=0x14c if name.endswith('x86') else 0x8664;require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'Executable identity '+name)
        imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(name+'-imports.txt')).write_text(imports);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.M|re.I);require(modules and set(x.lower() for x in modules)=={'kernel32.dll'},'Unexpected direct DLL dependency '+name);executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),direct_imports=modules))
    def execute(name,args,log):
        r=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=60);log.write_text(r.stdout+r.stderr);require(r.returncode==0,'Probe failed '+str(log)+' '+r.stdout+r.stderr);return r.stdout.strip()
    # Re-run original normal surface parser and actual datacache fill. Its
    # transcript supplies reference IDs ONLY to the original clip oracle.
    oldSurface=next(r for r in surface['records'] if r['name']=='configured-manifest');fixture=Path(oldSurface['path'])/'fixture.bin';require(sha(fixture)==oldSurface['input_sha256'],'Surface reference input identity');nativeSurface=run/'surface-native.bin';print(execute('surface_native_x86',[GAME/'bin/datacache.dll',libs,fixture,nativeSurface],run/'surface-native.log'),flush=True);require(sha(nativeSurface)==oldSurface['result_sha256'],'Fresh native surface/cache changed')
    raw=nativeSurface.read_bytes();n=struct.unpack_from('<I',raw)[0];at=4+4*n+4;q=struct.unpack_from('<I',raw,at)[0];at+=4+4*q;n=struct.unpack_from('<I',raw,at)[0];at+=4;nativeProperties=[]
    for i in range(n):index,bones=struct.unpack_from('<iI',raw,at);at+=8;boneIds=struct.unpack_from('<'+'i'*bones,raw,at);at+=bones*4;nativeProperties.append((index,boneIds))
    require(at==len(raw) and n==81,'Native surface transcript extent');nativeProperties=nativeProperties[:79]
    # Checked benchmark tables provide poses/type/identity only, never results.
    b=bsp.read_bytes();g=struct.unpack_from('<I',b,568)[0]
    for i in range(struct.unpack_from('<I',b,g)[0]):
        tag,_,_,offset,length=struct.unpack_from('<IHHII',b,g+4+i*16)
        if tag==0x73707270:break
    n=struct.unpack_from('<I',b,offset)[0];names=[b[offset+4+i*128:offset+4+(i+1)*128].split(b'\0')[0].decode('ascii') for i in range(n)];at=offset+4+n*128;at+=4+2*struct.unpack_from('<I',b,at)[0];count=struct.unpack_from('<I',b,at)[0];at+=4;props=[b[at+i*60:at+(i+1)*60] for i in range(count)];require((n,count)==(79,485),'Benchmark inventory');assets={r['model']:r for r in scope['records']};dummy=next(r['phy'] for r in scope['records'] if r['phy'])
    oldQueries=CLIP.parent/'queries.bin';require(sha(oldQueries)==clip['query_sha256'],'Prior owned sample identity');rows=oldQueries.read_bytes();require(len(rows)==18430*44,'Clip sample extent');queries=run/'queries.bin';queries.write_bytes(rows);groups=[[] for _ in names];runtime=[]
    for r in props:m=struct.unpack_from('<H',r,24)[0];runtime.append(r[30] if r[30]!=6 or assets[names[m]]['phy'] else 0)
    for i in range(18430):
        id,*values=struct.unpack_from('<Q6fIQ',rows,i*44);ordinal=(id>>32)-1;require(id==((ordinal+1)<<32)|0xdecafbad and ordinal<count,'Sample entity identity');r=props[ordinal];m=struct.unpack_from('<H',r,24)[0];cache=0 if assets[names[m]]['phy'] else 1;origin=struct.unpack_from('<3f',r);angles=struct.unpack_from('<3f',r,12);surfaceId=nativeProperties[m][0]&65535
        wire=struct.pack('<12f5IQ',*values[:6],*origin,*angles,values[6],runtime[ordinal]<<16,cache,0x40000000|ordinal,surfaceId,values[7]);groups[m].append((i,wire))
    expected=bytearray(18430*92);events=bytearray(18430*8);records=[]
    for m,group in enumerate(groups):
        case=run/('native-%03d'%m);case.mkdir();query=case/'queries.bin';query.write_bytes(b''.join(w for _,w in group));output=case/'native.bin';asset=assets[names[m]];execute('clip_native_x86',[GAME/'bin/engine.dll',libs,asset['phy'] or dummy,asset['mdl'],query,output],case/'native.log');data=output.read_bytes();ev=Path(str(output)+'.events').read_bytes();require(len(data)==len(group)*92 and len(ev)==len(group)*8,'Native clip output extent')
        for j,(i,_) in enumerate(group):expected[i*92:(i+1)*92]=data[j*92:(j+1)*92];events[i*8:(i+1)*8]=ev[j*8:(j+1)*8]
        records.append(dict(model=names[m],queries=len(group),path=str(case),trace_sha256=sha(output),query_sha256=sha(query),resolved_surface=nativeProperties[m][0]));
        if (m+1)%10==0:print('PASS fresh original full prop clip models='+str(m+1),flush=True)
    (run/'native-ordered.bin').write_bytes(expected);(run/'native-ordered.events').write_bytes(events)
    owned=run/'owned.bin';print(execute('scene_owned_x64',[bsp,mounts,queries,owned],run/'owned.log'),flush=True)
    def compare(actual,expected,label,stride):
        if actual!=expected:
            first=next((i for i,(x,y) in enumerate(zip(actual,expected)) if x!=y),min(len(actual),len(expected)));(run/'difference.json').write_text(json.dumps(dict(kind=label,byte=first,case=first//stride,field=first%stride,actual_size=len(actual),expected_size=len(expected)),indent=2));raise RuntimeError('Owned '+label+' mismatch '+str(run/'difference.json'))
    compare(owned.read_bytes(),expected,'trace',92);compare(Path(str(owned)+'.events').read_bytes(),events,'events',8)
    modelInfo=bytearray()
    for i,(index,bones) in enumerate(nativeProperties):modelInfo+=struct.pack('<QiI',((i+1)<<32)|0x12345678,index,len(bones))+struct.pack('<'+'i'*len(bones),*bones)
    compare(Path(str(owned)+'.models').read_bytes(),modelInfo,'resolved-model-cache',20)
    totals=collections.Counter();hitCase=None;nondefaultHitCase=None;unknownHits=0
    for i in range(18430):
        dispatch,hit=struct.unpack_from('<II',events,i*8);id=struct.unpack_from('<Q',rows,i*44)[0];ordinal=(id>>32)-1;m=struct.unpack_from('<H',props[ordinal],24)[0];totals['physics_dispatches']+=dispatch;totals['hits']+=hit
        if runtime[ordinal]==0:require(not dispatch and not hit,'Disabled/non-solid prop dispatched or hit');totals['runtime_nonsolid_queries']+=1
        if hit:
            hitCase=i;require(struct.unpack_from('<I',expected,i*92+68)[0]==(nativeProperties[m][0]&65535),'Hit surface annotation');unknownHits+=nativeProperties[m][0]==-1
            if nativeProperties[m][0]!=0:nondefaultHitCase=i
    require(hitCase is not None and totals['hits']==616,'Actual hit coverage changed');totals['unknown_surface_hits']=unknownHits
    # Native x86 cannot represent high64 world identities. Owned output should
    # differ only in that semantic field, with exact remaining trace bytes.
    wide=0x88776655decafbad;wire=bytearray(rows[hitCase*44:(hitCase+1)*44]);struct.pack_into('<Q',wire,36,wide);wideQuery=run/'wide-query.bin';wideQuery.write_bytes(wire);wideOut=run/'wide.bin';execute('scene_owned_x64',[bsp,mounts,wideQuery,wideOut],run/'wide.log');wideExpected=bytearray(expected[hitCase*92:(hitCase+1)*92]);struct.pack_into('<Q',wideExpected,80,wide);compare(wideOut.read_bytes(),wideExpected,'wide-world-trace',92)
    # Benchmark unknown-name models never hit in this corpus. Exercise the
    # completed unknown (-1 -> FFFF) path on an actual hitting model by giving
    # BOTH implementations an explicit default-only workspace script overlay.
    require(nondefaultHitCase is not None,'Need nondefault hit for unknown property case');case=run/'unknown-surface-hit';case.mkdir();root=case/'overlay';(root/'scripts').mkdir(parents=True);script='default {}';scriptName='scripts/default-only.txt';(root/'scripts/surfaceproperties_manifest.txt').write_text('surfaceproperties_manifest { file "'+scriptName+'" }');(root/scriptName).write_text(script);unknownMounts=case/'mounts.txt';unknownMounts.write_text(str(root)+'\n'+mounts.read_text())
    def ws(s):
        value=s.encode('ascii');return struct.pack('<I',len(value))+value
    payload=struct.pack('<I',1)+ws(scriptName)+ws(script)+struct.pack('<I',0)+struct.pack('<I',79)+b''.join(ws(assets[name]['mdl']) for name in names);surfaceFixture=case/'surface-fixture.bin';surfaceFixture.write_bytes(payload);unknownSurface=case/'surface.bin';execute('surface_native_x86',[GAME/'bin/datacache.dll',libs,surfaceFixture,unknownSurface],case/'surface.log');data=unknownSurface.read_bytes();require(struct.unpack_from('<4I',data)==(1,2,2,0) and struct.unpack_from('<I',data,16)[0]==79,'Controlled surface namespace extent');at=20;unknownProperties=[]
    for _ in names:
        index,n=struct.unpack_from('<iI',data,at);at+=8;boneIds=struct.unpack_from('<'+'i'*n,data,at);at+=n*4;unknownProperties.append((index,boneIds))
    require(at==len(data),'Controlled surface output span');unknownQuery=case/'owned-query.bin';unknownQuery.write_bytes(rows[nondefaultHitCase*44:(nondefaultHitCase+1)*44]);ordinal=(struct.unpack_from('<Q',unknownQuery.read_bytes())[0]>>32)-1;m=struct.unpack_from('<H',props[ordinal],24)[0];require(unknownProperties[m][0]==-1,'Native model property not unknown');wire=bytearray(next(w for i,w in groups[m] if i==nondefaultHitCase));struct.pack_into('<I',wire,64,unknownProperties[m][0]&65535);nativeQuery=case/'native-query.bin';nativeQuery.write_bytes(wire);native=case/'native.bin';ownedUnknown=case/'owned.bin';asset=assets[names[m]];execute('clip_native_x86',[GAME/'bin/engine.dll',libs,asset['phy'],asset['mdl'],nativeQuery,native],case/'native.log');execute('scene_owned_x64',[bsp,unknownMounts,unknownQuery,ownedUnknown],case/'owned.log');compare(ownedUnknown.read_bytes(),native.read_bytes(),'controlled-unknown-surface-hit',92);compare(Path(str(ownedUnknown)+'.events').read_bytes(),Path(str(native)+'.events').read_bytes(),'controlled-unknown-hit-events',8);require(struct.unpack_from('<II',Path(str(native)+'.events').read_bytes())==(1,1) and struct.unpack_from('<I',native.read_bytes(),68)[0]==65535,'Unknown surface actual hit coverage');unknownInfo=bytearray()
    for i,(index,bones) in enumerate(unknownProperties):unknownInfo+=struct.pack('<QiI',((i+1)<<32)|0x12345678,index,len(bones))+struct.pack('<'+'i'*len(bones),*bones)
    compare(Path(str(ownedUnknown)+'.models').read_bytes(),unknownInfo,'controlled-unknown-model-cache',20)
    controlledUnknown=dict(case=nondefaultHitCase,model=names[m],path=str(case),surface_metadata=65535,full_trace_sha256=sha(ownedUnknown))
    # Previously established registration and ordered queries remain unchanged.
    for record in spatial['records']:
        case=run/('spatial-'+record['name']);case.mkdir();old=Path(record['path']);query=old/'queries.bin';require(sha(query)==record['queries_sha256'],'Frozen ordered query identity');output=case/'owned.bin';execute('spatial_regression_x64',[bsp,mounts,query,output],case/'owned.log');require(sha(output)==record['ordered_sha256'] and sha(Path(str(output)+'.registrations'))==record['registration_sha256'] and sha(Path(str(output)+'.stored-boxes'))==record['stored_boxes_sha256'],'Spatial registration/order regression');compare(Path(str(owned)+'.registrations').read_bytes(),Path(str(output)+'.registrations').read_bytes(),'prepared-registration',48)
    safety=execute('scene_safety_x64',[bsp,mounts,run/'safety-files'],run/'scene-safety.log');print(safety,flush=True);spatialSafety=execute('spatial_safety_x64',[bsp,mounts],run/'spatial-safety.log');print(spatialSafety,flush=True)
    for p,s in originals.items():require(sha(Path(p))==s,'Original changed after tests '+p)
    names=['source_loose_collision_scene_assets_candidate.h','loose_collision_scene_test_fixture.h','probe_loose_collision_scene.cpp','test_loose_collision_scene.cpp','build_loose_collision_scene.cmd','verify_loose_collision_scene.py','source_static_prop_point_registry_candidate.h','source_static_prop_point_spatial_candidate.h'];changed={str(ROOT/'work'/p):sha(ROOT/'work'/p) for p in names};sources=dict(changed);pending=[ROOT/'work'/p for p in names]+[ROOT/'work'/p for p in ['probe_static_prop_registry_native.cpp','probe_studio_surface_properties.cpp','probe_static_prop_point_spatial.cpp','test_static_prop_point_spatial.cpp','x64_fsin_cos.asm']];visited=set()
    while pending:
        p=pending.pop()
        if p in visited:continue
        visited.add(p);sources[str(p)]=sha(p)
        if p.suffix in ['.cpp','.h']:
            for local in re.findall(r'^\s*#include\s*"([^"]+)"',p.read_text(),re.M):
                child=p.parent/local
                if child.is_file():pending.append(child)
    logs=[]
    for p in OUT.glob('*-build.log'):
        content=p.read_text();require(not re.search(r'\b(?:warning|error) [A-Z]+\d+',content),'Compiler diagnostic '+str(p));logs.append(dict(path=str(p),sha256=sha(p),warnings=0,errors=0))
    limits=['Owned preparation reads explicit ordered loose mounts/manifest/scripts/MDL/PHY and supplies real resolved surface IDs. This is not live SFM startup, Steam/archive/included-model/async/multisolid support.', 'Original surface/cache reference and original engine/physics clipping are separate processes with controlled model/cache/property/world services. No single whole-engine load/register/world-trace pipeline claim.', 'Per-prop point clipping only; complete world merge/filter/partition composition and scene/generation ownership integration are still required.', 'Surface table retains name/index namespace only; physical/audio/friction material data remain unimplemented.', 'Static-prop lighting guard preserved; no editor build/integration, animation/render/serialization/save changes or dependencies installed.']
    result=dict(status='PASS',path=str(run),models=79,props=485,queries=18430,totals=dict(totals),controlled_unknown_surface_hit=controlledUnknown,trace_differences=0,model_cache_differences=0,registration_differences=0,spatial_regression_queries=spatial['ordered_queries'],safety=safety,spatial_safety=spatialSafety,wide_world_identity=hex(wide),records=records,executables=executables,changed_sources=changed,sources=sources,build_logs=logs,original_hashes=originals,reference_receipts={str(p):sha(p) for p in [SURFACE,SPATIAL,CLIP,SCOPE]},limitations=limits)
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2));(OUT/'source-integrity.json').write_text(json.dumps(dict(status='PASS',authoritative_receipt=str(receipt),receipt_sha256=sha(receipt),sources=sources,changed_sources=changed,executables=executables,build_logs=logs,original_hashes=originals),indent=2))
    lines=['VERIFIED configured loose assets + actual surface IDs + runtime-state point clips.','Whole application PARTIALLY VERIFIED.',str(receipt),'Models=79 props=485 point clips=18430 differences=0',str(dict(totals)),'Additional controlled unknown-name actual collision hit: FFFF metadata/full trace exact','Prior ordered spatial/registration regressions='+str(spatial['ordered_queries'])+' exact',safety,spatialSafety,'Original hashes unchanged='+str(len(originals)),'','Exact freshly built/tested executables:']+[e['path']+' SHA256='+e['sha256'] for e in executables]+['','Changed files:']+list(changed)+['','Scope:']+limits+['','Reproduce:','cmd /c work\\build_loose_collision_scene.cmd','C:\\Users\\wigwo\\AppData\\Local\\Python\\bin\\python3.14.exe work\\verify_loose_collision_scene.py'];(OUT/'verification.txt').write_text('\n'.join(lines)+'\n');(OUT/'remaining-integration.txt').write_text('Next: use runtimeLease in concrete world+props engine-point services; preserve explicit swept flag, Source partition order, filter/ref-handle/overflow behavior and world annotations. Obtain actual original combined scene trace reference before lighting guard removal/editor integration/rebuild.\nConfigured asset/surface preparation now available; caller still supplies correct scene/generation IDs. General archives/include/async/multisolid/solid2/hulls/dynamic spatial mutations and full material physics data incomplete.\nContinue Sol High; goal active, no pause/model switch/whole-project percentage.\n');print('PASS '+str(receipt),flush=True)
if __name__=='__main__':main()
