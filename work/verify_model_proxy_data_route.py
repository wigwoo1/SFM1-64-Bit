"""Original studio/material proxy-data routing boundaries, not a full renderer.

Both libraries run in separate private unchanged-RX children. Observer services
capture raw pointer values; no pointed-to game object is fabricated or consumed.
"""
import hashlib,itertools,json,random,re,shutil,struct,subprocess,tempfile,time
from datetime import datetime,timezone
from pathlib import Path
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/model-proxy-data-route-20261027'
PARENT=ROOT/'outputs/ifm-puppet-proxy-context-20261027/latest-verification.json'
NEW=['probe_model_proxy_data_route.cpp','build_model_proxy_data_route.cmd','verify_model_proxy_data_route.py']
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
def need(b,s):
    if not b:raise RuntimeError(s)

def corpus():
    studio=[(0,0,2,255,0x12345678,0,0,0,0),(0,0,2,255,0,1,1,0,0),(1,0,2,1,0x12345678,0,0,0,0),(2,0,2,255,0x12345678,0,0,0,0)]
    for mask in range(256):studio.append((0,0,2,mask,0x12345678,0,0,0,0))
    for route,flags,count,skin,lod,token in itertools.product(range(3),range(4),range(3),[0,1,2,0xffffffff],range(2),[0,1,0xf1234567]):
        studio.append((route,flags,count,0xa5,token,skin,lod,3,0))
    rng=random.Random(2026102715)
    for _ in range(512):studio.append((rng.randrange(3),rng.randrange(4),rng.randrange(3),rng.getrandbits(8),rng.getrandbits(32),rng.choice([0,1,2,0xffffffff]),rng.randrange(2),rng.getrandbits(32),0))
    material=[(0,0,2,0,0x12345678,0,0,0,0),(0,1,2,0,0,0,0,0,0),(0,4,1,0,0xf1234567,0,0,0,0),(0,2,2,0,1,0,0,0,0)]
    for count,forward,shrink,token in itertools.product([0,1,2,7,31,255],range(2),range(2),[0,1,4,0x7fffffff,0x80000000,0xffff0000,0xffffffff,0xf1234567]):
        material.append((0,forward+2*shrink,count,0,token,0,0,0,0))
    for forward,token in itertools.product(range(2),[0,1,4,0x7fffffff,0x80000000,0xffff0000,0xffffffff,0xf1234567]):material.append((0,forward+4,1,0,token,0,0,0,0))
    for _ in range(512):material.append((0,rng.randrange(2),rng.randrange(256),0,rng.getrandbits(32),0,0,0,0))
    return dict(studio=studio,material=material)

def expected(kind,q):
    route,flags,count,mask,token,skin,lod,body,extra=q
    if kind=='material':
        final=0 if count and flags&2 else 3 if count and flags&4 else count
        calls=1 if count and flags&2 else final
        return final,[(0,0,0)]+[(1,i+1,token) for i in range(calls)]
    if route==2:return count,[]
    if route==1:
        yes=mask&1
        return count,[(0,16,yes)]+([(1,16,token)] if yes else [])
    family=1 if flags&1 else 1 if skin==1 else 0
    meshes=([0,1,0,2] if count>=1 else [])+([2,3,7,3] if count>=2 else [])
    seen=set();events=[]
    for texture in meshes:
        selected=7-texture if family else texture
        if selected in seen:continue
        seen.add(selected)
        material=8*lod+(0 if flags&2 and selected==2 else selected)
        yes=(mask>>(material%8))&1
        events.append((0,material,yes))
        if yes:events.append((1,material,token))
    return count,events

def decode(raw):
    at=0;records=[]
    while at<len(raw):
        need(at+12<=len(raw),'Route header extent');index,final,n=struct.unpack_from('<3I',raw,at);at+=12
        need(n<=512 and at+n*12<=len(raw),'Route event extent')
        events=list(struct.iter_unpack('<3I',raw[at:at+n*12]));at+=n*12;records.append((index,final,events))
    need(at==len(raw),'Route output tail');return records

def main():
    OUT.mkdir(exist_ok=True);ptr=json.loads(PARENT.read_text());parent=Path(ptr['receipt'])
    need(sha(parent)==ptr['receipt_sha256']=='e312d07252e61ca09a83f9b7dfe71358b553051113b6de0271e43c3c1e696dd5','Accepted context receipt')
    prior=json.loads(parent.read_text());frozen=Path(ptr['replay']);need(sha(frozen/'manifest.json')==ptr['replay_manifest_sha256'],'Accepted context manifest');need(all(sha(frozen/n)==h for n,h in json.loads((frozen/'manifest.json').read_text()).items()),'All frozen context evidence')
    protected=[prior[k] for k in ['sources','original_hashes','private_library_hashes','benchmark_material_hashes','fixture_hashes']]
    for group in protected:need(all(sha(n)==h for n,h in group.items()),'Protected input before')
    old_exes=prior['executables']+prior['reused_executables'];need(all(sha(e['path'])==e['sha256'] for e in old_exes),'Previous 39 binaries')
    sources=source_closure([Path(n) for n in prior['sources']]+[ROOT/'work'/n for n in NEW])
    studio=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll')
    engine=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
    material=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/materialsystem.dll'
    for p in [studio,engine]:need(sha(p)==prior['original_hashes'][str(p)],'Pinned library '+str(p))
    need(sha(material)==prior['private_library_hashes'][str(material)],'Pinned private material library')
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
    si,sranges=check_ranges(studio,'studiorender-disassembly.txt',[
        ('studio_proxy_prebind_loop',0x10053cd0,0x10053e5f),('bodypart_model_selector',0x10001d40,0x10001d90),
        ('draw_entry_to_queued_prebind_static',0x1005745f,0x100574af),
        ('immediate_draw_client_data_argument_static',0x10050430,0x1005045d)])
    mi,mranges=check_ranges(material,'materialsystem-disassembly.txt',[
        ('material_proxy_dispatch',0x10004980,0x10004aba),('forward_proxy_dispatch',0x10007700,0x10007711),
        ('context_bind_proxy_data_argument_static',0x10023fed,0x10024014)])
    ei,eranges=check_ranges(engine,'engine-disassembly.txt',[
        ('draw_state_renderable_copy_static',0x100d6807,0x100d680d),
        ('studio_drawinfo_creation_static',0x100db067,0x100db0b8),
        ('studio_draw_slot28_call_static',0x100db14b,0x100db189)])
    need(si.word(0x100710dc+28*4)==0x10057340,'VStudioRender slot28')
    need(mi.word(0x10095d74+53*4)==0x10004980 and mi.word(0x10095a9c+53*4)==0x10007700,'Actual material and forward dispatch slots')
    need(ei.word(0x1040e9b8)==0x103614b4 and ei.string(0x103614b4)=='VStudioRender026' and ei.word(0x1040e9bc)==0x106941d4,'Engine interface table binding')
    start=time.time();build=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_model_proxy_data_route.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build-driver.log').write_text(build.stdout+build.stderr);need(build.returncode==0,'Fresh two reference builds')
    executables=[];logs={}
    for kind in ['material','studio']:
        exe=OUT/(kind+'_route_reference_x86.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        need(exe.stat().st_mtime>=start-2 and struct.unpack_from('<H',raw,pe+4)[0]==0x14c and struct.unpack_from('<H',raw,pe+24)[0]==0x10b and b'asInvoker' in raw,'Fresh x86 reference architecture')
        imports=subprocess.run([str(DUMP),'/imports',str(exe)],capture_output=True,text=True,check=True).stdout;modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',imports,re.I|re.M);need(modules==['KERNEL32.dll'],'Direct import scope')
        log=OUT/(kind+'-build.log');need(not re.search(r'\b(?:warning|error)\s+[A-Z]+\d+\s*:',log.read_text(),re.I),'Clean build '+kind);logs[str(log)]=sha(log);executables.append(dict(path=str(exe),sha256=sha(exe),machine='0x14c',direct_imports=modules))
    def execute(exe,args,label):
        r=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(r.stdout+r.stderr);need(r.returncode==0,label+' failed; inspect retained log');return r.stdout
    counts={};checks=0;data=corpus()
    for kind,rows in data.items():
        query=run/(kind+'-query.bin');query.write_bytes(b''.join(struct.pack('<9I',*q) for q in rows));need(query.read_bytes()[:4*36]==(OUT/(kind+'-explore-query.bin')).read_bytes(),'Retained initial queries '+kind)
        dest=run/(kind+'-output.bin');trace=execute(OUT/(kind+'_route_reference_x86.exe'),[studio if kind=='studio' else material,query,dest],kind)
        records=decode(dest.read_bytes());need(len(records)==len(rows),'All routing cases recorded');events=0
        for i,(q,record) in enumerate(zip(rows,records)):
            final,want=expected(kind,q)
            if record!=(i,final,want):
                (run/(kind+'-difference.json')).write_text(json.dumps(dict(index=i,query=q,actual=record,expected=(i,final,want)),indent=2));raise RuntimeError('Routing mismatch; evidence retained')
            checks+=1;events+=len(want)
        need(trace=='PASS routing_cases='+str(len(rows))+' events='+str(events)+'\n','Actual trace cardinalities')
        exploration=(OUT/(kind+'-explore-output.bin')).read_bytes();need(dest.read_bytes()[:len(exploration)]==exploration,'Retained original observations '+kind)
        counts[kind]=dict(cases=len(rows),events=events,query_sha256=sha(query),output_sha256=sha(dest),bytes=dest.stat().st_size)
    # Preserve the accepted owned value proof and actual puppet-interface
    # observation without rebuilding unchanged programs or claiming co-mapping.
    context=Path(prior['executables'][0]['path']);puppet=run/'puppet-interface.bin'
    execute(context,[prior['ifm_path'],puppet],'prior-puppet');need(sha(puppet)==prior['interface_output_sha256'],'Prior32 actual puppet cases')
    weapon_parent=Path(prior['prior_receipt']);weapon=json.loads(weapon_parent.read_text());owned=Path(weapon['executables'][1]['path']);owned_out=run/'weapon-owned.bin';execute(owned,[run/'absent-original.dll',weapon_parent.parent/'query.bin',owned_out],'prior-weapon-owned');need(sha(owned_out)==weapon['output_sha256'],'All15664 owned weapon outputs unchanged')
    buildinfo=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text());need(len(buildinfo['editor_source_dependencies'])==310 and all(sha(ROOT/n)==h for n,h in buildinfo['editor_source_dependencies'].items()) and sha(ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe')==prior['editor_sha256'],'Editor unchanged')
    for group in [sources,*protected]:need(all(sha(n)==h for n,h in group.items()),'Protected final input identities')
    need(all(sha(e['path'])==e['sha256'] for e in old_exes) and not (run/'absent-original.dll').exists(),'Prior executable and absent-library identities')
    result=dict(status='PASS',goal_status='ACTIVE / INCOMPLETE',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',scope='Actual studio prebind material selection/raw proxy-data forwarding and actual material/queue-friendly per-proxy dispatch boundaries with observers; renderer-to-client composition incomplete',routing=counts,independent_case_checks=checks,original_static_ranges=dict(studio=sranges,material=mranges,engine=eranges),sources=sources,new_sources=NEW,prior_sources_unchanged=len(prior['sources']),original_hashes=prior['original_hashes'],private_library_hashes=prior['private_library_hashes'],benchmark_material_hashes=prior['benchmark_material_hashes'],fixture_hashes=prior['fixture_hashes'],executables=executables,reused_executables=old_exes,build_logs=logs,prior_puppet_cases=32,prior_weapon_cases=15664,prior_owned_executable=weapon['executables'][1],prior_receipt=str(parent),prior_receipt_sha256=sha(parent),editor_dependencies_unchanged=310,editor_sha256=prior['editor_sha256'],editor_rebuilt=False,editor_runtime_test='NOT RUN',limits=['Studio method10053CD0 is the explicit prebind path observed on the queued branch; immediate actual mesh/material Bind path still needs full routing proof','Studio/material/client/IFM are separate preferred-base original children; boundary spies are not combined original renderer/material/proxy execution','CMaterial dispatch is measured with explicit normal debug mode0 and system guard false; debug modes/guarded value notifications remain unverified','Controlled studio metadata covers two bodies, two skin families/LODs, duplicate texture indices, aliased material pointers and override mode0/1; not a general model format/renderer proof','Raw opaque tokens are forwarded without dereference in spies; their real lifetime/authenticated puppet origin still needs a production adapter proof','No existing source changes, proxy registration, editor integration, fullpilot acceptance, original writes or live SFM'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');replay=OUT/('replay-'+run.name.removeprefix('verification-'));replay.mkdir()
    included=[Path(n) for n in sources]+[Path(e['path']) for e in executables]+[Path(n) for n in logs]+[receipt]+list(run.glob('*.log'))+list(run.glob('*.bin'))
    for p in included:
        dest=replay/p.relative_to(ROOT);dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,dest)
    manifest={p.relative_to(replay).as_posix():sha(p) for p in replay.rglob('*') if p.is_file()};(replay/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt),receipt_sha256=sha(receipt),replay=str(replay),replay_manifest_sha256=sha(replay/'manifest.json')),indent=2)+'\n')
    print(json.dumps(dict(status='PASS',routing=counts,checks=checks)),flush=True)

if __name__=='__main__':main()
